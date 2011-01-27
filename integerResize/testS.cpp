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

inline void myResizeLineImage(const int* in, int factor, int ibeg, int iend, int istride, int* out, int obeg, int oend, int ostride, int additionalBits=0) {
	//~ std::cout << ibeg << " " << iend << " " << istride << ",,";
	int o = obeg;
	const int ONE_HALF = 1<<(factor-additionalBits-1);
	for(int i = ibeg; i<iend; i+=istride) {
		for(int j = 0; j < (1<<factor); ++j, o+=ostride) {
			//~ std::cout << i << ":" << (in[i]>>8) << "=>" << o << " " ;
			out[o] = (((1<<factor)-j)*in[i]+j*in[i+istride]);
			out[o] += ONE_HALF;
			out[o] >>= factor-additionalBits;
		}
	}
	if(additionalBits > 0) {
		out[oend] = in[iend]<<additionalBits;
	} else {
		out[oend] = in[iend] + (1<<(-additionalBits-1)); // one half
		out[oend] >>= (-additionalBits);
	}
	
	//~ std::cout << std::endl;
	return;
}

inline void myResizeImage(const int* in, const int w, const int h, const int factor, int* out) {

		const int wnew = factor*(w-1)+1;
		const int hnew = factor*(h-1)+1;
		int* imgTmpData = new int[w*hnew];
		int lnfac = log2(factor); // factor has to be 2**lnfac, nur Zweierpotenzen!

		for(int x = 0; x < w; ++x) {  // increase height, calculate exact
			myResizeLineImage(in, lnfac, x, w*(h-1)+x, w, imgTmpData, x, w*(hnew-1)+x, w, lnfac);
		}
		
		for(int y = 0; y < hnew; ++y) {  // increase width, round to integers again
			myResizeLineImage(imgTmpData, lnfac, y*w, (y+1)*w-1, 1, out, y*wnew, (y+1)*wnew-1, 1, -lnfac);
		}
		delete [] imgTmpData;
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
		//~ printFPImg(img);
		int* imgGrData = new int[wnew*hnew];

		
		//~ myResizeImage(img.data(), w, h, factor, imgGrData);

		// Ausgabe
		//~ IImage imgTmp(w,hnew,imgTmpData);
		//~ printFPImg(imgTmp);
		//~ IImage imgGr(wnew,hnew,imgGrData);
		//~ printFPImg(imgGr);

		ImageImportInfo info(argv[1]);
		importImage(info, destImage(img));
		unsigned int runs = 100;

		clock_t start, end;

		start = clock();  // measure the time; my variant
		for(int i = 0; i < runs; i++) {
			myResizeImage(img.data(), w, h, factor, imgGrData);
		}
		end = clock();                  // Ende der Zeitmessung
		std::cout << runs << " runs." << std::endl;
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);


		start = clock();  // measure the time; VIGRA-standard
		IImage bb(wnew,hnew,imgGrData);
		for(int i = 0; i < runs; i++) {
			vigra:: resizeImageLinearInterpolation(srcImageRange(img), 
				destImageRange(bb));
		}
		// end
		end = clock();                  // Ende der Zeitmessung
		std::cout << runs << " runs." << std::endl;
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		
		IImage res(wnew,hnew,imgGrData);

		FImage diff(wnew, hnew);
		vigra::combineTwoImages(srcImageRange(bb), srcImage(res), destImage(diff), Arg1()-Arg2());
		vigra::FindMinMax<vigra::FImage::PixelType> minmax;

		vigra::inspectImage(srcImageRange(diff), minmax);

		std::cout << "Diff Min: " << minmax.min << " Max: " << minmax.max << std::endl;
		

		vigra::exportImage(srcImageRange(res),
                      vigra::ImageExportInfo(argv[2]));
	
		// free allocated memory
		delete [] imgGrData;
	}
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
	return 0;
}
