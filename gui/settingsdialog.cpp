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

#include <QWidget>
#include <QFileDialog>
#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget * parent) 
    : QDialog(parent)
{
    setupUi(this);
    connect(m_selectFilterFile, SIGNAL(clicked()), SLOT(selectFilterFile()));
}

SettingsDialog::~SettingsDialog()
{

}

void SettingsDialog::selectFilterFile()
{
    QString filename =  QFileDialog::getOpenFileName ( this, "Filter file selection",
            m_filterFilename->text(), "Tiff Image (*.tif *.tiff)");
    if(filename != "") {
        m_filterFilename->setText(filename);
    }
}

void SettingsDialog::setFilterFilename(const QString& filename)
{
    m_filterFilename->setText(filename);
}
