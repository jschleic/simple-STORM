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

#include "wienerStorm.hxx" // why has this to be the first include??
#include "previewtimer.h"
#include <QImage>
#include <QTimer>

PreviewTimer::PreviewTimer(PreviewImage* previewImage)
    : m_previewImage(previewImage),
    m_timer(new QTimer(this))
{
    connect(m_timer, SIGNAL(timeout()), SLOT(updatePreview()));
}

PreviewTimer::~PreviewTimer()
{
}

void PreviewTimer::start(int msec)
{
    m_timer->start(msec);
}

void PreviewTimer::stop()
{
    m_timer->stop();
}

void PreviewTimer::updatePreview()
{
    if(m_previewImage->hasNewResults()) {
        emit previewChanged(m_previewImage->getPreviewImage());
    }
}



namespace vigra
{
/********************************************************/
/*                                                      */
/*                  GrayToRGBAccessor                   */
/*                                                      */
/********************************************************/

    /** Create an RGB view for a grayscale image by making all three channels
        equal.

    <b>\#include</b> <vigra/rgbvalue.hxx><br>
    Namespace: vigra
    */
template <class VALUETYPE>
class GrayToRGBAAccessor
{
   public:
    typedef typename vigra::TinyVector<VALUETYPE,4> value_type;

     /** Get RGB value for the given pixel.
     */
    template <class ITERATOR>
    value_type operator()(ITERATOR const & i) const {
             return value_type(*i,*i,*i, 0xff); }

     /** Get RGB value at an offset
     */
    template <class ITERATOR, class DIFFERENCE>
    value_type operator()(ITERATOR const & i, DIFFERENCE d) const
    {
        return value_type(i[d],i[d],i[d],0xff);
    }

    template <class V, class ITERATOR>
    void set(V const & value, ITERATOR const & i) const
    { 
        V v = detail::RequiresExplicitCast<VALUETYPE>::cast(value); 
        *i = value_type(v,v,v,0xff);
    }

};

} // namespace vigra

PreviewImage::PreviewImage(const StormModel* const model, const vigra::Shape3& shape, 
            const QFuture<std::set<Coord<float> > >& futureResult, 
            const int maxwidth, const int maxheight)
    : m_model(model),
    m_shape(shape),
    m_newwidth(model->factor()*(shape[0]-1)+1),
    m_newheight(model->factor()*(shape[1]-1)+1),
    m_futureResult(futureResult),
    m_processedIndex(0),
    m_scale(1.f)
{
    // scale image down to fit display properly
    if(maxwidth>0 && m_newwidth>maxwidth) {
        m_scale = float(maxwidth) / m_newwidth;
    }
    if(maxheight>0 && m_newheight>maxheight) {
        float scaley = float(maxheight) / m_newheight;
        m_scale = (scaley<m_scale)?scaley:m_scale; // minimum
    }
    m_newheight = int(m_scale*m_newheight);
    m_newwidth = int(m_scale*m_newwidth);

    m_colorResult.resize(m_newwidth, m_newheight);
    m_result.resize(m_newwidth, m_newheight);
    m_result = 0.;
}

PreviewImage::~PreviewImage()
{
}

QImage PreviewImage::getPreviewImage()
{
    // resulting image
    int resultCount = m_futureResult.resultCount();
    for(int i = m_processedIndex; i < resultCount; ++i) {
        const std::set<Coord<float> > coords = m_futureResult.resultAt(i);
        std::set<Coord<float> >::iterator it2;

        for(it2 = coords.begin(); it2 != coords.end(); it2++) {
            Coord<float> c = *it2;
            m_result(int(m_scale*c.x), int(m_scale*c.y)) += c.val;
        }
    }
    m_processedIndex = resultCount;

    // some maxima are very strong so we scale the image as appropriate :
    double maxlim = 0., minlim = 0;
    findMinMaxPercentile(m_result, 0., minlim, 0.996, maxlim);
    std::cout << "cropping output values to range [" << minlim << ", " << maxlim << "]" << std::endl;
    vigra::transformImage(srcImageRange(m_result), destImage(m_colorResult, GrayToRGBAAccessor<uchar>()), ifThenElse(Arg1()>Param(maxlim), Param(255), Arg1()*Param(255./(maxlim-minlim))));
    QImage resultImage (reinterpret_cast<const uchar*>(m_colorResult.begin()), m_newwidth, m_newheight, QImage::Format_RGB32);
    return resultImage.copy();
}

bool PreviewImage::hasNewResults()
{
    int newResults = m_futureResult.resultCount()-m_processedIndex;
    qDebug()<< newResults;
    if(newResults > 64) {
        return true;
    } else {
        return false;
    }
}
