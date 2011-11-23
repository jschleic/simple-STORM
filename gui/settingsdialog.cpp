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
#include "qdebug.h"

SettingsDialog::SettingsDialog(QWidget * parent) 
    : QDialog(parent)
{
    setupUi(this);
    connect(m_selectFilterFile, SIGNAL(clicked()), SLOT(selectFilterFile()));

    QList<QString> roilen;
    QList<int> roival;
    roilen << "Fast (ROI 5px)" << "Accurate (ROI 9px)";
    roival << 5                << 9;
    setRoilenAlternatives(roilen, roival);
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

void SettingsDialog::setRoilenAlternatives(const QList<QString>& desc, const QList<int>& value)
{
    if (desc.size() != value.size()) {
        qDebug() << "desc and value must have the same number of elements.";
        return;
    }
    for(int i = 0; i < desc.size(); ++i) {
        m_roilen->insertItem(i, desc[i], QVariant(value[i]));
    }
}

void SettingsDialog::setRoilen(const int roilen) {
    for(int i=0; i<m_roilen->count(); ++i) {
        if(m_roilen->itemData(i) == roilen) {
            m_roilen->setCurrentIndex(i);
            break;
        }
    }
}

void SettingsDialog::setPixelsize(const int sz) {
    m_pixelsize->setValue(sz);
}
