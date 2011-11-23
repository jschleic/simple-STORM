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

#ifndef PREVIEWTIMER_H
#define PREVIEWTIMER_H

#include <QObject>
#include <vigra/multi_array.hxx>
#include <vigra/basicimage.hxx>
#include "myimportinfo.h"
#include "stormmodel.h"
#include <QFuture>
#include <QImage>

class QTimer;
class QImage;
template <class T> class Coord;

class PreviewImage
{
    public:
        PreviewImage(const StormModel* const, const vigra::Shape3&, const QFuture<std::set<Coord<float> > >& futureResult,
                const int maxwidth=-1, const int maxheight=-1);
        ~PreviewImage();
        QImage getPreviewImage();
        bool hasNewResults();
    private:
        void drawScaleBar(QImage&);
        BasicImage<TinyVector<uchar,4> > m_colorResult;
        DImage m_result;
        const StormModel* const m_model;
        const vigra::Shape3 m_shape;
        int m_newwidth;
        int m_newheight;
        const QFuture<std::set<Coord<float> > >& m_futureResult;
        unsigned int m_processedIndex;
        float m_scale;
};

class PreviewTimer : public QObject
{
    Q_OBJECT
    public:
        PreviewTimer(PreviewImage*);
        ~PreviewTimer();
    signals:
        void previewChanged(QImage);
    public slots:
        void updatePreview();
        void start(int msec);
        void stop();
    private:
        PreviewImage* m_previewImage;
        QTimer* m_timer;
};


#endif // PREVIEWTIMER_H
