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

/*
 * Encapsulate filtering in fourier domain 
 * using fftw / fftwf
 * re-using plans and
 * thus making applyFourierFilter thread-safe
 * 
 * Essentially this is a rewritten thread-safe version of 
 * vigra::applyFourierFilter for real-valued floating point images.
 */

// first version for filtering an FImage only
typedef float T; // when setting this to double all fftw-calls have to be replaced: sed s/fftwf_/fftw_/
class FFTFilter {
public:
    FFTFilter(const vigra::BasicImageView<T> & im);
    ~FFTFilter();

    void applyFourierFilter(vigra::BasicImageView<T> & im, 
                        const vigra::BasicImage<T> & filter, 
                        vigra::BasicImageView<T> & result) const;

private:
    fftwf_plan forwardPlan;
    fftwf_plan backwardPlan;
    int w,h;
    T normFactor;
};

// constructor generates a forward and a backward plan. 
// NOT THREAD-SAFE!
FFTFilter::FFTFilter(const vigra::BasicImageView<T> & im) {
    w=im.width();
    h=im.height();
    normFactor = 1. / (w*h);
    vigra::BasicImage<vigra::FFTWComplex<T> > complexImg(w/2+1,h);
    vigra::FImage resultImg(w,h);
    forwardPlan = fftwf_plan_dft_r2c_2d(h, w, (T *)im.begin(),
                           (fftwf_complex *)complexImg.begin(),
                           FFTW_ESTIMATE );
    fftwf_execute(forwardPlan);
    backwardPlan = fftwf_plan_dft_c2r_2d(h, w, (fftwf_complex *) complexImg.begin(),
                            (T *) resultImg.begin(), 
                            FFTW_ESTIMATE);
}

FFTFilter::~FFTFilter() {
    fftwf_destroy_plan(forwardPlan);
    fftwf_destroy_plan(backwardPlan);
}

// this function is thread-safe (tested with OpenMP).
void FFTFilter::applyFourierFilter (vigra::BasicImageView<T> & im, const vigra::BasicImage<T> & filter, vigra::BasicImageView<T> & result) const {
    // test for right memory layout (fftw expects a 2*width*height floats array)
    if (&(*(im.upperLeft() + vigra::Diff2D(w, 0))) != &(*(im.upperLeft() + vigra::Diff2D(0, 1)))) {
        std::cerr << "wrong memory layout of input data" << std::endl;
        return;
    }
    vigra::BasicImage<vigra::FFTWComplex<T> > complexImg(w/2+1,h);
    fftwf_execute_dft_r2c(
          forwardPlan,
          (T *)im.begin(), (fftwf_complex *)complexImg.begin());
    // convolve in freq. domain (in complexImg), only the left half of filter is used due to symmetry
    combineTwoImages(srcImageRange(complexImg), srcImage(filter),
                     destImage(complexImg), std::multiplies<vigra::FFTWComplex<T> >());
    fftwf_execute_dft_c2r(
            backwardPlan,
            (fftwf_complex *)complexImg.begin(), (T *)result.begin());
    transformImage(srcImageRange(result), destImage(result), 
            vigra::functor::Arg1()*vigra::functor::Param(normFactor));
}

#endif // FFTFILTER_H
