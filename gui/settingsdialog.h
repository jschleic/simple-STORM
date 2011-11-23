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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "ui_settingsdialog.h"

class QDialog;

class SettingsDialog : public QDialog, private Ui::SettingsDialog
{
    Q_OBJECT
    public:
        SettingsDialog(QWidget * parent=0);
        ~SettingsDialog();
        QString filterFilename() { return m_filterFilename->text(); }
        int roilen() { return m_roilen->itemData(m_roilen->currentIndex()).toInt(); }
        int pixelsize() { return m_pixelsize->value(); }
    public slots:
        void setFilterFilename(const QString &);
        void setRoilenAlternatives(const QList<QString>& desc, const QList<int>& value);
        void setRoilen(const int roilen);
        void setPixelsize(const int sz);
    private slots:
        void selectFilterFile();
        
};

#endif // STORMPARAMSDIALOG_H
