#include <iostream>
#include <vigra/impex.hxx>
#include <vigra/hdf5impex.hxx>
#include <vigra/multi_array.hxx>

#include <vigra/sifImport.hxx>

using namespace std;
using namespace vigra;



int main(int argc, char** argv) {
    if(argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " infile outfile" << std::endl;
        std::cout << "(supported formats: infile: sif, outfile: hdf5)" << std::endl;
        
        return 1;
    }
    
    try
    {
	SIFImportInfo info(argv[1]);

        // create a 3D array of appropriate size
        typedef MultiArray<3, float>::difference_type Shape;
        MultiArray<3, float> in(Shape(info.width(), info.height(), info.stacksize()));

	cout << "Image Shape: " << Shape(info.width(), info.height(), info.stacksize()) << endl;

	readSIF(info, in); //Eingabe Bild
		
	writeHDF5(argv[2], "/data", in);
       
    }
    catch (vigra::StdException & e)
    {
        std::cout << e.what() << std::endl;
        cout<<"There was an error:"<<endl;
        return 1;
    }
	
	
}
