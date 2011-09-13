/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*         Copyright 2010 by Joachim Schleicher and Ullrich Koethe      */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/************************************************************************/

#define CHUNKSIZE 10

#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include "program_options_getopt.h"
#include "wienerStorm.hxx"
#include "configVersion.hxx"

#include <vigra/impex.hxx>
#include "myimportinfo.hxx"
#ifdef HDF5_FOUND
	#include <vigra/hdf5impex.hxx>
#endif

#include <vigra/timing.hxx>


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
	
    try
    {

		MultiArray<3,float> in;
		typedef MultiArrayShape<3>::type Shape;

        MyImportInfo info(infile);
        in.reshape(info.shape());
        readVolume(info, in);
        int stacksize = info.shape()[2];
        Size2D size2 (info.shapeOfDimension(0), info.shapeOfDimension(1)); // isnt' there a slicing operator?
        

		if(verbose) {
			std::cout << "Images with Shape: " << info.shape() << std::endl;
			std::cout << "Processing a stack of " << stacksize << " images..." << std::endl;
		}


		// found spots. One Vector over all images in stack
		// the inner set contains all spots in the image
		std::vector<std::set<Coord<float> > > res_coords(stacksize);
		BasicImage<float> filter(info.shapeOfDimension(0), info.shapeOfDimension(1)); // filter in fourier space
		DImage res((size2-Diff2D(1,1))*factor+Diff2D(1,1));
		// check if outfile is writable, otherwise throw error -> exit
        exportImage(srcImageRange(res), ImageExportInfo(outfile.c_str()));
		if(coordsfile!="") {
			std::ofstream cf (coordsfile.c_str());
			vigra_precondition(cf.is_open(), "Could not open coordinate-file for writing.");
			cf.close();
		}

		USETICTOC;
		TIC;  // measure the time

		// STORM Algorithmus
		generateFilter(in, filter, filterfile);  // use the specified one or create wiener filter from the data
		wienerStorm(in, filter, res_coords, threshold, factor, roilen, frames, verbose);
		
		// resulting image
		drawCoordsToImage<Coord<float> >(res_coords, res);
		
		int numSpots = 0;
		if(coordsfile != "") {
			std::set<Coord<float> >::iterator it2;
			std::ofstream cfile (coordsfile.c_str());
			cfile << size2.width() << " " << size2.height() << " " << stacksize << std::endl;
			cfile << std::fixed; // fixed instead of scientific format
			for(unsigned int j = 0; j < res_coords.size(); j++) {
				for(it2=res_coords[j].begin(); it2 != res_coords[j].end(); it2++) {
					numSpots++;
					Coord<float> c = *it2;
					cfile << std::setprecision(3) << (float)c.x/factor << " " << (float)c.y/factor << " "
						<< j << " " << std::setprecision(1) << c.val << " 0" << std::endl;
				}
			}
			cfile.close();
		}
		
		// end: done.
		TOC;
		std::cout << "detected " << numSpots << " spots." << std::endl;

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
