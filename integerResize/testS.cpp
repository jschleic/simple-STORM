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
#include <vigra/resizeimage.hxx>
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


int main(int argc, char** argv) {
	try {
		// all Integer-numbers are shifted by 8
		const int w = 128;
		const int h = 128;
		const int factor=16;
		const int wnew = factor*(w-1)+1;
		const int hnew = factor*(h-1)+1;
		IImage img(w,h);
		img = 0;
		img(1,1) = 50<<8;
		img(1,2) = 10<<8;
		img(0,2) = 1<<8;
		img(0,3) = 7<<8;

		ImageImportInfo info(argv[1]);
		importImage(info, destImage(img));
		unsigned int runs = 100;

		clock_t start, end;

		IImage res(wnew,hnew);
		start = clock();  // measure the time; my variant
		for(int i = 0; i < runs; i++) {
			myResizeImageLinear(srcImageRange(img), destImageRange(res));
		}
		end = clock();                  // Ende der Zeitmessung
		std::cout << runs << " runs." << std::endl;
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);


		start = clock();  // measure the time; VIGRA-standard
		IImage bb(wnew,hnew);
		for(int i = 0; i < runs; i++) {
			vigra:: resizeImageLinearInterpolation(srcImageRange(img), 
				destImageRange(bb));
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
