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

#ifndef STORMMODEL_H
#define STORMMODEL_H

#include <QObject>

class StormModel : public QObject
{
	Q_OBJECT
	public:
		StormModel(QObject * parent=0);
		~StormModel();
	public slots:
		bool initStorm(); /**< executes the data processing with its private parameters */
		void abortStorm(); /**< close file pointers, no further processing */
		void finishStorm(); /**< save results and close all file pointers */
		void executeStormImages(const int from, const int to); /**< run storm algorithm */
		int  numFrames(); /**< return the number of frames. initStorm() has to be called first! */
		void setThreshold(const int);
		void setFactor(const int);
		void setInputFilename(const QString&);
		void setFilterFilename(const QString&);
	signals:
		void progress(int);
	private:
		int m_threshold;
		int m_factor;
		QString m_inputFilename;
		QString m_filterFilename;

};

#endif // STORMMODEL_H
