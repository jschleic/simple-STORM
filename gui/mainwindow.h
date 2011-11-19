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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

class MainView;
class QCloseEvent;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    public:
        MainWindow();
        ~MainWindow();
        
        MainView *mainview() const { return m_mainView; }

    protected:
        void closeEvent(QCloseEvent *event);

    signals:
        void action_openCoordinatesList_triggered();
        void action_createFilter_triggered();
        void action_showStormparamsDialog_triggered();
        void action_showAboutDialog_triggered();
        void action_showSettingsDialog_triggered();

    private:
        void readSettings();
        void writeSettings();
};

#endif // MAINWINDOW_H
