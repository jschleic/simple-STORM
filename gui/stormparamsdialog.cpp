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
#include "stormparamsdialog.h"

Stormparamsdialog::Stormparamsdialog(QWidget * parent) 
	: QDialog(parent)
{
	setupUi(this);
	connectSignals();
}

Stormparamsdialog::~Stormparamsdialog()
{

}

void Stormparamsdialog::connectSignals()
{
	connect(m_selectInputFile, SIGNAL(clicked()), SLOT(selectInputFile()));
	connect(m_selectFilterFile, SIGNAL(clicked()), SLOT(selectFilterFile()));
}

void Stormparamsdialog::selectInputFile()
{
	QString filename = 	QFileDialog::getOpenFileName ( this, "Input file selection" );
	m_inputFileName->setText(filename);
}

void Stormparamsdialog::selectFilterFile()
{
	QString filename = 	QFileDialog::getOpenFileName ( this, "Filter file selection" );
	m_filterFile->setText(filename);
}
