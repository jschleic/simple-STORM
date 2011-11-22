/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*      Copyright 2010-2011 by Joachim Schleicher and Ullrich Koethe    */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/
 

#include <vigra/stdconvolution.hxx>
#include <vigra/convolution.hxx>
#include <vigra/recursiveconvolution.hxx>
#include <vigra/resizeimage.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/fftw3.hxx> 
#include <vigra/localminmax.hxx>
#include <vigra/splineimageview.hxx>
#include <set>
#include <fstream>
#include <iomanip>
#ifdef OPENMP_FOUND
    #include <omp.h>
#endif //OPENMP_FOUND

#include "util.h"
#include "fftfilter.hxx"
#include "myimportinfo.h"

using namespace vigra;
using namespace vigra::functor;

/**
 * @file dSTORM data processing for localization microscopy
 * 
 * This file contains functions to localizes single molecule 
 * point-spread-functions by interpolation of each image in an
 * image stack after prefiltering. A Wiener filter can be learned
 * from good-quality input data and afterwards applied to 
 * low SNR-measurements.
 * 
 * For algorithmic details and performance measurements please refer to
 * the diploma thesis available at
 * http://hci.iwr.uni-heidelberg.de/Staff/jschleic/
 * 
 * Some helper functions can be found in util.h, and the filter in Fourier 
 * domain is applied using fftw-wrappers in fftfilter.h.
 * 
 * @date 2010-2011 Diploma thesis J. Schleicher
 */

//--------------------------------------------------------------------------
// helper classes and functions
//--------------------------------------------------------------------------

/**
 * BSpline coefficients but no prefiltering
 */
template <int ORDER, class T>
class BSplineWOPrefilter
 : public BSpline<ORDER, T> {

    public:
    /** Prefilter coefficients
        (array has zero length, since image is already prefiltered).
    */
    ArrayVector<double> const & prefilterCoefficients() const
    {
        static ArrayVector<double> b;
        return b;
    }
};

/**
 * Class to keep an image coordinate with corresponding pixel value
 * 
 * This corresponds to a vigra::Point2D with an additional value at that coordinate.
 */
template <class VALUETYPE>
class Coord{
    public:
        typedef VALUETYPE value_type;
        Coord(const int x_,const int y_,const VALUETYPE val_,const VALUETYPE asym_=1.) 
            : x(x_), y(y_), val(val_), asymmetry(asym_) {  }
        int x;
        int y;
        VALUETYPE val;
        VALUETYPE asymmetry;

        bool operator<(const Coord<VALUETYPE>& c2) const {
            return ((this->y==c2.y)&&(this->x < c2.x)) || (this->y < c2.y);
        }

        // Coords are equal, if they're at exactly the same position and have the same value
        bool operator==(const Coord<VALUETYPE>& c2) const {
            bool ret = (this->x == c2.x) && (this->y == c2.y) && (this->val == c2.val);
            return ret;
        }
};


// hack to push the coordinates into an array instead of marking them in 
// a target image.
// This is used as an accessor although it doesn't access the pixel values ;-)
// To work on ROIs, a global offset can be set with setOffset().
template <class T, class ITERATOR>
class VectorPushAccessor{
    public:
        typedef typename T::value_type value_type;
        VectorPushAccessor(std::set<T>& arr, ITERATOR it_start)
            : m_arr(arr), m_it_start(it_start), m_offset() {        }

        T const &   operator() (ITERATOR const &i) const {
            return NumericTraits<T>::zero();
        }
        template<class V>
        void    set (V const & /*value*/, ITERATOR const &i) {
            int x = i.x+m_offset.x;
            int y = i.y-m_it_start.y+m_offset.y;
            typename T::value_type val = *i;
            T c (x,y,val);
            m_arr.insert(c);
        }
        void setOffset(Diff2D offset) {
            m_offset = offset;
        }

    private:
        std::set<T>& m_arr;
        ITERATOR m_it_start;
        Diff2D m_offset;
};

/**
 * Draw coordinates from all frames into the result image
 */
template <class C, class Image>
void drawCoordsToImage(const std::vector<std::set<C> >& coords, Image& res) {
    res = 0;
    typename std::vector<std::set<C> >::const_iterator it;
    //  loop over the images
    for(it = coords.begin(); it != coords.end(); ++it) {
        drawCoordsToImage( *it, res);
    }
}

/**
 *  Draw coordinates detected in one frame into the resulting image
 */
template <class C, class Image>
void drawCoordsToImage(const std::set<C>& coords, Image& res) {
    //  loop over the coordinates
    typename std::set<C>::iterator it2;

    for(it2 = coords.begin(); it2 != coords.end(); it2++) {
        C c = *it2;
        res(c.x, c.y) += c.val;
    }
}

template <class C>
int saveCoordsFile(const std::string& filename, const std::vector<std::set<C> >& coords, 
            const MultiArrayShape<3>::type & shape, const int factor) {
    int numSpots = 0;
    std::set<Coord<float> >::iterator it2;
    std::ofstream cfile (filename.c_str());
    cfile << shape[0] << " " << shape[1] << " " << shape[2] << std::endl;
    cfile << std::fixed; // fixed instead of scientific format
    for(unsigned int j = 0; j < coords.size(); j++) {
        for(it2=coords[j].begin(); it2 != coords[j].end(); it2++) {
            numSpots++;
            Coord<float> c = *it2;
            cfile << std::setprecision(3) << (float)c.x/factor << " " << (float)c.y/factor << " "
                << j << " " << std::setprecision(1) << c.val << " " << std::setprecision(3) << c.asymmetry << std::endl;
        }
    }
    cfile.close();
    return numSpots;
}

/** 
 * finds the value, so that the given percentage of pixels is above / below that value.
 */
template <class Image>
void findMinMaxPercentile(Image& im, double minPerc, double& minVal, double maxPerc, double& maxVal) {
    std::vector<typename Image::value_type> v;
    for(int y=0; y<im.height(); y++) {
        for(int x=0; x<im.width(); x++) {
            v.push_back(im[y][x]);
        }
    }
    std::sort(v.begin(),v.end());
    minVal=v[(int)(v.size()*minPerc)];
    maxVal=v[(int)(v.size()*maxPerc)];
}

/**
 * Add asymmetry to the coordinates list
 */
template <class SrcIterator, class SrcAccessor, class T>
inline void determineAsymmetry(triple<SrcIterator, SrcIterator, SrcAccessor> s,
        std::set<Coord<T> >& coords,
        const int factor) {
    determineAsymmetry(s.first, s.second, s.third, coords, factor);
}

template <class SrcIterator, class SrcAccessor, class T>
void determineAsymmetry(SrcIterator srcUpperLeft,
        SrcIterator srcLowerRight,
        SrcAccessor acc,
        std::set<Coord<T> >& coords,
        const int factor) {
    vigra::SplineImageView<3,T> sview(srcUpperLeft, srcLowerRight, acc, true);
    std::set<Coord<float> > newcoords;
    std::set<Coord<float> >::iterator it2;
    for(it2 = coords.begin(); it2 != coords.end(); it2++) {
        const Coord<float>& c = *it2;
        T sxx = sview.dxx((float)(c.x)/factor, (float)(c.y)/factor);
        T syy = sview.dyy((float)(c.x)/factor, (float)(c.y)/factor);
        T sxy = sview.dxy((float)(c.x)/factor, (float)(c.y)/factor);
        // calculate the eigenvalues
        T ev1 = (sxx+syy)/2. - sqrt((sxx+syy)*(sxx+syy)/4. + sxy*sxy - sxx*syy);
        T ev2 = (sxx+syy)/2. + sqrt((sxx+syy)*(sxx+syy)/4. + sxy*sxy - sxx*syy);
        Coord<float> cc (c.x, c.y, c.val, ev1/ev2);
        newcoords.insert(cc); // copy for now. Hack hack hack...
    }
    coords=newcoords;
}


//--------------------------------------------------------------------------
// GENERATE WIENER FILTER
//--------------------------------------------------------------------------
// Most of the following functions are available twice: Once taking a
// MultiArrayView (data) as input and once with MyImportInfo (filepointer).
// Since the algorithms work on single-frames only, there is no need to
// put the complete dataset into RAM but every frame can be read from disk
// when needed. The class MyImportInfo transparently handles hdf5 and sif
// input file pointers.

/**
 * Calculate Power-Spektrum
 */
template <class T, class DestIterator, class DestAccessor>
void powerSpectrum(const MultiArrayView<3, T>& array, 
                   DestIterator res_ul, DestAccessor res_acc) {
    unsigned int stacksize = array.size(2);
    unsigned int w = array.size(0);
    unsigned int h = array.size(1); 
    vigra::DImage ps(w, h);
    vigra::DImage ps_center(w, h);
    ps = 0;
    
    for(unsigned int i = 0; i < stacksize; i++) {
        MultiArrayView <2, T> array2 = array.bindOuter(i); // select current image
        BasicImageView<T> input = makeBasicImageView(array2);  // access data as BasicImage     

        vigra::FFTWComplexImage fourier(w, h);
        fourierTransform(srcImageRange(input), destImage(fourier));
        
        // there is no squared magnitude accessor, so we use the magnitude here
        vigra::combineTwoImages(srcImageRange(ps), 
                srcImage(fourier, FFTWSquaredMagnitudeAccessor<double>()), 
                destImage(ps), Arg1()+Arg2());
        
        helper::progress(i, stacksize); // report progress
    }
    std::cout << std::endl;

    moveDCToCenter(srcImageRange(ps), destImage(ps_center));
    vigra::transformImage(
            srcImageRange(ps_center), 
            destIter(res_ul, res_acc), 
            Arg1() / Param(stacksize));
}

/**
 * Calculate Power-Spektrum
 */
template <class DestIterator, class DestAccessor>
void powerSpectrum(const MyImportInfo& info, 
                   DestIterator res_ul, DestAccessor res_acc) {
    unsigned int stacksize = info.shapeOfDimension(2);
    unsigned int w = info.shapeOfDimension(0);
    unsigned int h = info.shapeOfDimension(1); 
    typedef float T;
    MultiArray<3, T> im(Shape3(w,h,1));
    vigra::DImage ps(w, h);
    vigra::DImage ps_center(w, h);
    ps = 0;
    
    for(unsigned int i = 0; i < stacksize; i++) {
        readBlock(info, Shape3(0,0,i), Shape3(w,h,1), im);
        MultiArrayView <2, T> array2 = im.bindOuter(0); // select current image
        BasicImageView<T> input = makeBasicImageView(array2);  // access data as BasicImage     

        vigra::FFTWComplexImage fourier(w, h);
        fourierTransform(srcImageRange(input), destImage(fourier));
        
        // there is no squared magnitude accessor, so we use the magnitude here
        vigra::combineTwoImages(srcImageRange(ps), 
                srcImage(fourier, FFTWSquaredMagnitudeAccessor<double>()), 
                destImage(ps), Arg1()+Arg2());
        
        helper::progress(i, stacksize); // report progress
    }
    std::cout << std::endl;

    moveDCToCenter(srcImageRange(ps), destImage(ps_center));
    vigra::transformImage(
            srcImageRange(ps_center), 
            destIter(res_ul, res_acc), 
            Arg1() / Param(stacksize));
}


template <class DestIterator, class DestAccessor, class StormDataSet>
inline
void powerSpectrum(
                   const StormDataSet& im,
                   pair<DestIterator, DestAccessor> ps)
{
    powerSpectrum(im, ps.first, ps.second);
}

/**
 * Estimate Noise Power
 */
template <class SrcIterator, class SrcAccessor>
typename SrcIterator::value_type estimateNoisePower(int w, int h,
        SrcIterator is, SrcIterator end, SrcAccessor as)
{
    typedef double sum_type; // use double here since the sum can get larger than float range

    vigra::FindSum<sum_type> sum;   // init functor
    vigra::FindSum<sum_type> sumROI;   // init functor
    vigra::BImage mask(w,h);
    mask = 0;
    // TODO: this size should depend on image dimensions!
    for(int y = 10; y < h-10; y++) {  // select center of the fft image
        for(int x = 10; x < w-10; x++) {
            mask(x,y) = 1;
        }
    }
    vigra::inspectImage(is, end, as, sum);
    vigra::inspectImageIf(is, end, as, mask.upperLeft(), mask.accessor(), sumROI);

    sum_type s = sum() - sumROI();
    return s / (w*h - (w-20)*(h-20));
}


template <class SrcIterator, class SrcAccessor>
inline
typename SrcIterator::value_type estimateNoisePower(int w, int h,
                   triple<SrcIterator, SrcIterator, SrcAccessor> ps)
{
    return estimateNoisePower(w, h, ps.first, ps.second, ps.third);
}

/**
 * Construct Wiener Filter using noise power estimated 
 * at high frequencies.
 */
// Wiener filter is defined as 
// H(f) = (|X(f)|)^2/[(|X(f)|)^2 + (|N(f)|)^2]
// where X(f) is the power of the signal and 
// N(f) is the power of the noise
// (e.g., see http://cnx.org/content/m12522/latest/)
template <class T, class DestImage, class StormData>
void constructWienerFilter(StormData& im, 
                DestImage& dest) {

    int w = im.shape(0);
    int h = im.shape(1);
    BasicImage<T> ps(w,h);
    powerSpectrum(im, destImage(ps));
    T noise = estimateNoisePower(w,h,srcImageRange(ps));
    // mtf = ps - noise #remove noise power
    // mtf[mtf < 0] = 0
    // return mtf / ps
    transformImage(srcImageRange(ps),   
            destImage(ps), 
            ifThenElse(Arg1()-Param(noise)>Param(0.), (Arg1()-Param(noise))/Arg1(), Param(0.)));
    moveDCToUpperLeft(srcImageRange(ps), destImage(dest));

}


/** 
 Generate a filter for enhancing the image quality in fourier space.
 Either using constructWienerFilter() or by loading the given file.
  
 @param filter if this file exists, load it. Otherwise create a filter
        from the data and save it to file 'filter'
 @param in 3-dimensional measurement as MultiArrayView<3,float> or MyImageInfo
*/
template <class T, class StormDataSet>
void generateFilter(StormDataSet& in, BasicImage<T>& filter, const std::string& filterfile) {
    bool constructNewFilter = true;
    if(filterfile != "" && helper::fileExists(filterfile)) {
        vigra::ImageImportInfo filterinfo(filterfile.c_str());

        if(filterinfo.isGrayscale())
        {
            std::cout << "using filter from file " << filterfile << std::endl;
            vigra::BasicImage<T> filterIn(filterinfo.width(), filterinfo.height());
            vigra::importImage(filterinfo, destImage(filterIn)); // read the image
            vigra::resizeImageSplineInterpolation(srcImageRange(filterIn), destImageRange(filter));
            constructNewFilter = false;
        }
        else
        {
            std::cout << "filter image must be grayscale" << std::endl;
        }
    }
    if(constructNewFilter) {
        std::cout << "generating wiener filter from the data" << std::endl;
        constructWienerFilter<T>(in, filter);
        vigra::exportImage(srcImageRange(filter), filterfile.c_str()); // save to disk
    }
    
}

//--------------------------------------------------------------------------
// STORM DATA PROCESSING
//--------------------------------------------------------------------------

/** 
 * Estimate Background level and subtract it from the image
 */
template <class Image>
void subtractBackground(Image& im, Image& bg) {
    float sigma = 10.; // todo: estimate from data
    vigra::recursiveSmoothX(srcImageRange(im), destImage(bg), sigma);
    vigra::recursiveSmoothY(srcImageRange(bg), destImage(bg), sigma);

    vigra::combineTwoImages(srcImageRange(im), srcImage(bg), destImage(im), Arg1()-Arg2());
}

/**
 * Prefilter for BSpline-Interpolation
 */
template <int ORDER, class Image>
void prefilterBSpline(Image& im) {
    ArrayVector<double> const & b = BSpline<ORDER, double>().prefilterCoefficients();

    for(unsigned int i=0; i<b.size(); ++i)
    {
        recursiveFilterX(srcImageRange(im), destImage(im), b[i], BORDER_TREATMENT_REFLECT);
        recursiveFilterY(srcImageRange(im), destImage(im), b[i], BORDER_TREATMENT_REFLECT);
    }
}

/**
 * Localize Maxima of the spots and return a list with coordinates
 * 
 * This is the actual loop over a microscopic image stack to 
 * reconstruct a super-resolution image out of single molecule detections.
 * 
 * The localization is done on per-frame basis in wienerStormSingleFrame()
 * 
 * @param im MultiArrayView on the actual image data
 */
template <class T>
void wienerStorm(const MultiArrayView<3, T>& im, const BasicImage<T>& filter, 
            std::vector<std::set<Coord<T> > >& maxima_coords, 
            const T threshold=800, const int factor=8, const int mylen=9,
            const std::string &frames="", const char verbose=0) {
    
    unsigned int stacksize = im.size(2);
    unsigned int w = im.size(0);
    unsigned int h = im.size(1);
    unsigned int w_xxl = factor*(w-1)+1;
    unsigned int h_xxl = factor*(h-1)+1;
    unsigned int i_stride=1;
    int i_beg=0, i_end=stacksize;
    if(frames!="") {
        helper::rangeSplit(frames, i_beg, i_end, i_stride);
        if(i_beg < 0) i_end = stacksize+i_beg; // allow counting backwards from the end
        if(i_end < 0) i_end = stacksize+i_end; // allow counting backwards from the end
        if(verbose) std::cout << "processing frames [" << i_beg << ":" 
            << i_end << ":" << i_stride << "]" << std::endl;
    }
    
    // TODO: Precondition: res must have size (factor*(w-1)+1, factor*(h-1)+1)
    // filter must have the size of input


    // initialize fftw-wrapper; create plans
    BasicImageView<T> sampleinput = makeBasicImageView(im.bindOuter(0));  // access first frame as BasicImage
    FFTFilter<T> fftwWrapper(sampleinput);

    std::cout << "Finding the maximum spots in the images..." << std::endl;
    helper::progress(-1,-1); // reset progress

    //over all images in stack
    #pragma omp parallel for schedule(static, CHUNKSIZE)
    for(int i = i_beg; i < i_end; i+=i_stride) {
        MultiArrayView <2, T> array = im.bindOuter(i); // select current image

        wienerStormSingleFrame(array, filter, maxima_coords[i], 
                fftwWrapper, // TODO (this is no real function argument but should be global)
                threshold, factor, mylen, verbose);

        #ifdef OPENMP_FOUND
        if(omp_get_thread_num()==0) { // master thread
            helper::progress(i+1, i_end); // update progress bar
        }
        #else
            helper::progress(i+1, i_end); // update progress bar
        #endif //OPENMP_FOUND       
    }
    std::cout << std::endl;
}

/**
 * Localize Maxima of the spots and return a list with coordinates
 * 
 * This is the actual loop over a microscopic image stack to 
 * reconstruct a super-resolution image out of single molecule detections.
 * 
 * The localization is done on per-frame basis in wienerStormSingleFrame()
 * 
 * @param info MyImportInfo file info containing the image stack
 */
template <class T>
void wienerStorm(const MyImportInfo& info, const BasicImage<T>& filter, 
            std::vector<std::set<Coord<T> > >& maxima_coords, 
            const T threshold=800, const int factor=8, const int mylen=9,
            const std::string &frames="", const char verbose=0) {

    unsigned int stacksize = info.shape(2);
    unsigned int w = info.shape(0);
    unsigned int h = info.shape(1);
    unsigned int w_xxl = factor*(w-1)+1;
    unsigned int h_xxl = factor*(h-1)+1;
    unsigned int i_stride=1;
    int i_beg=0, i_end=stacksize;
    if(frames!="") {
        helper::rangeSplit(frames, i_beg, i_end, i_stride);
        if(i_beg < 0) i_end = stacksize+i_beg; // allow counting backwards from the end
        if(i_end < 0) i_end = stacksize+i_end; // allow counting backwards from the end
        if(verbose) std::cout << "processing frames [" << i_beg << ":" 
            << i_end << ":" << i_stride << "]" << std::endl;
    }
    
    // TODO: Precondition: res must have size (factor*(w-1)+1, factor*(h-1)+1)
    // filter must have the size of input
    MultiArray<3, T> im(Shape3(w,h,1));

    // initialize fftw-wrapper; create plans
    readBlock(info, Shape3(0,0,0), Shape3(w,h,1), im);
    BasicImageView<T> sampleinput = makeBasicImageView(im.bindOuter(0));  // access first frame as BasicImage
    FFTFilter<T> fftwWrapper(srcImageRange(sampleinput));

    #ifndef STORM_QT // silence stdout
    std::cout << "Finding the maximum spots in the images..." << std::endl;
    #endif // STORM_QT
    helper::progress(-1,-1); // reset progress

    //over all images in stack
    #pragma omp parallel for schedule(static, CHUNKSIZE) firstprivate(im)
    for(int i = i_beg; i < i_end; i+=i_stride) {
        readBlock(info, Shape3(0,0,i), Shape3(w,h,1), im);
        MultiArrayView <2, T> array = im.bindOuter(0); // select current image

        wienerStormSingleFrame(array, filter, maxima_coords[i], 
                fftwWrapper, // TODO (this is no real function argument but should be global)
                threshold, factor, mylen, verbose);

        #ifdef OPENMP_FOUND
        if(omp_get_thread_num()==0) { // master thread
            helper::progress(i+1, i_end); // update progress bar
        }
        #else
            helper::progress(i+1, i_end); // update progress bar
        #endif //OPENMP_FOUND       
    }
    #ifndef STORM_QT // silence stdout
    std::cout << std::endl;
    #endif // STORM_QT
}

template <class T>
void wienerStormSingleFrame(const MultiArrayView<2, T>& in, const BasicImage<T>& filter, 
            std::set<Coord<T> >& maxima_coords, 
            FFTFilter<T> & fftwWrapper,
            const T threshold=800, const int factor=8, const int mylen=9,
            const char verbose=0) {

    unsigned int w = in.shape(0); // width
    unsigned int h = in.shape(1); // height
    BasicImage<T> filtered(w,h);
    BasicImage<T> bg(w,h);        // background
    // ROI:
    const int mylen2 = mylen/2;
    unsigned int w_roi = factor*(mylen-1)+1;
    unsigned int h_roi = factor*(mylen-1)+1;
    BasicImage<T> im_xxl(w_roi, h_roi);

    BasicImageView<T> input = makeBasicImageView(in);  // access data as BasicImage

    //fft, filter with Wiener filter in frequency domain, inverse fft, take real part
    BasicImageView<T> filteredView(filtered.data(), filtered.size());
    fftwWrapper.applyFourierFilter(srcImageRange(input), srcImage(filter), destImage(filteredView));
    //~ vigra::gaussianSmoothing(srcImageRange(input), destImage(filtered), 1.2);
    subtractBackground(filtered, bg);
    vigra::FindMinMax<T> bgMinmax;
    vigra::inspectImage(srcImageRange(bg), bgMinmax);
    T baseline = bgMinmax.min;

    std::set<Coord<T> > maxima_candidates_vect;  // we use a set for the coordinates to automatically squeeze duplicates 
                                                 // (from overlapping ROIs)
    VectorPushAccessor<Coord<T>, typename BasicImage<T>::const_traverser> maxima_candidates(maxima_candidates_vect, filtered.upperLeft());
    vigra::localMaxima(srcImageRange(filtered), destImage(filtered, maxima_candidates), vigra::LocalMinmaxOptions().threshold(threshold));

    VectorPushAccessor<Coord<T>, typename BasicImage<T>::const_traverser> maxima_acc(maxima_coords, im_xxl.upperLeft());

    //upscale filtered image regions with spline interpolation
    std::set<Coord<float> >::iterator it2;
    for(it2=maxima_candidates_vect.begin(); it2 != maxima_candidates_vect.end(); it2++) {
            Coord<float> c = *it2;
            if(filtered(c.x,c.y)<(bg(c.x,c.y)-baseline)) { // skip very low signals
                continue;
            }
            Diff2D roi_ul (c.x-mylen2, c.y-mylen2);
            Diff2D roi_lr (c.x-mylen2+mylen, c.y-mylen2+mylen);

            Diff2D xxl_ul (0, 0);  // offset in xxl image
            Diff2D xxl_lr (0, 0);

            // Maxima-Candidates near the border
            if(c.x-mylen2<0 || c.y-mylen2<0 || c.x-mylen2+mylen>(int)w || c.y-mylen2+mylen>(int)h) {
                Diff2D _roi_ul (
                    ((c.x-mylen2)<0) ? 0 : (c.x-mylen2),
                    ((c.y-mylen2)<0) ? 0 : (c.y-mylen2) );
                Diff2D _roi_lr (
                    ((c.x-mylen2+mylen)>(int)w) ? w : (c.x-mylen2+mylen),
                    ((c.y-mylen2+mylen)>(int)h) ? h : (c.y-mylen2+mylen) );

                xxl_ul += (_roi_ul-roi_ul)*factor; // offset in xxl image
                xxl_lr += (_roi_lr-roi_lr)*factor;
                roi_ul = _roi_ul;
                roi_lr = _roi_lr;
            }

            vigra::resizeImageSplineInterpolation(
                    srcIterRange(filtered.upperLeft()+roi_ul, filtered.upperLeft()+roi_lr), 
                    destIterRange(im_xxl.upperLeft()+xxl_ul, im_xxl.lowerRight()+xxl_lr),
                    BSplineWOPrefilter<3,double>());
            // find local maxima that are above a given threshold
            // at least the values should be above background+baseline
            // here we include only internal pixels, no border
            // to get every maximum only once, the maxima are pushed into a std::set
            maxima_acc.setOffset(Diff2D(factor*(c.x-mylen2), factor*(c.y-mylen2)));
            vigra::localMaxima(srcIterRange(im_xxl.upperLeft()+xxl_ul+Diff2D(factor,factor), im_xxl.lowerRight()+xxl_lr-Diff2D(factor,factor)),
                    destIter(im_xxl.upperLeft()+xxl_ul+Diff2D(factor,factor), maxima_acc), vigra::LocalMinmaxOptions().threshold(threshold));
    }
    determineAsymmetry(srcImageRange(filtered), maxima_coords, factor);
}
