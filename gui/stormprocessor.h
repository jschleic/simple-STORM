/*
 * Copyright (C) 2011 Joachim Schleicher <J.Schleicher@stud.uni-heidelberg.de>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef STORMPROCESSOR_H
#define STORMPROCESSOR_H

#include <vigra/multi_array.hxx>
#include <vigra/basicimage.hxx>
#include "myimportinfo.h"
#include "stormmodel.h"
#include <QFuture>
#include <QMessageBox>
template <class T>
class Coord;

template <class T>
class FFTFilter;
class QImage;

template <class T>
class StormProcessor
{
    public:
        typedef std::set<Coord<T> > result_type;
        StormProcessor(const MyImportInfo* const info, const StormModel* const model, FFTFilter<T>* fftwWrapper);
        ~StormProcessor();
        std::set<Coord<T> > operator()(const int i) const {return executeFrame(i);}
        std::set<Coord<T> > executeFrame(const int i) const;

    private:
        const MyImportInfo * const m_info;
        vigra::BasicImage<float> m_filter;
        vigra::MultiArrayShape<3>::type m_shape;
        int m_threshold;
        int m_factor;
        int m_roilen;
        FFTFilter<T>* m_fftwWrapper;
};


template <class T>
StormProcessor<T>::StormProcessor(const MyImportInfo* const info, const StormModel* const model, FFTFilter<T>* fftwWrapper)
  : m_info(info),
    m_filter(vigra::BasicImage<T>(info->shape()[0], info->shape()[1])),
    m_shape(info->shape()),
    m_threshold(model->threshold()),
    m_factor(model->factor()),
    m_roilen(model->roilen()),
    m_fftwWrapper(fftwWrapper)
{
    try {
        vigra::exportImage(vigra::srcImageRange(m_filter), vigra::ImageExportInfo("daaaa.png"));
        // load filter image
        vigra::ImageImportInfo filterinfo(model->filterFilename().toStdString().c_str());
        if(!filterinfo.isGrayscale()) {
            // TODO: die?!
            QMessageBox::critical(0, "storm", "Filter should be grayscale.");
            vigra_fail("precondition failed: filter image should be grayscale");
        }
        vigra::BasicImage<T> filterIn(filterinfo.width(), filterinfo.height());
        vigra::importImage(filterinfo, destImage(filterIn)); // read the image
        vigra::resizeImageSplineInterpolation(srcImageRange(filterIn), destImageRange(m_filter));
    } catch (vigra::StdException & e) {
        QMessageBox::critical(0, "storm", "Filter file could not be opened");
        vigra_fail("filter could not be opened"); // TODO: make this class a QObject, emit Signal from here to notify app.
    }

}

template <class T>
StormProcessor<T>::~StormProcessor()
{
}

template <class T>
std::set<Coord<T> > StormProcessor<T>::executeFrame(const int frame) const
{
    MultiArray<3,T> in(vigra::Shape3(m_shape[0],m_shape[1],1)); //w x h x 1
    readBlock(*m_info, vigra::Shape3(0,0,frame), vigra::Shape3(m_shape[0],m_shape[1],1), in);
    std::set<Coord<T> > maxima_coords;
    MultiArrayView <2, T> in2 = in.bindOuter(0); // select current image
    wienerStormSingleFrame( in2, m_filter, maxima_coords,
            *m_fftwWrapper, (T)m_threshold, m_factor, m_roilen);

    return maxima_coords;
}

namespace storm
{
    template <class T>
    void saveResults(const StormModel* const model, const vigra::Shape3& shape, const std::vector<std::set<Coord<T> > >& res); /**< save results and close all file pointers */
    void executeStormImages(const int from, const int to); /**< run storm algorithm */

    template <class T>
    FFTFilter<T>* createFFTFilter(const MyImportInfo* const info);

    template <class T>
    void constructWienerFilter(const MyImportInfo* const info, const std::string& outfile)
    {
        vigra::BasicImage<T> filter(info->shape(0), info->shape(1));
        constructWienerFilter<T>(*info, filter);
        vigra::exportImage(srcImageRange(filter), outfile.c_str()); // save to disk
    }



//-- implementations
template <class T>
FFTFilter<T>* createFFTFilter(const MyImportInfo* const info)
{
    vigra::Shape3  shape = info->shape();
    // initialize fftw-wrapper; create plans
    MultiArray<3,T> in(vigra::Shape3(shape[0],shape[1],1)); //w x h x 1
    readBlock(*info, Shape3(0,0,0), Shape3(shape[0],shape[1],1), in);
    BasicImageView<T> sampleinput = makeBasicImageView(in.bindOuter(0));  // access first frame as BasicImage
    return new FFTFilter<T>(srcImageRange(sampleinput));
}

template <class T>
void saveResults(const StormModel* const model, const vigra::Shape3& shape, const std::vector<std::set<Coord<T> > >& coords)
{
    // save coordinates list and result image
    size_t pos = model->inputFilename().toStdString().find_last_of('.');
    std::string outfile = model->inputFilename().toStdString();
    outfile.replace(pos, 255, ".png"); // replace extension
    std::string coordsfile = model->inputFilename().toStdString();
    coordsfile.replace(pos, 255, ".txt");
    // resulting image
    int factor = model->factor();
    vigra::DImage result(factor*(shape[0]-1)+1,factor*(shape[1]-1)+1);
    drawCoordsToImage(coords, result);
    // some maxima are very strong so we scale the image as appropriate :
    double maxlim = 0., minlim = 0;
    findMinMaxPercentile(result, 0., minlim, 0.996, maxlim);
    std::cout << "cropping output values to range [" << minlim << ", " << maxlim << "]" << std::endl;
    if(maxlim > minlim) {
        transformImage(srcImageRange(result), destImage(result), ifThenElse(Arg1()>Param(maxlim), Param(maxlim), Arg1())); 
    }
    vigra::exportImage(vigra::srcImageRange(result), vigra::ImageExportInfo(outfile.c_str()));

    int numSpots = 0;
    if(coordsfile != "") {
        numSpots = saveCoordsFile(coordsfile, coords, shape, factor);
    }
    qDebug() << QString("found %1 spots.").arg(numSpots);

}


} // namespace storm


#endif // STORMPROCESSOR_H
