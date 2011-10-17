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
#include "myimportinfo.h"
#include "mainwindow.h"
#include "maincontroller.h"
#include "mainview.h"
#include "stormparamsdialog.h"
#include "settingsdialog.h"
#include "stormmodel.h"
#include "stormprocessor.h"
#include "config.h"
#include "previewtimer.h"
#include <qdebug.h>
#include <QMessageBox>
#include <QProgressDialog>
#include <set>

#include "fftfilter.h"
#include "myimportinfo.h"
#include <vigra/timing.hxx>
#include "configVersion.hxx"
#include "version.h"

MainController::MainController(MainWindow * window) 
	: QObject(window), 
	m_view(window->mainview()),
	m_stormparamsDialog(new Stormparamsdialog(window)),
	m_model(new StormModel(this))
{
	connectSignals(window);

	if(Config::filterFilename()=="") {
		QMessageBox::warning(m_view, "No filter selected", 
			"Please first select a filter file that is used by default to preprocess the measurements");
		showSettingsDialog();
	}

	// default values
	m_model->setThreshold(m_stormparamsDialog->threshold());
	m_model->setFactor(m_stormparamsDialog->factor());
	m_model->setFilterFilename(Config::filterFilename());
	m_model->setPreviewEnabled(m_stormparamsDialog->previewEnabled());

	showStormparamsDialog();
}

MainController::~MainController() 
{

}

void MainController::connectSignals(MainWindow* window)
{
	connect(window, SIGNAL(action_showAboutDialog_triggered()), SLOT(showAboutDialog()));
	connect(window, SIGNAL(action_showStormparamsDialog_triggered()), SLOT(showStormparamsDialog()));
	connect(window, SIGNAL(action_showSettingsDialog_triggered()), SLOT(showSettingsDialog()));

	connect(m_stormparamsDialog, SIGNAL(accepted()), this, SLOT(runStorm()));
	connect(m_stormparamsDialog, SIGNAL(inputFilenameChanged(const QString&)), m_model, SLOT(setInputFilename(const QString&)));
	connect(m_stormparamsDialog, SIGNAL(factorChanged(const int)), m_model, SLOT(setFactor(const int)));
	connect(m_stormparamsDialog, SIGNAL(thresholdChanged(const int)), m_model, SLOT(setThreshold(const int)));
	connect(m_stormparamsDialog, SIGNAL(previewEnabled(const bool)), m_model, SLOT(setPreviewEnabled(const bool)));
}

void MainController::showStormparamsDialog()
{
	m_stormparamsDialog->show();
}

void MainController::showAboutDialog()
{
	QMessageBox::about(m_view, "About simple storm", 
		QString("This is a simple frontend for the storm command line utility. \n" 
		"(c) 2011 Joachim Schleicher\n"
		"GUI Version %1, STORM Version %2").arg(STORMGUI_VERSION_STRING).arg(STORM_VERSION_STRING));
}

void MainController::showSettingsDialog()
{
	SettingsDialog* settings = new SettingsDialog(m_view);
	settings->setFilterFilename(Config::filterFilename());
	int result = settings->exec();
	if(result==QDialog::Accepted) {
		Config::setFilterFilename(settings->filterFilename());
		m_model->setFilterFilename(settings->filterFilename());
	}
	return;
}

void MainController::runStorm()
{
	MyImportInfo* info;
	try {
		info = new MyImportInfo(m_model->inputFilename().toStdString()); // open file
	} catch (vigra::StdException & e) {
		QMessageBox::warning(m_view, "Unable to open file", QString("The file %1 could not be opened").arg(m_model->inputFilename()));
		return;
	}

	int numFrames = info->shape()[2];
	USETICTOC;

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

	PreviewImage previewImage(m_model, info->shape(), result);
	PreviewTimer previewTimer(&previewImage);
	connect(&previewTimer, SIGNAL(previewChanged(QImage*)), m_view, SLOT(setPreview(QImage*)));
	if(m_model->previewEnabled()) {
		previewTimer.start(1000);
	}

	TIC;
	progressDialog.exec();
	futureWatcher.waitForFinished();
	previewTimer.stop();
	TOC;

	storm::saveResults(m_model, info->shape(), QVector<std::set<Coord<T> > >::fromList(result.results()).toStdVector()); // save results // TODO
	m_view->setPreview(previewImage.getPreviewImage());
	delete fftwWrapper;
	delete info;

}
