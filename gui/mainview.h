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

#ifndef MAINVIEW_H
#define MAINVIEW_H

#include "ui_mainview.h"
#include <QImage>

class QWidget;

class MainView : public QWidget, private Ui::MainView
{
    Q_OBJECT
    public:
        MainView(QWidget * parent=0);
        ~MainView();
    public slots:
        void setPreview(const QImage& preview);
        
};

#endif // MAINVIEW_H
