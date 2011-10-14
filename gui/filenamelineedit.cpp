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

#include "filenamelineedit.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QList>
#include <QString>
#include <QFileInfo>

FilenameLineEdit::FilenameLineEdit(QWidget * parent)
	: QLineEdit(parent)
{
	setAcceptDrops(true);
}

FilenameLineEdit::~FilenameLineEdit()
{
	
}

void FilenameLineEdit::dragEnterEvent(QDragEnterEvent* event)
{
    // accept just text/uri-list mime format
    if (event->mimeData()->hasFormat("text/uri-list")) 
    {     
        event->acceptProposedAction();
    }
}

void FilenameLineEdit::dropEvent(QDropEvent* event)
{
	QList<QUrl> urlList;
	QString fName;
	QFileInfo info;

	if (event->mimeData()->hasUrls())
	{
	urlList = event->mimeData()->urls(); // returns list of QUrls

	// if just text was dropped, urlList is empty (size == 0)
		if ( urlList.size() > 0) // if at least one QUrl is present in list
		{
			fName = urlList[0].toLocalFile(); // convert first QUrl to local path
			info.setFile( fName ); // information about file
			if ( info.isFile() ) setText( fName ); // if is file, setText
		}
	}
	event->acceptProposedAction();
}
