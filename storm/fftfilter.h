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

#endif // FFTFILTER_H
