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
#include <vigra/impex.hxx>
#include "wienerStorm.hxx"
#include <vigra/sifImport.hxx>

#ifdef PROGRAM_OPTIONS_GETOPT
#include <map>
#include "program_options_getopt.h"
#elif PROGRAM_OPTIONS_BOOST
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
namespace po = boost::program_options;
#else
#error No Program options possibility set.
#endif

#include <time.h>
#ifdef CLK_TCK
#else
#define CLK_TCK 1000000.
#endif



using namespace vigra;
using namespace vigra::functor;

#ifdef PROGRAM_OPTIONS_BOOST
int parseProgramOptions(int argc, char** argv, po::variables_map& vm) {
	// Declare the supported options.
	po::options_description desc("Usage: storm [Options] infile [outfile] \nAllowed options");
	desc.add_options()
		("help", "produce help message")
		("verbose", "verbose message output")
		("factor", po::value<int>()->default_value(4), "set upscale factor")
		("threshold", po::value<float>()->default_value(800), "set background threshold")
		("infile", po::value<std::string>(), "sif input file")
		("outfile", po::value<std::string>(), "output file (.bmp .jpg .png .tif)")
		("coordsfile", po::value<std::string>(), "coordinates output file (format: one line for every spot detected)")
		("filter", po::value<std::string>(), "specify a filter in fft space, preferably a tiff image. if not set, a wiener filter is generated from the data)")
	;

	po::positional_options_description p;
	p.add("infile", 1);
	p.add("outfile", 2);

	po::store(po::command_line_parser(argc, argv).
          options(desc).positional(p).run(), vm);
	po::notify(vm);    

	// Print usage message and quit
	if (vm.count("help") || vm.count("infile")==0) {
		std::cout << desc << "\n";
		return -1;
	}

	return 0;
}
#endif

// Draw all coordinates into the resulting image
template <class C, class Image>
void drawCoordsToImage(std::vector<std::vector<C> >& coords, Image& res) {
	res = 0;
	//  loop over the coordinates
	typename std::vector<std::vector<C> >::iterator it;
	typename std::vector<C>::iterator it2;
	for(it = coords.begin(); it != coords.end(); ++it) {
		std::vector<C> r = *it;
		for(it2 = r.begin(); it2 != r.end(); it2++) {
			C c = *it2;
			res(c.x, c.y) += c.val;
		}
	}
}


// MAIN
int main(int argc, char** argv) {
	// Read commandline Parameters
	#ifdef PROGRAM_OPTIONS_GETOPT
	std::map<char, float> params;
	std::map<char, std::string> files;
	if(parseProgramOptions(argc, argv, params, files)!=0) {
		return -1;
	}
	int factor = params['g'];
	float threshold = params['t'];
	std::string infile = files['i'];
	std::string outfile = files['o'];
	std::string coordsfile = files['c'];
	std::string filterfile = files['f'];
    char verbose = params['v'];
    
    // defaults:
    factor 		= (factor==0)?4:factor;
    threshold	= (threshold==0)?800:threshold;
    #endif // PROGRAM_OPTIONS_GETOPT
    
    #ifdef PROGRAM_OPTIONS_BOOST
	po::variables_map vm;
	if(parseProgramOptions(argc, argv, vm)!=0) {
 		return -1;
 	}
	int factor = vm["factor"].as<int>();
	float threshold = vm["threshold"].as<float>();
	std::string infile = vm["infile"].as<std::string>();
	std::string outfile, coordsfile, filterfile;
	if(vm.count("outfile")) outfile = vm["outfile"].as<std::string>();
	if(vm.count("coordsfile")) coordsfile = vm["coordsfile"].as<std::string>();
	if(vm.count("filter")) filterfile = vm["filter"].as<std::string>();
	char verbose;
	if(vm.count("verbose")) verbose = 1;

    #endif // PROGRAM_OPTIONS_BOOST
    
    // defaults: put out- and coordsfile into the same folder as input
    if(outfile=="") {
		outfile = infile;
		outfile.replace(outfile.size()-4, 4, ".png");
	}
    if(coordsfile=="") {
		coordsfile = infile;
		coordsfile.replace(coordsfile.size()-4, 4, ".txt");
	}
    
    if(verbose) {
		std::cout << "thr:" << threshold << " factor:" << factor << std::endl;
	}
	
    try
    {

		clock_t start, end;

		SIFImportInfo info(infile.c_str());

        // create a 3D array of appropriate size
        typedef MultiArray<3, float>::difference_type Shape;
        MultiArray<3, float> in(Shape(info.width(), info.height(), info.stacksize()));

		std::cout << "Images with Shape: " << Shape(info.width(), info.height(), info.stacksize()) << std::endl;
		std::cout << "Processing a stack of " << info.stacksize() << " images..." << std::endl;

		readSIF(info, in); //Eingabe Bild

		// found spots. One Vector over all images in stack
		// the other over all spots in the image
		std::vector<std::vector<Coord<float> > > res_coords(info.stacksize());
		BasicImage<float> filter(info.width(), info.height()); // filter in fourier space

		start = clock();  // measure the time

		// STORM Algorithmus
		generateFilter(in, filter, filterfile);  // use the specified one or create wiener filter from the data
		wienerStorm(in, filter, res_coords, threshold, factor);

		// resulting image
		DImage res(factor*(info.width()-1)+1, factor*(info.height()-1)+1);
		drawCoordsToImage<Coord<float> >(res_coords, res);
		
		if(coordsfile != "") {
			std::vector<Coord<float> >::iterator it2;
			std::ofstream outfile;
			outfile.open(coordsfile.c_str());
			outfile << info.width() << " " << info.height() << " " << info.stacksize() << std::endl;
			for(int j = 0; j < res_coords.size(); j++) {
				for(it2=res_coords[j].begin(); it2 != res_coords[j].end(); it2++) {
					Coord<float> c = *it2;
					outfile << (float)c.x/factor << " " << (float)c.y/factor << " "
						<< j << " " << c.val << " 0" << std::endl;
				}
			}
			outfile.close();
		}
		
		// end
		// fertig
		end = clock();                  // Ende der Zeitmessung
		printf("The time was : %.3f    \n",(end - start) / (double)CLK_TCK);
		

		// some maxima are very strong so we use a logarithmic scale:
		//~ transformImage(srcImageRange(res), destImage(res), log(Arg1()+Param(1.))); // log
		
		double maxlim = 0., minlim = 0;
		findMinMaxPercentile(res, 0., minlim, 0.996, maxlim);
		transformImage(srcImageRange(res), destImage(res), ifThenElse(Arg1()>Param(maxlim), Param(maxlim), Arg1())); 
        exportImage(srcImageRange(res), ImageExportInfo(outfile.c_str()));
        
        

    }
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
	
	
}
