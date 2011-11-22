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
#include <fstream>
#include <map>
#include "program_options_getopt.h"
#include "wienerStorm.hxx"
#include "configVersion.hxx"
#include "fftfilter.hxx"

#include <vigra/impex.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/sifImport.hxx>
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
	std::string infile = files['i'];
	std::string outfile = files['o'];
	std::string filterfile = files['f'];
    char verbose = (char)params['v'];


    try
    {

		MultiArray<3,float> in;
		MultiArray<3,float> out;
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
			out.reshape(Shape(info.width(), info.height(), info.stacksize()));
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
			out.reshape(Shape(width,height,stacksize));
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

		typedef float T;
		BasicImage<T> filter(width, height); // filter in fourier space
		writeHDF5(outfile.c_str(), "/data", out); // check if outfile writable

		// STORM Algorithmus
		generateFilter(in, filter, filterfile);  // use the specified one or create wiener filter from the data

		MultiArrayView <2, T> array0 = in.bindOuter(0); // select first image
		BasicImageView<T> firstImage = makeBasicImageView(array0);  // access data as BasicImage
		FFTFilter<T> fff(srcImageRange(firstImage));
		BasicImage<float > halffilter(width/2+1,height);
		copyImage(srcIterRange(filter.upperLeft(),filter.upperLeft()+Diff2D(width/2+1,height)), destImage(halffilter));

		#pragma omp parallel for schedule(static, CHUNKSIZE)
		for(int i = 0; i < stacksize; ++i) {
			MultiArrayView <2, T> array = in.bindOuter(i); // select current image
			MultiArrayView <2, T> outarray = out.bindOuter(i); // select current image

			BasicImageView<T> input = makeBasicImageView(array);  // access data as BasicImage
			BasicImageView<T> output = makeBasicImageView(outarray);  // access data as BasicImage

			//fft, filter with Wiener filter in frequency domain, inverse fft, take real part
			//~ vigra::applyFourierFilter(srcImageRange(input), srcImage(filter), destImage(output));
			fff.applyFourierFilter(srcImageRange(input), srcImage(halffilter), destImage(output));
		}

        writeHDF5(outfile.c_str(), "/data", out);
        

    }
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }	
}
