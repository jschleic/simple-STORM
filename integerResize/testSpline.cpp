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
#include <vigra/resizeimage.hxx> // vigra version for comparison
#include <vigra/combineimages.hxx>
#include <vigra/inspectimage.hxx>
#include <vigra/splines.hxx> // catmullromspline, bspline

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
		unsigned int runs = 10;

		clock_t start, end;

		IImage res(wnew,hnew);
		
		// my integer version
		start = clock();  // measure the time; my variant
		for(int i = 0; i < runs; ++i)  {
			myResizeImageSplineInterpolation(srcImageRange(img), destImageRange(res), vigra::CatmullRomSpline<double>());
		}

		end = clock();                  // Ende der Zeitmessung
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		//~ printFPImg(res);

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
