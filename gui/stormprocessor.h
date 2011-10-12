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
template <class T>
class Coord;

class FFTFilter;
class QImage;

template <class T>
class StormProcessor
{
	public:
		typedef std::set<Coord<T> > result_type;
		StormProcessor(const MyImportInfo* const info, const StormModel* const model, FFTFilter* fftwWrapper);
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
		FFTFilter* m_fftwWrapper;
};


template <class T>
StormProcessor<T>::StormProcessor(const MyImportInfo* const info, const StormModel* const model, FFTFilter* fftwWrapper)
  : m_info(info),
	m_filter(vigra::BasicImage<T>(info->shape()[0], info->shape()[1])),
	m_shape(info->shape()),
	m_threshold(model->threshold()),
	m_factor(model->factor()),
	m_roilen(model->roilen()),
	m_fftwWrapper(fftwWrapper)
{
	// load filter image
	vigra::ImageImportInfo filterinfo(model->filterFilename().toStdString().c_str());
	if(!filterinfo.isGrayscale()) {
		// TODO: die?!
		vigra_fail("precondition failed: filter image should be grayscale");
	}
	vigra::BasicImage<T> filterIn(filterinfo.width(), filterinfo.height());
	vigra::importImage(filterinfo, destImage(filterIn)); // read the image
	vigra::resizeImageSplineInterpolation(srcImageRange(filterIn), destImageRange(m_filter));

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
	MyImportInfo* initStorm(const StormModel* const); /**< executes the data processing with its private parameters */
	void saveResults(const StormModel* const model, const vigra::Shape3& shape, const std::vector<std::set<Coord<T> > >& res); /**< save results and close all file pointers */
	void executeStormImages(const int from, const int to); /**< run storm algorithm */
	FFTFilter* createFFTFilter(const MyImportInfo* const info);

} // namespace storm


#endif // STORMPROCESSOR_H
