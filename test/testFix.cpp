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
#include <stdio.h>
#include <vector>
#include "myfixedpoint.hxx"
#include <vigra/tinyvector.hxx>
#include <vigra/impex.hxx>
#include <vigra/localminmax.hxx>
#include "myresizeimage.hxx"

#include <time.h>
#ifdef CLK_TCK
#else
#define CLK_TCK 1000000.
#endif

using namespace std;
using namespace vigra;

template <class Image>
void printFPImg(Image img) {
	for( int i = 0; i < img.width(); i++) {
		for(int j = 0; j < img.height(); j++) {
			std::cout << fixed_point_cast<double>(img(j,i)) << " ";
		}
		std::cout << std::endl;
	}
}

template <class VALUETYPE>
class FixedPointConstAccessor
{
  public:
    typedef VALUETYPE value_type;

        /** read the current data item
        */
    template <class ITERATOR>
    VALUETYPE const operator()(ITERATOR const & i) const
        { return FixedPointCast<VALUETYPE>::cast(*i); }

        /** read the data item at an offset (can be 1D or 2D or higher order difference).
        */
    template <class ITERATOR, class DIFFERENCE>
    VALUETYPE const operator()(ITERATOR const & i, DIFFERENCE const & diff) const
    {
        return FixedPointCast<VALUETYPE>::cast(i[diff]);
    }
};


int main(int argc, char** argv) {
	try {
		typedef FixedPoint<14,2> fpIn;
		typedef FixedPoint<16,4> fpOut;
		
/// simple test example
		vigra::BasicImage<fpIn> img(4,4);
		vigra::BasicImage<fpOut> imgGr(13,13);
		vigra::DImage res;
		img = fpIn(0);
		img(1,2) = fpIn(10);
		printFPImg(img);
		
		// TODO: Die Spline-Interpolation brauche ich!
		vigra:: resizeImageLinearInterpolation(srcImageRange(img), 
			destImageRange(imgGr));
		printFPImg(imgGr);
		res.resize(13,13);
		copyImage(srcImageRange(imgGr,FixedPointConstAccessor<int>()), destImage(res));


/// real world example
		if(argc > 2) {
			ImageImportInfo info(argv[1]);
			int w = info.width();
			int h = info.height();
			FImage img(w, h);
			BasicImage<fpIn> imgfp(w, h);
			importImage(info, destImage(img));
			copyImage(srcImageRange(img), destImage(imgfp));
			unsigned int factor=16;
			BasicImage<fpOut> bb ((w-1)*factor+1, (h-1)*factor+1);
			unsigned int runs = 100;

			clock_t start, end;
			start = clock();  // measure the time
			
			for(int i = 0; i < runs; i++) {
				vigra:: resizeImageLinearInterpolation(srcImageRange(imgfp), 
					destImageRange(bb));
			}
			
			// end
			// fertig
			end = clock();                  // Ende der Zeitmessung
			std::cout << runs << " runs." << std::endl;
			printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
			
			res.resize((w-1)*factor+1, (h-1)*factor+1);
			copyImage(srcImageRange(imgGr,FixedPointConstAccessor<int>()), destImage(res));
		}

		vigra::exportImage(srcImageRange(res),
                      vigra::ImageExportInfo("myimage.jpg"));
		//~ std::cout << d << std::endl;
	
	}
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
	
}
