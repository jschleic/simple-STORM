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

#include <QObject>
#include <qdebug.h>
#include "stormmodel.h"

StormModel::StormModel(QObject * parent) 
	: QObject(parent)
{
}

StormModel::~StormModel()
{

}

void StormModel::runStorm()
{
	qDebug() << "runStorm requested. Not yet implemented";
	qDebug() << "factor: " << m_factor;
	qDebug() << "infile: " << m_inputFilename;
}

void StormModel::setThreshold(const int t)
{
	m_threshold = t;
}

void StormModel::setFactor(const int f)
{
	m_factor = f;
}

void StormModel::setInputFilename(const QString & f)
{
	m_inputFilename = f;
}

void StormModel::setFilterFilename(const QString & f)
{
	m_filterFilename = f;
}
