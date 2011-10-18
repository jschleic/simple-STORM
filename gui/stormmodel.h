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

#include <QObject>
#include <vigra/multi_array.hxx>
#include <vigra/basicimage.hxx>
#include <vector>
#include <set>

class MyImportInfo;

class StormModel : public QObject
{
    Q_OBJECT
    public:
        StormModel(QObject * parent=0);
        ~StormModel();
    public slots:
        void setThreshold(const int);
        void setFactor(const int);
        void setInputFilename(const QString&);
        void setFilterFilename(const QString&);
        void setPreviewEnabled(const bool);
        QString filterFilename() const { return m_filterFilename; }
        QString inputFilename() const { return m_inputFilename; }
        int threshold() const { return m_threshold; }
        int factor() const { return m_factor; }
        int roilen() const { return m_roilen; }
        bool previewEnabled() const { return m_previewEnabled; }

    private:
        int m_threshold;
        int m_factor;
        QString m_inputFilename;
        QString m_filterFilename;
        int m_roilen;
        bool m_previewEnabled;

};

#endif // STORMMODEL_H
