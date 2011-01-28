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
#include <vigra/resizeimage.hxx> // TODO! alles selber oder sehr vorsichtig!
#include <vigra/combineimages.hxx>
#include <vigra/inspectimage.hxx>

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
        std::cout << "idest " << idest << ": " << kernels[idest] << std::endl;
    }
}



int main(int argc, char** argv) {
	try {
		// all Integer-numbers are shifted by 8
		const int w = 6;
		const int h = 1;
		const int factor=8;
		const int wnew = factor*(w-1)+1;
		const int hnew = factor*(h-1)+1;
		IImage img(w,h);

		img(1,0) = 10<<8;
		printFPImg(img);

		//~ ImageImportInfo info(argv[1]);
		//~ importImage(info, destImage(img));
		unsigned int runs = 1;

		clock_t start, end;

		IImage res(wnew,hnew);
		
		Rational<int> samplingRatio(wnew - 1, w - 1);
		Rational<int> offset(0);
    	int period = lcm(samplingRatio.numerator(), samplingRatio.denominator());

		resampling_detail::MapTargetToSourceCoordinate mapCoordinate(samplingRatio, offset);

		int additionalBits = 10; // todo: what is an appropriate intermediate accuracy?
		ArrayVector<Kernel1D<int> > kernels(period);
		myCreateResamplingKernels(CatmullRomSpline<double>(), mapCoordinate, kernels, 1<<additionalBits); // TODO: BSpline uses double as default
		start = clock();  // measure the time; my variant
        IImage::Iterator::row_iterator rit = img.upperLeft().rowIterator();
        IImage::Iterator::row_iterator dit = res.upperLeft().rowIterator();
		for(int i = 0; i < runs; i++) {
			myResamplingConvolveLine(rit,rit+w,
				img.accessor(),
				dit, dit+wnew,
				res.accessor(),
				kernels,
				mapCoordinate);
		}
		transformImage(srcImageRange(res), destImage(res), Arg1()/Param(1<<additionalBits));  // transform back to the original accuracy
		end = clock();                  // Ende der Zeitmessung
		std::cout << runs << " runs." << std::endl;
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		printFPImg(res);
	exit(0);

		start = clock();  // measure the time; VIGRA-standard
		IImage bb(wnew,hnew);
		for(int i = 0; i < runs; i++) {
			vigra:: resizeImageSplineInterpolation(srcImageRange(img), 
				destImageRange(bb), vigra::CatmullRomSpline<double>());
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
