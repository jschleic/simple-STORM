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

#include <vigra/impex.hxx>
#include "myimportinfo.h"
#ifdef HDF5_FOUND
	#include <vigra/hdf5impex.hxx>
#endif

#include <vigra/timing.hxx>

using namespace vigra;

// MAIN
int main(int argc, char** argv) {

    if(argc != 3) {
        std::cout << "Usage: " << argv[0] << " infile outfile" << std::endl;
    }

    std::string infile(argv[1]);
    std::string outfile(argv[2]);

    try
    {

		MultiArray<3,float> in;
		typedef MultiArrayShape<3>::type Shape;

        MyImportInfo info(infile);
        in.reshape(info.shape());
        readVolume(info, in);
        int stacksize = info.shape()[2];
        Size2D size2 (info.shapeOfDimension(0), info.shapeOfDimension(1)); // isnt' there a slicing operator?
        

        std::cout << "Images with Shape: " << info.shape() << std::endl;



        writeHDF5(outfile.c_str(), "/data", in);
        
        

    }
    catch (vigra::StdException & e)
    {
        std::cout<<"There was an error:"<<std::endl;
        std::cout << e.what() << std::endl;
        return 1;
    }	
}
