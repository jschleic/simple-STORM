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
#include <fstream>
#include <map>
#include "program_options_getopt.h"
#include "wienerStorm.hxx"
#include "configVersion.hxx"

#include <vigra/impex.hxx>
#include <vigra/sifImport.hxx>
#ifdef HDF5_FOUND
	#include <vigra/hdf5impex.hxx>
#endif

#include <time.h>
#ifdef CLK_TCK
#else
#define CLK_TCK 1000000.
#endif

using namespace vigra;
using namespace vigra::functor;

// Draw all coordinates into the resulting image
template <class C, class Image>
void drawCoordsToImage(std::vector<std::set<C> >& coords, Image& res) {
	res = 0;
	//  loop over the coordinates
	typename std::vector<std::set<C> >::iterator it;
	typename std::set<C>::iterator it2;
	for(it = coords.begin(); it != coords.end(); ++it) {
		std::set<C> r = *it;
		for(it2 = r.begin(); it2 != r.end(); it2++) {
			C c = *it2;
			res(c.x, c.y) += c.val;
		}
	}
}


// MAIN
int main(int argc, char** argv) {
	// Read commandline Parameters
	std::map<char, double> params;
	std::map<char, std::string> files;
	if(parseProgramOptions(argc, argv, params, files)!=0) {
		return -1;
	}
	int factor = (int)params['g'];
	int roilen = (int)params['m'];
	float threshold = params['t'];
	std::string infile = files['i'];
	std::string outfile = files['o'];
	std::string coordsfile = files['c'];
	std::string filterfile = files['f'];
	std::string frames = files['F'];
    char verbose = (char)params['v'];
        
    if(verbose) {
		std::cout << "thr:" << threshold << " factor:" << factor << std::endl;
	}
	
	clock_t start, end;
    try
    {

		MultiArray<3,float> in;
		typedef MultiArray<3, float>::difference_type Shape;

		// TODO: read file in extra function
		std::string extension = infile.substr( infile.find_last_of('.'));
		int width, height, stacksize;
		if(extension==".sif") {
			SIFImportInfo info(infile.c_str());
			width = info.width();
			height = info.height();
			stacksize = info.stacksize();

			// create a 3D array of appropriate size
			in.reshape(Shape(info.width(), info.height(), info.stacksize()));
			readSIF(info, in); //Eingabe Bild
		} 
		#ifdef HDF5_FOUND
		else if (extension==".h5" || extension==".hdf" || extension==".hdf5") {
			HDF5ImportInfo info(infile.c_str(), "/data");
			
			//MultiArrayShape<3>::type shape(info.shape().begin()); // TinyVector Overload error?!
			width = info.shapeOfDimension(0);
			height = info.shapeOfDimension(1);
			stacksize = info.shapeOfDimension(2);
			in.reshape(Shape(width,height,stacksize));
			readHDF5(info, in);
		} 
		#endif // HDF5_FOUND
		else {
			vigra_precondition(false, "Wrong filename-extension given. Currently supported: .sif .h5 .hdf .hdf5");
			width=height=stacksize=0; // I dont want warnings
		}

		if(verbose) {
			std::cout << "Images with Shape: " << Shape(width, height, stacksize) << std::endl;
			std::cout << "Processing a stack of " << stacksize << " images..." << std::endl;
		}


		// found spots. One Vector over all images in stack
		// the inner set contains all spots in the image
		std::vector<std::set<Coord<float> > > res_coords(stacksize);
		BasicImage<float> filter(width, height); // filter in fourier space

		start = clock();  // measure the time

		// STORM Algorithmus
		generateFilter(in, filter, filterfile);  // use the specified one or create wiener filter from the data
		wienerStorm(in, filter, res_coords, threshold, factor, roilen, frames, verbose);
		
		// resulting image
		DImage res(factor*(width-1)+1, factor*(height-1)+1);
		drawCoordsToImage<Coord<float> >(res_coords, res);
		
		if(coordsfile != "") {
			std::set<Coord<float> >::iterator it2;
			std::ofstream outfile;
			outfile.open(coordsfile.c_str());
			outfile << width << " " << height << " " << stacksize << std::endl;
			for(unsigned int j = 0; j < res_coords.size(); j++) {
				for(it2=res_coords[j].begin(); it2 != res_coords[j].end(); it2++) {
					Coord<float> c = *it2;
					outfile << (float)c.x/factor << " " << (float)c.y/factor << " "
						<< j << " " << c.val << " 0" << std::endl;
				}
			}
			outfile.close();
		}
		
		// end: done.
		end = clock();                  // Ende der Zeitmessung
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		

		// some maxima are very strong so we scale the image as appropriate :
		double maxlim = 0., minlim = 0;
		findMinMaxPercentile(res, 0., minlim, 0.996, maxlim);
		std::cout << "cropping output values to range [" << minlim << ", " << maxlim << "]" << std::endl;
		if(maxlim > minlim) {
			transformImage(srcImageRange(res), destImage(res), ifThenElse(Arg1()>Param(maxlim), Param(maxlim), Arg1())); 
		}
        exportImage(srcImageRange(res), ImageExportInfo(outfile.c_str()));
        
        

    }
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }	
}
