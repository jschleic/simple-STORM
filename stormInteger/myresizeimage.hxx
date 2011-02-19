/************************************************************************/
/*                                                                      */
/*    Copyright 1998-2010 by Ullrich Koethe and Joachim Schleicher      */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
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


#include <vigra/basicimage.hxx>
#include <vigra/rational.hxx>
#include <vigra/separableconvolution.hxx> // Kernel1D
#include <vigra/functorexpression.hxx> // Arg1(), Param()...
#include <vigra/transformimage.hxx>
#include <algorithm>  // max

using namespace vigra;


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



template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void myResizeLineLinear(SrcIterator i1, SrcIterator iend, SrcAccessor as, int factor,
                           DestIterator id, DestIterator idend, DestAccessor ad, int additionalBits) {
	//~ std::cout << ibeg << " " << iend << " " << istride << ",,";
	--iend, --idend;
	const int ONE_HALF = 1<<(factor-additionalBits-1);
	for(; i1!=iend; ++i1) {
		for(int j = 0; j < (1<<factor); ++j, ++id) {
			//~ std::cout << i << ":" << (as(i1)>>8) << "=>" << j << " " ;
			typename SrcAccessor::value_type o = (((1<<factor)-j)*as(i1)+j*as(i1,1));
			o += ONE_HALF;
			o >>= factor-additionalBits;
			ad.set(o, id);
		}
	}
	if(additionalBits > 0) {
		ad.set(as(iend)<<additionalBits, idend);
	} else {
		ad.set( (as(iend) + (1<<(-additionalBits-1)))>>(-additionalBits), idend);
	}
	
	//~ std::cout << std::endl;
	return;
}

// --
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void myResizeImageLinear(SrcIterator is, SrcIterator iend, SrcAccessor sa,
		DestIterator id, DestIterator idend, DestAccessor da) {

    typedef typename SrcAccessor::value_type SRCVT;
    typedef SRCVT TMPTYPE;

	// Preconditions and calculations...
    int w = iend.x - is.x;
    int h = iend.y - is.y;

    int wnew_dest = idend.x - id.x;
    int hnew_dest = idend.y - id.y;
    	
    int xfactor = (wnew_dest-1) / (w-1);
    int yfactor = (hnew_dest-1) / (h-1);
    
	int wnew = xfactor*(w-1)+1;
	int hnew = yfactor*(h-1)+1;
	
	int xlnfac = log2(xfactor); // factor has to be 2**lnfac, only powers of two supported!
	int ylnfac = log2(yfactor); // factor has to be 2**lnfac, only powers of two supported!

	vigra_precondition((wnew>=w) && (hnew>=h), "resizeImage: currently only upscaling supported");
	vigra_precondition((wnew==wnew_dest) && (hnew==hnew_dest), "resizeImage: This functions only works for integer factors");
	vigra_precondition(((1<<xlnfac)==xfactor) && ((1<<ylnfac)==yfactor), "resizeImage: This functions only works for factors beeing powers of two");

	// here we go
	BasicImage<SRCVT> imgTmp (w,hnew);

    typedef BasicImage<TMPTYPE> TmpImage;
    typedef typename TmpImage::traverser TmpImageIterator;
    typename BasicImage<TMPTYPE>::Iterator yt = imgTmp.upperLeft();

	for(int x = 0; x < w; ++x, ++is.x, ++yt.x) {  // increase height, calculate exact
        typename SrcIterator::column_iterator c1 = is.columnIterator();
        typename TmpImageIterator::column_iterator ct = yt.columnIterator();
		
		myResizeLineLinear( c1, c1 + h, sa, ylnfac,
							ct, ct + hnew, imgTmp.accessor(), ylnfac);
	}

    yt = imgTmp.upperLeft();
	
	for(int y = 0; y < hnew; ++y, ++yt.y, ++id.y) {  // increase width, round to integers again
        typename DestIterator::row_iterator rd = id.rowIterator();
        typename TmpImageIterator::row_iterator rt = yt.rowIterator();
		myResizeLineLinear(rt, rt + w, imgTmp.accessor(), xlnfac,
                                          rd, rd + wnew, da, -ylnfac);
	}
}

// --
template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor>
inline void
myResizeImageLinear(triple<SrcIterator, SrcIterator, SrcAccessor> src, 
                               triple<DestIterator, DestIterator, DestAccessor> dest)
{
    myResizeImageLinear(src.first, src.second, src.third, 
                                   dest.first, dest.second, dest.third);
}


//######################################################################

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
inline void
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
inline void
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

//######################################################################





template <class SrcIter, class SrcAcc,
          class DestIter, class DestAcc,
          class KernelArray,
          class Functor>
inline void
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
	// assuming, input is below 16bit or even just 8bit (short int or char)
	// then multiplication with 15bit kernel values stays below 31bit.
	// when using additional bits for the intermediate result, we have 
	// even less bits available for the input data

    int width_old = src_iter_end.x - src_iter.x;
    int height_old = src_iter_end.y - src_iter.y;

    int width_new = dest_iter_end.x - dest_iter.x;
    int height_new = dest_iter_end.y - dest_iter.y;
    
    int xfactor = (width_new-1) / (width_old-1);
    int yfactor = (height_new-1) / (height_old-1);
    int factor = std::max(xfactor, yfactor);

    vigra_precondition((width_old > 1) && (height_old > 1),
                 "resizeImageSplineInterpolation(): "
                 "Source image to small.\n");

    vigra_precondition((width_new > 1) && (height_new > 1),
                 "resizeImageSplineInterpolation(): "
                 "Destination image to small.\n");
             
	typedef typename SrcAccessor::value_type TmpType;
	typedef typename vigra::BasicImage<TmpType> TmpImage;
	typedef typename vigra::BasicImage<typename SrcAccessor::value_type> SrcImage;
	typedef typename vigra::BasicImage<typename DestAccessor::value_type> DestImage;
                 	
	Rational<int> xratio(width_new - 1, width_old - 1);
	Rational<int> yratio(height_new - 1, height_old - 1);
	Rational<int> offset(0);
	my_resampling_detail::MapTargetToSourceCoordinate xmapCoordinate(xratio, offset);
	my_resampling_detail::MapTargetToSourceCoordinate ymapCoordinate(yratio, offset);
	int xperiod = lcm(xratio.numerator(), xratio.denominator());
	int yperiod = lcm(yratio.numerator(), yratio.denominator());

	int kernelbits = 11; // todo: what is appropriate bit width
	int additionalBits = 2*log2(factor)-1; // todo: what is an appropriate accuracy for intermediate result?
	ArrayVector<Kernel1D<int> > ykernels(yperiod);
	ArrayVector<Kernel1D<int> > xkernels(xperiod);
	
	// kernels in y direction
	myCreateResamplingKernels(spline, ymapCoordinate, ykernels, 1<<kernelbits); // TODO: BSpline uses double as default
	// and in x direction
	myCreateResamplingKernels(spline, xmapCoordinate, xkernels, 1<<kernelbits); // TODO: BSpline uses double as default

	TmpImage tmp(width_old, height_new);

	// resize in y-direction
	typename TmpImage::Iterator y_tmp = tmp.upperLeft();
	for(int x = 0; x < width_old; ++x, ++src_iter.x, ++y_tmp.x) {
		typename SrcImage::ConstIterator::column_iterator rit = src_iter.columnIterator();
		typename TmpImage::Iterator::column_iterator dit = y_tmp.columnIterator();
		myResamplingConvolveLine(rit,rit+height_old,
			src_acc,
			dit, dit+height_new,
			tmp.accessor(),
			ykernels,
			ymapCoordinate);
	}
	// appropriate rounding
	const int one_half_tmp = 1<<(kernelbits-additionalBits-1);
	transformImage(srcImageRange(tmp), destImage(tmp), 
			(vigra::functor::Arg1()+vigra::functor::Param(one_half_tmp))/vigra::functor::Param(1<<(kernelbits-additionalBits)));  // transform back to the original accuracy

	// resize in x-direction
	y_tmp = tmp.upperLeft();
	typename DestImage::Iterator res_iter = dest_iter;
	for(int y = 0; y < height_new; ++y, ++y_tmp.y, ++res_iter.y) {
		typename TmpImage::Iterator::row_iterator rit = y_tmp.rowIterator();
		typename DestImage::Iterator::row_iterator dit = res_iter.rowIterator();
		myResamplingConvolveLine(rit,rit+width_old,
			tmp.accessor(),
			dit, dit+width_new,
			dest_acc,
			xkernels,
			xmapCoordinate);
	}
	const int one_half = 1<<(kernelbits+additionalBits-1);
	vigra::transformImage(dest_iter, dest_iter_end, dest_acc, dest_iter, dest_acc, 
			(vigra::functor::Arg1()+vigra::functor::Param(one_half))/vigra::functor::Param(1<<(kernelbits+additionalBits)));  // transform back to the original accuracy
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

