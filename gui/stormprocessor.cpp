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

#define STORM_QT // Qt version
#include "wienerStorm.hxx"

#include "myimportinfo.hxx"
#include "stormprocessor.h"

typedef MultiArrayShape<3>::type Shape;

template <class T>
StormProcessor<T>::StormProcessor(MyImportInfo* info, const vigra::BasicImage<T> filter, const Shape3& shape,
			const int threshold, const int factor, const int roilen)
  : m_info(info),
  	m_filter(filter),
  	m_shape(shape),
  	m_threshold(threshold),
  	m_factor(factor),
  	m_roilen(roilen)
{
	// initialize fftw-wrapper; create plans
	MultiArrayView<3,T> in(m_shape[0],m_shape[1],1); //w x h x 1
    readBlock(info, Shape3(0,0,0), Shape3(m_shape[0],m_shape[1],1), in);
	BasicImageView<T> sampleinput = makeBasicImageView(in.bindOuter(0));  // access first frame as BasicImage
	m_fftwWrapper = new FFTFilter(sampleinput);

}

template <class T>
StormProcessor<T>::~StormProcessor()
{
	delete m_fftwWrapper;
}

template <class T>
std::set<Coord<T> > StormProcessor<T>::executeFrame(const int frame)
{
	MultiArrayView<3,T> in(m_shape[0],m_shape[1],1); //w x h x 1
	readBlock(m_info, Shape3(0,0,frame), Shape3(m_shape[0],m_shape[1],1), in);
	std::set<Coord<T> >& maxima_coords;
	MultiArrayView <2, T> in2 = in.bindOuter(0); // select current image
	wienerStormSingleFrame( in2, m_filter, maxima_coords, 
            m_fftwWrapper, m_threshold, m_factor, m_roilen);

	return maxima_coords;	
}
