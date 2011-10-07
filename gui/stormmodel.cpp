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

#include <QObject>
#include <qdebug.h>
#include "wienerStorm.hxx"
#include "stormmodel.h"
#include "myimportinfo.hxx"

typedef MultiArrayShape<3>::type Shape;

StormModel::StormModel(QObject * parent) 
	: QObject(parent),
	m_info(NULL),
	m_roilen(9)
{
}

StormModel::~StormModel()
{

}

bool StormModel::initStorm()
{
	qDebug() << "factor   : " << m_factor;
	qDebug() << "infile   : " << m_inputFilename;
	qDebug() << "filter   : " << m_filterFilename;
	qDebug() << "threshold: " << m_threshold;

	m_info = new MyImportInfo(m_inputFilename.toStdString());
	m_shape = m_info->shape();
	int stacksize = m_info->shapeOfDimension(2);

	Size2D size2 (m_shape[0], m_shape[1]); // isnt' there a slicing operator?
	m_filter.resize(size2); // filter in fourier space
	m_result.resize((size2-Diff2D(1,1))*m_factor+Diff2D(1,1));
	// found spots. One Vector over all images in stack
	// the inner set contains all spots in the image
	m_coords.resize(m_shape[2]); // stacksize vector elements

	qDebug() << "opened file. shape: " << m_shape[0] << " " << m_shape[1] << " " << m_shape[2];
// TODO
	//~ // check if outfile is writable, otherwise throw error -> exit
	//~ exportImage(srcImageRange(res), ImageExportInfo(outfile.c_str()));
	//~ if(coordsfile!="") {
		//~ std::ofstream cf (coordsfile.c_str());
		//~ vigra_precondition(cf.is_open(), "Could not open coordinate-file for writing.");
		//~ cf.close();
	//~ }
// TODO ende

	// WienerFilter: TODO -- here load or error. no filter generation please
	vigra::MultiArray<3,float> in;
	generateFilter(in, m_filter, m_filterFilename.toStdString());  // use the specified one or create wiener filter from the data
	qDebug() << "init done.";
}

void StormModel::abortStorm()
{
	// TODO: close filepointers
	delete m_info;
}

void StormModel::finishStorm()
{
	// TODO: save coordinates list and result image
	// resulting image
	//~ drawCoordsToImage(m_coords, m_result);
	//~ 
	//~ int numSpots = 0;
	//~ if(coordsfile != "") {
		//~ numSpots = saveCoordsFile(coordsfile, res_coords, info.shape(), factor);
	//~ }
	delete m_info;
}

void StormModel::executeStormImages(const int from, const int to)
{
	QString frames = QString("%1:%2").arg(from).arg(to);
	wienerStorm(*m_info, m_filter, m_coords, (float)m_threshold, m_factor, m_roilen, frames.toStdString());
}

int StormModel::numFrames()
{
	if (m_info != NULL) {
		return m_shape[2];
	} else {
		return -1;
	}
}

void StormModel::setThreshold(const int t)
{
	m_threshold = t;
}

void StormModel::setFactor(const int f)
{
	m_factor = f;
}

void StormModel::setInputFilename(const QString & f)
{
	m_inputFilename = f;
}

void StormModel::setFilterFilename(const QString & f)
{
	m_filterFilename = f;
}
