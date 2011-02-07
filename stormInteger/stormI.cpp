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
#ifdef HDF5_FOUND
	#include <vigra/hdf5impex.hxx>
#endif

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
		MultiArray<3,float> in;
		typedef MultiArray<3, float>::difference_type Shape;

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
		#else
			#warning Compiling without HDF5. No hdf5-input will be possible
		#endif // HDF5_FOUND
		else {
			vigra_precondition(false, "Wrong filename-extension given. Currently supported: .sif .h5 .hdf .hdf5");
			width=height=stacksize=0; // I dont want warnings
		}
		std::cout << "Images with Shape: " << Shape(width, height, stacksize) << std::endl;
		std::cout << "Processing a stack of " << stacksize << " images..." << std::endl;


		// found spots. One Vector over all images in stack
		// the inner one over all spots in the image
		std::vector<std::vector<Coord<float> > > res_coords(stacksize);

		start = clock();  // measure the time

		// STORM Algorithmus
		// TODO: load filter coeffs
		//~ generateFilter(in, filter, filterfile);  // use the specified one or create wiener filter from the data
		
		//~ vigra::Kernel2D<float> filter;
		//~ 
		//~ filter.initExplicitly(Diff2D(-5,-5), Diff2D(5,5)) =
		//~ 6.82061004e-04,  -1.70514323e-03,  -2.34587912e-03,
          //~ 2.54144181e-05,  -7.53481805e-04,   3.99524341e-03,
         //~ -1.82707041e-03,  -1.01589066e-03,  -2.49648816e-03,
         //~ -1.36125498e-03,   1.05668097e-03,
        //~ -1.77996844e-03,  -2.76245724e-03,   2.97157865e-03,
          //~ 7.40959744e-03,   2.87141807e-03,   9.32082069e-03,
          //~ 3.00057453e-03,   6.51313928e-03,   1.86222296e-03,
         //~ -3.08734225e-03,  -1.57424476e-03,
        //~ -2.10459308e-03,   2.56263671e-03,   8.15956797e-03,
         //~ -1.42163651e-03,  -1.83842053e-02,  -9.67897170e-03,
         //~ -1.51505626e-02,   9.08090372e-04,   8.57278355e-03,
          //~ 2.28224483e-03,  -2.40138455e-03,
         //~ 5.46324192e-04,   6.35264268e-03,  -1.44714040e-03,
         //~ -2.14248549e-02,  -1.89065851e-02,   1.09059976e-02,
         //~ -1.77173916e-02,  -2.08104433e-02,  -1.05560323e-03,
          //~ 6.84167310e-03,   6.94183606e-04,
         //~ 2.41118410e-03,   2.86977388e-03,  -1.66698850e-02,
         //~ -1.05616346e-02,   7.28007949e-02,   1.53224535e-01,
          //~ 6.91412061e-02,  -1.51059367e-02,  -1.88650059e-02,
          //~ 2.99100244e-03,   3.05378153e-03,
         //~ 5.12412344e-03,   2.14494105e-03,  -1.79994262e-02,
          //~ 1.99028291e-02,   1.69933100e-01,   2.90208753e-01,
          //~ 1.69933100e-01,   1.99028291e-02,  -1.79994262e-02,
          //~ 2.14494105e-03,   5.12412344e-03,
         //~ 3.05378153e-03,   2.99100244e-03,  -1.88650059e-02,
         //~ -1.51059367e-02,   6.91412061e-02,   1.53224535e-01,
          //~ 7.28007949e-02,  -1.05616346e-02,  -1.66698850e-02,
          //~ 2.86977388e-03,   2.41118410e-03,
         //~ 6.94183606e-04,   6.84167310e-03,  -1.05560323e-03,
         //~ -2.08104433e-02,  -1.77173916e-02,   1.09059976e-02,
         //~ -1.89065851e-02,  -2.14248549e-02,  -1.44714040e-03,
          //~ 6.35264268e-03,   5.46324192e-04,
        //~ -2.40138455e-03,   2.28224483e-03,   8.57278355e-03,
          //~ 9.08090372e-04,  -1.51505626e-02,  -9.67897170e-03,
         //~ -1.83842053e-02,  -1.42163651e-03,   8.15956797e-03,
          //~ 2.56263671e-03,  -2.10459308e-03,
        //~ -1.57424476e-03,  -3.08734225e-03,   1.86222296e-03,
          //~ 6.51313928e-03,   3.00057453e-03,   9.32082069e-03,
          //~ 2.87141807e-03,   7.40959744e-03,   2.97157865e-03,
         //~ -2.76245724e-03,  -1.77996844e-03,
         //~ 1.05668097e-03,  -1.36125498e-03,  -2.49648816e-03,
         //~ -1.01589066e-03,  -1.82707041e-03,   3.99524341e-03,
         //~ -7.53481805e-04,   2.54144181e-05,  -2.34587912e-03,
         //~ -1.70514323e-03,   6.82061004e-04;
		
		
		vigra::Kernel2D<int> filter;
		
		filter.initExplicitly(Diff2D(-5,-5), Diff2D(5,5)) =		
		 1,   -2,   -2,    0,   -1,    4,   -2,   -1,   -3,   -1,    1,
         -2,   -3,    3,    7,    3,    9,    3,    7,    2,   -3,   -2,
         -2,    3,    8,   -1,  -19,  -10,  -15,    1,    9,    2,   -2,
          1,    6,   -1,  -22,  -19,   11,  -18,  -21,   -1,    7,    1,
          2,    3,  -17,  -11,   74,  155,   70,  -15,  -19,    3,    3,
          5,    2,  -18,   20,  172,  294,  172,   20,  -18,    2,    5,
          3,    3,  -19,  -15,   70,  155,   74,  -11,  -17,    3,    2,
          1,    7,   -1,  -21,  -18,   11,  -19,  -22,   -1,    6,    1,
         -2,    2,    9,    1,  -15,  -10,  -19,   -1,    8,    3,   -2,
         -2,   -3,    2,    7,    3,    9,    3,    7,    3,   -3,   -2,
          1,   -1,   -3,   -1,   -2,    4,   -1,    0,   -2,   -2,    1;

		wienerStorm(in, filter, res_coords, threshold, factor);

		// resulting image
		DImage res(factor*(width-1)+1, factor*(height-1)+1);
		drawCoordsToImage<Coord<float> >(res_coords, res);
		
		if(coordsfile != "") {
			std::vector<Coord<float> >::iterator it2;
			std::ofstream outfile;
			outfile.open(coordsfile.c_str());
			outfile << width << " " << height << " " << stacksize << std::endl;
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
