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
#include <vigra/multi_array.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/fftw3.hxx> 
#include <vigra/localminmax.hxx>
#include <vigra/splines.hxx> // catmullromspline, bspline
#include <vigra/transformimage.hxx>
#include <vigra/functorexpression.hxx>
#include "myresizeimage.hxx"

using namespace vigra;
using namespace vigra::functor;



// struct to keep an image coordinate
template <class VALUETYPE>
class Coord{
	public:
		typedef VALUETYPE value_type;
		Coord(const int x_,const int y_,const VALUETYPE val_) : x(x_), y(y_), val(val_) {  }
		int x;
		int y;
		VALUETYPE val;
};


// hack to push the coordinates into an array instead of marking them in 
// a target image
template <class T, class ITERATOR>
class VectorPushAccessor{
	public:
		typedef typename T::value_type value_type;
		VectorPushAccessor(std::vector<T>& arr, ITERATOR it_start) 
			: m_arr(arr), m_it_start(it_start) {		}
	

		T const & 	operator() (ITERATOR const &i) const {
			return NumericTraits<T>::zero();
		}
		//~ template<class V , class ITERATOR , class DIFFERENCE >
		//~ void 	set (V const &value, ITERATOR const &i, DIFFERENCE const &diff) const
		template<class V>
		void 	set (V const &value, ITERATOR const &i) {
			int x = i.x;
			int y = i.y-m_it_start.y;
			typename T::value_type val = *i;
			T c (x,y,val);
			m_arr.push_back(c);
		}
	
	private:
		std::vector<T>& m_arr;
		ITERATOR m_it_start;
	
};


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
template <class T, class H>
void wienerStorm(MultiArrayView<3, T>& im, Kernel2D<H>& filter, 
			std::vector<std::vector<Coord<T> > >& maxima_coords, 
			T threshold=800, int factor=8) {
	
	unsigned int stacksize = im.size(2);
	unsigned int w = im.size(0);
	unsigned int h = im.size(1);
	unsigned int w_xxl = factor*(w-1)+1;
	unsigned int h_xxl = factor*(h-1)+1;
    
	// TODO: Precondition: res must have size (factor*(w-1)+1, factor*(h-1)+1)
	// filter is now a kernel

    BasicImage<int> im_in(w,h);
    BasicImage<int> filtered(w,h);
    BasicImage<int> im_xxl(w_xxl, h_xxl);
    
    std::cout << "Finding the maximum spots in the images..." << std::endl;
    //over all images in stack
    for(unsigned int i = 0; i < stacksize; i++) {
    //~ for(int i = 0; i < 10; i++) {
		MultiArrayView <2, T> array = im.bindOuter(i); // select current image

		BasicImageView<T> input = makeBasicImageView(array);  // access data as BasicImage
		
		vigra::copyImage(srcImageRange(input), destImage(im_in));
        //fft, filter with Wiener filter in frequency domain, inverse fft, take real part
        vigra::convolveImage(srcImageRange(im_in), destImage(filtered), kernel2d(filter));
        vigra::transformImage(srcImageRange(filtered), destImage(filtered), Arg1()/Param(1024));
        //upscale filtered image with spline interpolation
		myResizeImageSplineInterpolation(srcImageRange(filtered), destImageRange(im_xxl), vigra::CatmullRomSpline<double>());
		//~ vigra::resizeImageCatmullRomInterpolation(srcImageRange(filtered), destImageRange(im_xxl));
        
        //find local maxima that are above a given threshold
        //~ maxima = 0;
		VectorPushAccessor<Coord<float>, typename BasicImage<int>::const_traverser> maxima_acc(maxima_coords[i], im_xxl.upperLeft());
		vigra::localMaxima(srcImageRange(im_xxl), destImage(im_xxl, maxima_acc), vigra::LocalMinmaxOptions().threshold(threshold));

        if(i%10==9) {
			std::cout << i+1 << " ";   // 
			flush(std::cout);
		}
	}
	std::cout << std::endl;
	
	
}

