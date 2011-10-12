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

#include <qdebug.h>

#define STORM_QT // Qt version
#include "wienerStorm.hxx"

#include "myimportinfo.h"
#include "stormprocessor.h"
#include "stormmodel.h"
#include <vigra/impex.hxx>
#include <QImage>


namespace storm
{

MyImportInfo* initStorm(const StormModel* const model)
{

	MyImportInfo* info = new MyImportInfo(model->inputFilename().toStdString());
	Shape3 shape = info->shape();

	Size2D size2 (shape[0], shape[1]); // isnt' there a slicing operator?

	qDebug() << "opened file. shape: " << shape[0] << " " << shape[1] << " " << shape[2];
// TODO
	//~ // check if outfile is writable, otherwise throw error -> exit
	//~ exportImage(srcImageRange(res), ImageExportInfo(outfile.c_str()));
	//~ if(coordsfile!="") {
		//~ std::ofstream cf (coordsfile.c_str());
		//~ vigra_precondition(cf.is_open(), "Could not open coordinate-file for writing.");
		//~ cf.close();
	//~ }
// TODO ende

	qDebug() << "init done.";
	return info;
}

FFTFilter* createFFTFilter(const MyImportInfo* const info)
{
	vigra::Shape3  shape = info->shape();
	// initialize fftw-wrapper; create plans
	MultiArray<3,T> in(vigra::Shape3(shape[0],shape[1],1)); //w x h x 1
    readBlock(*info, Shape3(0,0,0), Shape3(shape[0],shape[1],1), in);
	BasicImageView<T> sampleinput = makeBasicImageView(in.bindOuter(0));  // access first frame as BasicImage
	return new FFTFilter(sampleinput);
}

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

