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

#include "config.h"
#include <QString>
#include <QSettings>

namespace Config {
    
QString filterFilename() {
    QSettings settings;
    return settings.value("storm/filterFilename").toString();
}

void setFilterFilename(const QString& fn) {
    QSettings settings;
    settings.setValue("storm/filterFilename", fn);
}

int roilen() {
    QSettings settings;
    return settings.value("storm/roilen", 5).toInt();
}

void setRoilen(const int roilen) {
    QSettings settings;
    settings.setValue("storm/roilen", roilen);
}

int pixelsize() {
    QSettings settings;
    return settings.value("storm/pixelsize",  110).toInt();
}

void setPixelsize(const int sz) {
    QSettings settings;
    settings.setValue("storm/pixelsize", sz);
}

} // namespace Config
