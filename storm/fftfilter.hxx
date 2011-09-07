/*
 * Encapsulate filtering in fourier domain 
 * using fftw / fftwf
 * re-using plans and
 * thus making applyFourierFilter thread-safe
 */

// first version for filtering an FImage only
typedef float T;
class FFTFilter {
public:
	FFTFilter(const int w, const int h, vigra::BasicImageView<T> & im);
	~FFTFilter();

	void applyFourierFilter(vigra::BasicImageView<T> & im, vigra::BasicImage<T> & filter, vigra::BasicImageView<T> & result);

private:
	fftwf_plan forwardPlan;
	fftwf_plan backwardPlan;
	vigra::BasicImage<FFTWComplex<T> > complexImg;
};

// constructor generates a forward and a backward plan. 
// NOT THREAD-SAFE!
FFTFilter::FFTFilter(const int w, const int h, vigra::BasicImageView<T> & im) {
	complexImg.resize(w/2+1,h);
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

// this function should be thread-safe.
void FFTFilter::applyFourierFilter (vigra::BasicImageView<T> & im, vigra::BasicImage<T> & filter, vigra::BasicImageView<T> & result) {
	// test for right memory layout (fftw expects a 2*width*height floats array)
    if (&(*(im.upperLeft() + Diff2D(im.width(), 0))) != &(*(im.upperLeft() + Diff2D(0, 1)))) {
		std::cout << "wrong memory layout of input data" << std::endl;
		return;
	}

	fftwf_execute_dft_r2c(
          forwardPlan,
          (T *)im.begin(), (fftwf_complex *)complexImg.begin());
    // convolve in freq. domain (in complexResultImg)
    combineTwoImages(srcImageRange(complexImg), srcImage(filter),
                     destImage(complexImg), std::multiplies<FFTWComplex<T> >());
	fftwf_execute_dft_c2r(
			backwardPlan,
			(fftwf_complex *)complexImg.begin(), (T *)result.begin());
	float normFactor = 1. / (im.width()*im.height());
	transformImage(srcImageRange(result), destImage(result), 
			vigra::functor::Arg1()*vigra::functor::Param(normFactor));
}
