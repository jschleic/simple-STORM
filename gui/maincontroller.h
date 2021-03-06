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

#ifndef MAINCONTROLLER_H
#define MAINCONTROLLER_H

#include <QObject>
class MainView;
class StormModel;
class MainWindow;
class Stormparamsdialog;

class MainController : public QObject
{
    Q_OBJECT
    public:
        MainController(MainWindow * window);
        ~MainController();

    private slots:
        void showStormparamsDialog();
        void showAboutDialog();
        void showSettingsDialog();
        void showCreateFilterDialog();
        void runStorm();
        void runCreateWienerFilter(const QString&, const QString&) const;

    private:
        MainView *m_view;
        Stormparamsdialog * m_stormparamsDialog;
        StormModel * m_model;

        void connectSignals(MainWindow* window);
};

#endif // MAINCONTROLLER_H
