/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*         Copyright 2011 by Joachim Schleicher                         */
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
/************************************************************************/

#ifndef FFTFILTER_H
#define FFTFILTER_H

#include <vigra/basicimage.hxx>
#include <vigra/basicimageview.hxx>
#include <vigra/fftw3.hxx>

/*
 * Encapsulate filtering in fourier domain 
 * using fftw / fftwf
 * re-using plans and
 * thus making applyFourierFilter thread-safe
 * 
 * Concerning thread-safety in fftw see:
 * http://www.fftw.org/fftw3_doc/Thread-safety.html
 * 
 * Essentially this is a rewritten thread-safe version of 
 * vigra::applyFourierFilter for real-valued floating point images.
 * See there for usage examples.
 */

using namespace vigra; // for now

// first version for filtering an float-Images only
// when using this for double all fftw-calls have to be replaced: sed s/fftwf_/fftw_/
template <class T>
class FFTFilter {
};

template <> // currently only float supported!!
class FFTFilter<float> {
public:
    typedef float value_type;

    template <class SrcImageIterator, class SrcAccessor>
    FFTFilter(SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa);
    template <class SrcImageIterator, class SrcAccessor>
    FFTFilter(triple<SrcImageIterator, SrcImageIterator, SrcAccessor>);
    ~FFTFilter() {    
        fftwf_destroy_plan(forwardPlan);
        fftwf_destroy_plan(backwardPlan); 
    }

    template <class SrcImageIterator, class SrcAccessor,
          class FilterImageIterator, class FilterAccessor,
          class DestImageIterator, class DestAccessor>
    void applyFourierFilter(SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa,
                            FilterImageIterator filterUpperLeft, FilterAccessor fa,
                            DestImageIterator destUpperLeft, DestAccessor da) const;
    template <class SrcImageIterator, class SrcAccessor,
          class FilterImageIterator, class FilterAccessor,
          class DestImageIterator, class DestAccessor>
    void applyFourierFilter(triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                        pair<FilterImageIterator, FilterAccessor> filter,
                        pair<DestImageIterator, DestAccessor> dest) const;

private:
    template <class SrcImageIterator, class SrcAccessor>
    void init(SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa);
    fftwf_plan forwardPlan;
    fftwf_plan backwardPlan;
    int w,h;
    value_type normFactor;
};


// constructor generates a forward and a backward plan. 
// NOT THREAD-SAFE!
template <class SrcImageIterator, class SrcAccessor>
FFTFilter<float>::FFTFilter(SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa) {
    init(srcUpperLeft, srcLowerRight, sa);
}

template <class SrcImageIterator, class SrcAccessor>
void FFTFilter<float>::init(SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa) {
    w= srcLowerRight.x - srcUpperLeft.x;
    h= srcLowerRight.y - srcUpperLeft.y;
    normFactor = 1. / (w*h);
    vigra::BasicImage<vigra::FFTWComplex<value_type> > complexImg(w/2+1,h);
    vigra::BasicImage<value_type> resultImg(w,h);
    forwardPlan = fftwf_plan_dft_r2c_2d(h, w, (value_type *)&(*srcUpperLeft),
                           (fftwf_complex *)complexImg.begin(),
                           FFTW_ESTIMATE );
    fftwf_execute(forwardPlan);
    backwardPlan = fftwf_plan_dft_c2r_2d(h, w, (fftwf_complex *) complexImg.begin(),
                            (value_type *) resultImg.begin(), 
                            FFTW_ESTIMATE);
}

template <class SrcImageIterator, class SrcAccessor>
FFTFilter<float>::FFTFilter(triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src) {
    init(src.first, src.second, src.third);
}


// this function is thread-safe (tested with OpenMP).
template <class SrcImageIterator, class SrcAccessor,
          class FilterImageIterator, class FilterAccessor,
          class DestImageIterator, class DestAccessor>
void FFTFilter<float>::applyFourierFilter (SrcImageIterator srcUpperLeft,
                            SrcImageIterator srcLowerRight, SrcAccessor sa,
                            FilterImageIterator filterUpperLeft, FilterAccessor fa,
                            DestImageIterator destUpperLeft, DestAccessor da) const {
    // test for correct memory layout (fftw expects a 2*width*height floats array)
    vigra_precondition (&(*(srcUpperLeft + Diff2D(w, 0))) == &(*(srcUpperLeft + Diff2D(0, 1))),
        "wrong memory layout of input data");

    vigra::BasicImage<vigra::FFTWComplex<value_type> > complexImg(w/2+1,h);
    fftwf_execute_dft_r2c(
          forwardPlan,
          (value_type *)&(*srcUpperLeft), (fftwf_complex *)complexImg.begin());
    // convolve in freq. domain (in complexImg), only the left half of filter is used due to symmetry
    combineTwoImages(srcImageRange(complexImg), srcIter(filterUpperLeft,fa),
                     destImage(complexImg), std::multiplies<vigra::FFTWComplex<value_type> >());
    fftwf_execute_dft_c2r(
            backwardPlan,
            (fftwf_complex *)complexImg.begin(), (value_type *)&(*destUpperLeft));
    transformImage(srcIterRange(destUpperLeft, destUpperLeft + Diff2D(w, h),da), 
            destIter(destUpperLeft,da), 
            vigra::functor::Arg1()*vigra::functor::Param(normFactor));
}

template <class SrcImageIterator, class SrcAccessor,
          class FilterImageIterator, class FilterAccessor,
          class DestImageIterator, class DestAccessor>
inline
void FFTFilter<float>::applyFourierFilter(triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                        pair<FilterImageIterator, FilterAccessor> filter,
                        pair<DestImageIterator, DestAccessor> dest) const
{
    applyFourierFilter(src.first, src.second, src.third,
                       filter.first, filter.second,
                       dest.first, dest.second);
}

#endif // FFTFILTER_H
