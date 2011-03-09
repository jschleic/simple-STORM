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
#include <vigra/multi_impex.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/utilities.hxx>



using namespace vigra;
using namespace vigra::functor;


// MAIN
int main(int argc, char** argv) {

	if(argc != 3) {
		std::cout << "usage: " << argv[0] << " infile.sif outfile" << std::endl << std::endl;
		std::cout << "Converts one sif file to tiff stack" << std::endl;
	}

	std::string infile = argv[1];
	std::string outfile = argv[2];
	
    try
    {

		MultiArray<3,float> in;
		typedef MultiArray<3, float>::difference_type Shape;


		int width, height, stacksize;
		HDF5ImportInfo info(infile.c_str(), "/data");
		
		width = info.shapeOfDimension(0);
		height = info.shapeOfDimension(1);
		stacksize = info.shapeOfDimension(2);

		// create a 3D array of appropriate size
		in.reshape(Shape(width,height,stacksize));
		readHDF5(info, in); //Eingabe Bild

		std::cout << "Images with Shape: " << Shape(width, height, stacksize) << std::endl;
		std::cout << "Processing a stack of " << stacksize << " images..." << std::endl;


		exportVolume(in, VolumeExportInfo(outfile.c_str(), ".tif"));
		


    }
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }
	
	
}
