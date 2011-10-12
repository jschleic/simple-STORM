/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*      Copyright 2010-2011 by Joachim Schleicher                       */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/
 
#include <string>
#include <vigra/impex.hxx>
#include <vigra/sifImport.hxx>
#ifdef HDF5_FOUND
	#include <vigra/hdf5impex.hxx>
#endif

#include "myimportinfo.h"

#define N 3 // could eventually be a template parameter later on

using namespace vigra;


MyImportInfo::MyImportInfo(const std::string & filename) :
    m_filename(filename)
{

    std::string extension = filename.substr( filename.find_last_of('.'));
    if(extension==".tif" || extension==".tiff") {
        m_type = TIFF;
        vigra::ImageImportInfo* info = new vigra::ImageImportInfo(filename.c_str());
        ptr = (void*) info;
        m_shape = Shape(info->width(), info->height(), info->numImages());
    }
    else if(extension==".sif") {
        m_type = SIF;
        vigra::SIFImportInfo* info = new vigra::SIFImportInfo (filename.c_str());
        ptr = (void*) info;
        m_shape = Shape(info->shape()[0],info->shape()[1],info->shape()[2]);
    } 
    #ifdef HDF5_FOUND
    else if (extension==".h5" || extension==".hdf" || extension==".hdf5") {
        m_type = HDF5;
        vigra::HDF5File* h5file = new vigra::HDF5File(filename.c_str(), HDF5File::Open);
        ArrayVector<hsize_t> shape = h5file->getDatasetShape("/data");
        m_shape = Shape(shape[0],shape[1],shape[2]);
        ptr = (void*) h5file;
    } 
    #endif // HDF5_FOUND
    else {
        vigra_precondition(false, "Wrong filename-extension given. Currently supported: .sif .h5 .hdf .hdf5 .tif .tiff");
    }

}

MyImportInfo::~MyImportInfo() {
    switch(m_type) {
        case TIFF:
            delete (ImageImportInfo*)ptr;
            break;
        case SIF:
            delete (SIFImportInfo*)ptr;
            break;
        #ifdef HDF5_FOUND
        case HDF5:
            delete (HDF5File*)ptr;
            break;
        #endif // HDF5_FOUND
        default:
            break;
        }
}

