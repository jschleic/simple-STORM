/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*         Copyright 2010 by Joachim Schleicher and Ullrich Koethe      */
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
#include <vigra/resizeimage.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/fftw3.hxx> 
#include <vigra/localminmax.hxx>
#include <set>

using namespace vigra;
using namespace vigra::functor;


template <class T, class DestIterator, class DestAccessor>
void powerSpectrum(MultiArrayView<3, T>& array, 
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
		
		if(i%100==99) {
			std::cout << i+1 << " ";
			flush(std::cout);
		}
	}
	std::cout << std::endl;

    moveDCToCenter(srcImageRange(ps), destImage(ps_center));
	vigra::transformImage(
			srcImageRange(ps_center), 
			destIter(res_ul, res_acc), 
			Arg1() / Param(stacksize));
}


template <class T, 
		  class DestIterator, class DestAccessor>
inline
void powerSpectrum(
                   MultiArrayView<3, T>& im,
                   pair<DestIterator, DestAccessor> ps)
{
    powerSpectrum(im, ps.first, ps.second);
}


template <class SrcIterator, class SrcAccessor>
typename SrcIterator::value_type estimateNoisePower(int w, int h,
		SrcIterator is, SrcIterator end, SrcAccessor as)
{
	typedef typename SrcIterator::value_type src_type;

    vigra::FindSum<src_type> sum;   // init functor
    vigra::FindSum<src_type> sumROI;   // init functor
	vigra::BImage mask(w,h);
	mask = 0;
	for(int y = 10; y < h-10; y++) {  // select center of the fft image
		for(int x = 10; x < w-10; x++) {
			mask(x,y) = 1;
		}
	}
    vigra::inspectImage(is, end, as, sum);
    vigra::inspectImageIf(is, end, as, mask.upperLeft(), mask.accessor(), sumROI);

	src_type s = sum() - sumROI();
	return s / (w*h - (w-20)*(h-20));
}


template <class SrcIterator, class SrcAccessor>
inline
typename SrcIterator::value_type estimateNoisePower(int w, int h,
                   triple<SrcIterator, SrcIterator, SrcAccessor> ps)
{
    return estimateNoisePower(w, h, ps.first, ps.second, ps.third);
}


template <class T, class DestImage>
void constructWienerFilter(MultiArrayView<3, T>& im, 
				DestImage& dest) {
    // Wiener filter is defined as 
    // H(f) = (|X(f)|)^2/[(|X(f)|)^2 + (|N(f)|)^2]
    // where X(f) is the power of the signal and 
    // N(f) is the power of the noise
    // (e.g., see http://cnx.org/content/m12522/latest/)

	int w = im.size(0);
	int h = im.size(1);
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

// struct to keep an image coordinate
template <class VALUETYPE>
class Coord{
	public:
		typedef VALUETYPE value_type;
		Coord(const int x_,const int y_,const VALUETYPE val_) : x(x_), y(y_), val(val_) {  }
		int x;
		int y;
		VALUETYPE val;
		bool operator<(const Coord<VALUETYPE>& c2) const {
			return ((this->y==c2.y)&&(this->x < c2.x)) || (this->y < c2.y);
		}

};


// hack to push the coordinates into an array instead of marking them in 
// a target image
template <class T, class ITERATOR>
class VectorPushAccessor{
	public:
		typedef typename T::value_type value_type;
		VectorPushAccessor(std::set<T>& arr, ITERATOR it_start) 
			: m_arr(arr), m_it_start(it_start), m_offset() {		}
	

		T const & 	operator() (ITERATOR const &i) const {
			return NumericTraits<T>::zero();
		}
		//~ template<class V , class ITERATOR , class DIFFERENCE >
		//~ void 	set (V const &value, ITERATOR const &i, DIFFERENCE const &diff) const
		template<class V>
		void 	set (V const &value, ITERATOR const &i) {
			int x = i.x+m_offset.x;
			int y = i.y-m_it_start.y+m_offset.y;
			typename T::value_type val = *i;
			T c (x,y,val);
			m_arr.insert(c);
		}
		unsigned int size() {
			   return m_arr.size();
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
 Generate a filter for enhancing the image quality in fourier space.
 Either using constructWienerFilter() or by loading the given file.
*/
template <class T>
void generateFilter(MultiArrayView<3, T>& in, BasicImage<T>& filter, std::string& filterfile) {
	bool constructNewFilter = true;
	if(filterfile != "") {
		vigra::ImageImportInfo filterinfo(filterfile.c_str());

		if(filterinfo.isGrayscale())
		{
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
		constructWienerFilter(in, filter);
	}
	
}

/** finds value, so that the given percentage of pixels is above / below 
  the value.
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
 Localize Maxima of the spots and return a list with coordinates
*/
template <class T>
void wienerStorm(MultiArrayView<3, T>& im, BasicImage<T>& filter, 
			std::vector<std::set<Coord<T> > >& maxima_coords, 
			T threshold=800, int factor=8) {
	
	unsigned int stacksize = im.size(2);
	unsigned int w = im.size(0);
	unsigned int h = im.size(1);
	unsigned int w_xxl = factor*(w-1)+1;
	unsigned int h_xxl = factor*(h-1)+1;
    
	// TODO: Precondition: res must have size (factor*(w-1)+1, factor*(h-1)+1)
	// filter must have the size of input

    BasicImage<T> filtered(w,h);
	const int mylen = 9; // todo: what is an appropriate roi?
	const int mylen2 = mylen/2;
	unsigned int w_roi = factor*(mylen-1)+1;
	unsigned int h_roi = factor*(mylen-1)+1;
	BasicImage<T> im_xxl(w_roi, h_roi);
    
    std::cout << "Finding the maximum spots in the images..." << std::endl;
    //over all images in stack
    for(unsigned int i = 0; i < stacksize; i++) {
		MultiArrayView <2, T> array = im.bindOuter(i); // select current image

		BasicImageView<T> input = makeBasicImageView(array);  // access data as BasicImage
		
        //fft, filter with Wiener filter in frequency domain, inverse fft, take real part
        vigra::applyFourierFilter(srcImageRange(input), srcImage(filter), destImage(filtered));
        //~ vigra::gaussianSmoothing(srcImageRange(input), destImage(filtered), 1.2);

		std::set<Coord<T> > maxima_candidates_vect;
		VectorPushAccessor<Coord<T>, typename BasicImage<T>::const_traverser> maxima_candidates(maxima_candidates_vect, filtered.upperLeft());
		vigra::localMaxima(srcImageRange(filtered), destImage(filtered, maxima_candidates), vigra::LocalMinmaxOptions().threshold(threshold-4*factor));

		VectorPushAccessor<Coord<T>, typename BasicImage<T>::const_traverser> maxima_acc(maxima_coords[i], im_xxl.upperLeft());

		//upscale filtered image regions with spline interpolation
		std::set<Coord<float> >::iterator it2;
		for(it2=maxima_candidates_vect.begin(); it2 != maxima_candidates_vect.end(); it2++) {
				Coord<float> c = *it2;
				Diff2D roi_ul (c.x-mylen2, c.y-mylen2);
				Diff2D roi_lr (c.x-mylen2+mylen, c.y-mylen2+mylen);

				Diff2D xxl_ul (0, 0);  // offset in xxl image
				Diff2D xxl_lr (0, 0);

				// TODO: Pixels near the border
				if(c.x-mylen2<0 || c.y-mylen2<0 || c.x-mylen2+mylen>w || c.y-mylen2+mylen>h) {
					Diff2D _roi_ul (
						((c.x-mylen2)<0) ? 0 : (c.x-mylen2),
						((c.y-mylen2)<0) ? 0 : (c.y-mylen2) );
					Diff2D _roi_lr (
						((c.x-mylen2+mylen)>w) ? w : (c.x-mylen2+mylen),
						((c.y-mylen2+mylen)>h) ? h : (c.y-mylen2+mylen) );
						
					xxl_ul += (_roi_ul-roi_ul)*factor; // offset in xxl image
					xxl_lr += (_roi_lr-roi_lr)*factor;
					roi_ul = _roi_ul;
					roi_lr = _roi_lr;
				}
				

				vigra::resizeImageCatmullRomInterpolation(
				//~ vigra::resizeImageSplineInterpolation(
						srcIterRange(filtered.upperLeft()+roi_ul, filtered.upperLeft()+roi_lr), 
						destIterRange(im_xxl.upperLeft()+xxl_ul, im_xxl.lowerRight()+xxl_lr));
				//find local maxima that are above a given threshold
				// TODO: include only internal pixels to get only one maximum
				maxima_acc.setOffset(Diff2D(factor*(c.x-mylen2), factor*(c.y-mylen2)));
				vigra::localMaxima(srcIterRange(im_xxl.upperLeft()+xxl_ul+Diff2D(factor,factor), im_xxl.lowerRight()+xxl_lr-Diff2D(factor,factor)),
						destIter(im_xxl.upperLeft()+xxl_ul+Diff2D(factor,factor), maxima_acc), vigra::LocalMinmaxOptions().threshold(threshold));
		}
		
		
		
		

        if(i%10==9) {
			std::cout << i+1 << " ";   // 
			flush(std::cout);
		}
	}
	std::cout << std::endl;
	
	
}

