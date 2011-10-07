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
 
#include "mainwindow.h"
#include "maincontroller.h"
#include "mainview.h"
#include "stormparamsdialog.h"
#include "stormmodel.h"
#include <qdebug.h>
#include <QMessageBox>
#include <QProgressDialog>

MainController::MainController(MainWindow * window) 
	: QObject(window), 
	m_view(window->mainview()),
	m_stormparamsDialog(new Stormparamsdialog(m_view)),
	m_model(new StormModel())
{
	connectSignals(window);

	// default values
	m_model->setThreshold(m_stormparamsDialog->threshold());
	m_model->setFactor(m_stormparamsDialog->factor());

	showStormparamsDialog();
}

MainController::~MainController() 
{

}

void MainController::connectSignals(MainWindow* window)
{
	connect(window, SIGNAL(action_showAboutDialog_triggered()), SLOT(showAboutDialog()));
	connect(window, SIGNAL(action_showStormparamsDialog_triggered()), SIGNAL(showStormparamsDialog()));
	connect(this, SIGNAL(showStormparamsDialog()), SLOT(startStormDialog()));

	connect(m_stormparamsDialog, SIGNAL(accepted()), this, SLOT(runStorm()));
	connect(m_stormparamsDialog, SIGNAL(inputFilenameChanged(const QString&)), m_model, SLOT(setInputFilename(const QString&)));
	connect(m_stormparamsDialog, SIGNAL(factorChanged(const int)), m_model, SLOT(setFactor(const int)));
	connect(m_stormparamsDialog, SIGNAL(thresholdChanged(const int)), m_model, SLOT(setThreshold(const int)));
	connect(m_stormparamsDialog, SIGNAL(filterFilenameChanged(const QString&)), m_model, SLOT(setFilterFilename(const QString&)));
}

void MainController::startStormDialog()
{
	m_stormparamsDialog->show();
}

void MainController::showAboutDialog()
{
	QMessageBox::about(m_view, "About simple storm", 
	"This is only a simple frontend for the storm command line utility. \n" 
	"(c) 2011 Joachim Schleicher");
}

void MainController::runStorm()
{
	if(!m_model->initStorm()) { // open files...
		qDebug()<< "error starting storm. STOP.";
		return;
	}

	int numFrames = m_model->numFrames();
	int chunksize = 50;
	int numChunks = numFrames/chunksize;

	QProgressDialog progress("Processing storm data...", "Abort", 0, numChunks, m_view);
	progress.setWindowModality(Qt::WindowModal);

	for (int i = 0; i < numChunks; i++) {
		progress.setValue(i);

		if (progress.wasCanceled()) {
			qDebug() << "aborted storm.";
			m_model->abortStorm(); // close files
			break;
		}
		m_model->executeStormImages(i*chunksize,(i+1)*chunksize);
	}
	m_model->finishStorm(); // save results
	progress.setValue(m_model->numFrames());

}
