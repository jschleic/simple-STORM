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
 
#include <QtCore>
#include "wienerStorm.hxx"
#include "mainwindow.h"
#include "maincontroller.h"
#include "mainview.h"
#include "stormparamsdialog.h"
#include "stormmodel.h"
#include "stormprocessor.h"
#include <qdebug.h>
#include <QMessageBox>
#include <QProgressDialog>
#include <set>

#include "fftfilter.h"
#include "myimportinfo.h"

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
	MyImportInfo* info = storm::initStorm(m_model); // open files...
	if(info == NULL) { 
		qDebug()<< "error starting storm. STOP.";
		return;
	}

	int numFrames = info->shape()[2];
	//~ int numFrames=1000;

	QProgressDialog progressDialog("Processing storm data...", "Abort", 0, numFrames, m_view);
	progressDialog.setWindowModality(Qt::WindowModal);
	QFutureWatcher<void> futureWatcher;
	connect(&futureWatcher, SIGNAL(finished()), &progressDialog, SLOT(reset()));
	connect(&progressDialog, SIGNAL(canceled()), &futureWatcher, SLOT(cancel()));
	connect(&futureWatcher, SIGNAL(progressRangeChanged(int,int)), &progressDialog, SLOT(setRange(int,int)));
	connect(&futureWatcher, SIGNAL(progressValueChanged(int)), &progressDialog, SLOT(setValue(int)));

	QList<int> range;
	for(int i = 0; i < numFrames; ++i) {
		range.append(i);
	}
	FFTFilter* fftwWrapper = storm::createFFTFilter(info);
	QFuture<std::set<Coord<float> > > result = QtConcurrent::mapped(range, StormProcessor<float>(info, m_model, fftwWrapper));
	futureWatcher.setFuture(result);

	progressDialog.exec();

	storm::saveResults(m_model, info->shape(), QVector<std::set<Coord<T> > >::fromList(result.results()).toStdVector()); // save results // TODO
	delete fftwWrapper;
	delete info;

}
