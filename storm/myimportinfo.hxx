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

#ifndef MYIMPORTINFO_H
#define MYIMPORTINFO_H

#define N 3 // could eventually be a template parameter later on

enum FileType { UNDEFINED, TIFF, HDF5, SIF };

using namespace vigra;

class MyImportInfo {
    typedef vigra::MultiArrayShape<N>::type Shape;
  public:
    MyImportInfo(std::string & filename);
    ~MyImportInfo();

    const Shape & shape() const { return m_shape; }
    vigra::MultiArrayIndex shape(const int dim) const { return m_shape[dim]; }
    std::string getAttribute(std::string & key); // like a dictionary // TODO

    vigra::MultiArrayIndex numDimensions() const;

    vigra::MultiArrayIndex shapeOfDimension(const int dim) const { return m_shape[dim]; }

    FileType type() const { return m_type; };
    
    const std::string & filename() const { return m_filename; }
    
    void * ptr; // hack

  private:
    std::string m_filename;
    Shape m_shape;
    FileType m_type;

};

MyImportInfo::MyImportInfo(std::string & filename) :
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
        case HDF5:
            delete (HDF5File*)ptr;
            break;
        default:
            break;
        }
}

template <class  T>
void readVolume(MyImportInfo & info, MultiArrayView<N, T> & array) {
    std::string filename = info.filename();
    switch(info.type()) {
        case TIFF:
        {
            ImageImportInfo* info2 = reinterpret_cast<ImageImportInfo*>(info.ptr);
            vigra_precondition(array.size(2)==info2->numImages(),"array shape and number of images in tiff file differ.");
            for(int i = 0; i < info2->numImages(); ++i) {
                MultiArrayView <2, T> img = array.bindOuter(i);
                BasicImageView <T> v = makeBasicImageView(img);
                info2->setImageIndex(i);
                importImage(*info2, destImage(v));
            }
            break;
        }
        case SIF:
        {
            SIFImportInfo* info2 = reinterpret_cast<SIFImportInfo*>(info.ptr);
            readSIF(*info2, array);
            break;
        }
        #ifdef HDF5_FOUND
        case HDF5:
        {
            HDF5File* info2 = reinterpret_cast<HDF5File*>(info.ptr);
            info2->read("/data", array);
            break;
        }
        #endif // HDF5_FOUND
        default:
            vigra_fail("decoder for type not implemented.");
    }
}

template <class  T>
void readBlock(const MyImportInfo & info, 
            const MultiArrayShape<N>::type& blockOffset, 
            const MultiArrayShape<N>::type& blockShape, 
            MultiArrayView<N, T> & array) 
{
    std::string filename = info.filename();
    switch(info.type()) {
        case TIFF:
        {
            ImageImportInfo* info2 = reinterpret_cast<ImageImportInfo*>(info.ptr);
            vigra_precondition(blockOffset[0]==0 && blockOffset[1]==0 && 
                    blockShape[0]==info.shapeOfDimension(0) && blockShape[1]==info.shapeOfDimension(1),
                    "for Tiff images only complete Frames are currently supported as ROIs");
            vigra_precondition(array.size(2)==blockShape[2],"array shape and number of images in ROI for tiff file differ.");
            vigra_precondition(blockShape[2] <= info2->numImages(), "block shape larger than number of frames in the image");
            for(int i = 0; i < blockShape[2]; ++i) {
                MultiArrayView <2, T> img = array.bindOuter(i);
                BasicImageView <T> v = makeBasicImageView(img);
                info2->setImageIndex(i+blockOffset[2]);
                importImage(*info2, destImage(v));
            }
            break;
        }
        case SIF:
        {
            SIFImportInfo* info2 = reinterpret_cast<SIFImportInfo*>(info.ptr);
            readSIFBlock(*info2, blockOffset, blockShape, array);
            break;
        }
        #ifdef HDF5_FOUND
        case HDF5:
        {
            HDF5File* info2 = reinterpret_cast<HDF5File*>(info.ptr);
            info2->readBlock("/data", blockOffset, blockShape, array);
            break;
        }
        #endif // HDF5_FOUND
        default:
            vigra_fail("decoder for type not implemented.");
    }
}

#endif // MYIMPORTINFO_H
