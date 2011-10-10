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

#ifndef STORMMODEL_H
#define STORMMODEL_H

#include <vigra/multi_array.hxx>
#include <vigra/basicimage.hxx>

class MyImportInfo;
template <class T>
class Coord;

template <class T>
class StormProcessor
{
	public:
		StormProcessor(MyImportInfo* info, const vigra::BasicImage<T> filter, const Shape3& shape,
			const int threshold, const int factor, const int roilen);
		~StormProcessor();
		std::set<Coord<T> > operator()(const int i) {return executeFrame(i);}
		std::set<Coord<T> > executeFrame(const int i);

	private:
		MyImportInfo * m_info;
		vigra::BasicImage<float> m_filter;
		vigra::MultiArrayShape<3>::type m_shape;
		int m_threshold;
		int m_factor;
		int m_roilen;
		FFTFilter* m_fftwWrapper;
};

#endif // STORMMODEL_H
