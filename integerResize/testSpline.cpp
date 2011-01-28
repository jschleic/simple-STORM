/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*         Copyright 2010 by Joachim Schleicher and Ullrich Koethe      */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/************************************************************************/


#include <iostream>
#include <stdio.h> // for  printf
#include <vector>

#include "myresizeimage.hxx"

#include <vigra/impex.hxx>
//~ #include <vigra/resizeimage.hxx> // TODO! alles selber oder sehr vorsichtig!
#include <vigra/combineimages.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/separableconvolution.hxx> // Kernel1D
#include <vigra/splines.hxx> // catmullromspline, bspline
#include <vigra/functorexpression.hxx> // Arg1(), Param()...

#include <time.h>
#ifdef CLK_TCK
#else
#define CLK_TCK 1000000.
#endif

using namespace std;
using namespace vigra;
using namespace vigra::functor;

template <class Image>
void printFPImg(Image img) {
	for( int i = 0; i < img.height(); i++) {
		for(int j = 0; j < img.width(); j++) {
			std::cout << (double)img(j,i)/256. << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

namespace std {

template <class T>
ostream & operator<<(ostream & s, vigra::Kernel1D<T>& k)
{
	for(int i = k.left(); i <= k.right(); ++i) {
		if(i==0) {
			s << "*" << k[i] << "* ";
		} else {
			s << k[i] << " ";
		}
	}
	s << std::endl;
    return s;
}

} // namespace std

//--------- helper functions
namespace my_resampling_detail {
	
struct MapTargetToSourceCoordinate
{
    MapTargetToSourceCoordinate(Rational<int> const & samplingRatio,
                                Rational<int> const & offset)
    : a(samplingRatio.denominator()*offset.denominator()),
      b(samplingRatio.numerator()*offset.numerator()),
      c(samplingRatio.numerator()*offset.denominator())
    {}

//        the following functions are more efficient realizations of:
//             rational_cast<T>(i / samplingRatio + offset);
//        we need efficiency because this may be called in the inner loop

    int operator()(int i) const
    {
        return (i * a + b) / c;
    }

    double toDouble(int i) const
    {
        return double(i * a + b) / c;
    }

    Rational<int> toRational(int i) const
    {
        return Rational<int>(i * a + b, c);
    }
    
    bool isExpand2() const
    {
        return a == 1 && b == 0 && c == 2;
    }
    
    bool isReduce2() const
    {
        return a == 2 && b == 0 && c == 1;
    }

    int a, b, c;
};


} // resampling_detail

template <class SrcIter, class SrcAcc,
          class DestIter, class DestAcc,
          class KernelArray>
void
myResamplingExpandLine2(SrcIter s, SrcIter send, SrcAcc src,
                       DestIter d, DestIter dend, DestAcc dest,
                       KernelArray const & kernels)
{
    typedef typename KernelArray::value_type Kernel;
    typedef typename KernelArray::const_reference KernelRef;
    typedef typename Kernel::const_iterator KernelIter;

    typedef typename
        PromoteTraits<typename SrcAcc::value_type, typename Kernel::value_type>::Promote
        TmpType;

    int wo = send - s;
    int wn = dend - d;
    int wo2 = 2*wo - 2;
    
    int ileft = std::max(kernels[0].right(), kernels[1].right());
    int iright = wo + std::min(kernels[0].left(), kernels[1].left()) - 1;
    for(int i = 0; i < wn; ++i, ++d)
    {
        int is = i / 2;
        KernelRef kernel = kernels[i & 1];
        KernelIter k = kernel.center() + kernel.right();
        TmpType sum = NumericTraits<TmpType>::zero();        
        if(is < ileft)
        {
            for(int m=is-kernel.right(); m <= is-kernel.left(); ++m, --k)
            {
                int mm = (m < 0) 
                        ? -m 
                        : m;
                sum += *k * src(s, mm);
            }        
        }
        else if(is > iright)
        {
            for(int m=is-kernel.right(); m <= is-kernel.left(); ++m, --k)
            {
                int mm =  (m >= wo) 
                            ? wo2 - m
                            : m;
                sum += *k * src(s, mm);
            }        
        }
        else
        {
            SrcIter ss = s + is - kernel.right();
            for(int m = 0; m < kernel.size(); ++m, --k, ++ss)
            {
                sum += *k * src(ss);
            }        
        }
        dest.set(sum, d);
    }
}

template <class SrcIter, class SrcAcc,
          class DestIter, class DestAcc,
          class KernelArray>
void
myResamplingReduceLine2(SrcIter s, SrcIter send, SrcAcc src,
                       DestIter d, DestIter dend, DestAcc dest,
                       KernelArray const & kernels)
{
    typedef typename KernelArray::value_type Kernel;
    typedef typename KernelArray::const_reference KernelRef;
    typedef typename Kernel::const_iterator KernelIter;

    KernelRef kernel = kernels[0];
    KernelIter kbegin = kernel.center() + kernel.right();

    typedef typename
        PromoteTraits<typename SrcAcc::value_type, typename Kernel::value_type>::Promote
        TmpType;

    int wo = send - s;
    int wn = dend - d;
    int wo2 = 2*wo - 2;
    
    int ileft = kernel.right();
    int iright = wo + kernel.left() - 1;
    for(int i = 0; i < wn; ++i, ++d)
    {
        int is = 2 * i;
        KernelIter k = kbegin;
        TmpType sum = NumericTraits<TmpType>::zero();        
        if(is < ileft)
        {
            for(int m=is-kernel.right(); m <= is-kernel.left(); ++m, --k)
            {
                int mm = (m < 0) 
                        ? -m 
                        : m;
                sum += *k * src(s, mm);
            }        
        }
        else if(is > iright)
        {
            for(int m=is-kernel.right(); m <= is-kernel.left(); ++m, --k)
            {
                int mm =  (m >= wo) 
                            ? wo2 - m
                            : m;
                sum += *k * src(s, mm);
            }        
        }
        else
        {
            SrcIter ss = s + is - kernel.right();
            for(int m = 0; m < kernel.size(); ++m, --k, ++ss)
            {
                sum += *k * src(ss);
            }        
        }
        dest.set(sum, d);
    }
}


//------------------------------------------
template <class SrcIter, class SrcAcc,
          class DestIter, class DestAcc,
          class KernelArray,
          class Functor>
void
myResamplingConvolveLine(SrcIter s, SrcIter send, SrcAcc src,
                       DestIter d, DestIter dend, DestAcc dest,
                       KernelArray const & kernels,
                       Functor mapTargetToSourceCoordinate)
{
    if(mapTargetToSourceCoordinate.isExpand2())
    {
        myResamplingExpandLine2(s, send, src, d, dend, dest, kernels);
        return;
    }
    if(mapTargetToSourceCoordinate.isReduce2())
    {
        myResamplingReduceLine2(s, send, src, d, dend, dest, kernels);
        return;
    }
    
    typedef typename SrcAcc::value_type TmpType; // TODO
    typedef typename KernelArray::value_type Kernel;
    typedef typename Kernel::const_iterator KernelIter;

    int wo = send - s;
    int wn = dend - d;
    int wo2 = 2*wo - 2;

    int i;
    typename KernelArray::const_iterator kernel = kernels.begin();
    for(i=0; i<wn; ++i, ++d, ++kernel)
    {
        // use the kernels periodically
        if(kernel == kernels.end())
            kernel = kernels.begin();

        // calculate current target point into source location
        int is = mapTargetToSourceCoordinate(i);

        TmpType sum = NumericTraits<TmpType>::zero();

        int lbound = is - kernel->right(),
            hbound = is - kernel->left();

        KernelIter k = kernel->center() + kernel->right();
        if(lbound < 0 || hbound >= wo)
        {
            vigra_precondition(-lbound < wo && wo2 - hbound >= 0,
                "resamplingConvolveLine(): kernel or offset larger than image.");
            for(int m=lbound; m <= hbound; ++m, --k)
            {
                int mm = (m < 0) ?
                            -m :
                            (m >= wo) ?
                                wo2 - m :
                                m;
                sum = TmpType(sum + *k * src(s, mm));
            }
        }
        else
        {
            SrcIter ss = s + lbound;
            SrcIter ssend = s + hbound;

            for(; ss <= ssend; ++ss, --k)
            {
                sum = TmpType(sum + *k * src(ss));
            }
        }

        dest.set(sum, d);
    }
}

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class SPLINE>
void
myResizeImageSplineInterpolation(
    SrcIterator src_iter, SrcIterator src_iter_end, SrcAccessor src_acc,
    DestIterator dest_iter, DestIterator dest_iter_end, DestAccessor dest_acc,
    SPLINE const & spline)
{
	// assuming, input is below 16bit (short int)

    int width_old = src_iter_end.x - src_iter.x;
    int height_old = src_iter_end.y - src_iter.y;

    int width_new = dest_iter_end.x - dest_iter.x;
    int height_new = dest_iter_end.y - dest_iter.y;
    
    int xfactor = (width_new-1) / (width_old-1);
    int yfactor = (height_new-1) / (height_old-1);
    int factor = max(xfactor, yfactor);

    vigra_precondition((width_old > 1) && (height_old > 1),
                 "resizeImageSplineInterpolation(): "
                 "Source image to small.\n");

    vigra_precondition((width_new > 1) && (height_new > 1),
                 "resizeImageSplineInterpolation(): "
                 "Destination image to small.\n");
             
	typedef typename SrcAccessor::value_type TmpType;
	typedef typename vigra::BasicImage<TmpType> TmpImage;
                 	
	Rational<int> xratio(width_new - 1, width_old - 1);
	Rational<int> yratio(height_new - 1, height_old - 1);
	Rational<int> offset(0);
	my_resampling_detail::MapTargetToSourceCoordinate xmapCoordinate(xratio, offset);
	my_resampling_detail::MapTargetToSourceCoordinate ymapCoordinate(yratio, offset);
	int xperiod = lcm(xratio.numerator(), xratio.denominator());
	int yperiod = lcm(yratio.numerator(), yratio.denominator());

	int kernelbits = 15; // todo: what is appropriate bit width
	int additionalBits = log2(factor); // todo: what is an appropriate accuracy for intermediate result?
	ArrayVector<Kernel1D<int> > ykernels(yperiod);
	ArrayVector<Kernel1D<int> > xkernels(xperiod);
	
	// kernels in y direction
	myCreateResamplingKernels(spline, ymapCoordinate, ykernels, 1<<kernelbits); // TODO: BSpline uses double as default
	// and in x direction
	myCreateResamplingKernels(spline, xmapCoordinate, xkernels, 1<<kernelbits); // TODO: BSpline uses double as default

	TmpImage tmp(width_old, height_new);

	// resize in y-direction
	IImage::Iterator y_tmp = tmp.upperLeft();
	for(int x = 0; x < width_old; ++x, ++src_iter.x, ++y_tmp.x) {
		IImage::ConstIterator::column_iterator rit = src_iter.columnIterator();
		IImage::Iterator::column_iterator dit = y_tmp.columnIterator();
		myResamplingConvolveLine(rit,rit+height_old,
			src_acc,
			dit, dit+height_new,
			tmp.accessor(),
			ykernels,
			ymapCoordinate);
	}
	transformImage(srcImageRange(tmp), destImage(tmp), Arg1()/Param(1<<(kernelbits-additionalBits)));  // transform back to the original accuracy

	// resize in x-direction
	y_tmp = tmp.upperLeft();
	IImage::Iterator res_iter = dest_iter;
	for(int y = 0; y < height_new; ++y, ++y_tmp.y, ++res_iter.y) {
		IImage::Iterator::row_iterator rit = y_tmp.rowIterator();
		IImage::Iterator::row_iterator dit = res_iter.rowIterator();
		myResamplingConvolveLine(rit,rit+width_old,
			tmp.accessor(),
			dit, dit+width_new,
			dest_acc,
			xkernels,
			xmapCoordinate);
	}
	vigra::transformImage(dest_iter, dest_iter_end, dest_acc, dest_iter, dest_acc, Arg1()/Param(1<<(kernelbits+additionalBits)));  // transform back to the original accuracy
}


template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class SPLINE>
inline
void
myResizeImageSplineInterpolation(triple<SrcIterator, SrcIterator, SrcAccessor> src,
                      triple<DestIterator, DestIterator, DestAccessor> dest,
                      SPLINE const & spline)
{
    myResizeImageSplineInterpolation(src.first, src.second, src.third,
                                   dest.first, dest.second, dest.third, spline);
}


//--------- create Kernels
// TODO: Refactor for Rational and Fixed Point values
template <class Kernel, class MapCoordinate, class KernelArray>
void
myCreateResamplingKernels(Kernel const & kernel,
             MapCoordinate const & mapCoordinate, KernelArray & kernels, int upscaleFactor)
{
	// assuming offset==0 here
    for(unsigned int idest = 0; idest < kernels.size(); ++idest)
    {
        int isrc = mapCoordinate(idest);
        double idsrc = mapCoordinate.toDouble(idest);
        double offset = idsrc - isrc;
        double radius = kernel.radius();
        int left = int(ceil(-radius - offset));
        int right = int(floor(radius - offset));
        kernels[idest].initExplicitly(left, right);

        double x = left + offset;
        for(int i = left; i <= right; ++i, ++x)
            kernels[idest][i] = kernel(x)*upscaleFactor;
        kernels[idest].normalize(upscaleFactor, kernel.derivativeOrder(), offset);
        //~ std::cout << "idest " << idest << ": " << kernels[idest] << std::endl;
    }
}



int main(int argc, char** argv) {
	try {
		// all Integer-numbers are shifted by 8
		const int w = 128;
		const int h = 128;
		const int factor=16;
		const int wnew = factor*(w-1)+1;
		const int hnew = factor*(h-1)+1;
		IImage img(w,h);

		//~ img(1,0) = 10<<8;
		//~ printFPImg(img);

		ImageImportInfo info(argv[1]);
		importImage(info, destImage(img));
		unsigned int runs = 1;

		clock_t start, end;

		IImage res(wnew,hnew);
		
		// my integer version
		myResizeImageSplineInterpolation(srcImageRange(img), destImageRange(res), vigra::CatmullRomSpline<double>());

		end = clock();                  // Ende der Zeitmessung
		//~ printFPImg(res);
		vigra::exportImage(srcImageRange(res),
                      vigra::ImageExportInfo(argv[2]));
	exit(0);

		start = clock();  // measure the time; VIGRA-standard
		IImage bb(wnew,hnew);
		for(int i = 0; i < runs; i++) {
			//~ vigra:: resizeImageSplineInterpolation(srcImageRange(img), 
				//~ destImageRange(bb), vigra::CatmullRomSpline<double>());
		}
		// end
		end = clock();                  // Ende der Zeitmessung
		std::cout << runs << " runs." << std::endl;
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		

		FImage diff(wnew, hnew);
		vigra::combineTwoImages(srcImageRange(bb), srcImage(res), destImage(diff), Arg1()-Arg2());
		vigra::FindMinMax<vigra::FImage::PixelType> minmax;

		vigra::inspectImage(srcImageRange(diff), minmax);

		std::cout << "Diff Min: " << minmax.min << " Max: " << minmax.max << std::endl;
		

		vigra::exportImage(srcImageRange(res),
                      vigra::ImageExportInfo(argv[2]));
	
	}
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
	return 0;
}
