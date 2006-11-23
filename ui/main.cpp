/** 
 *  This file is a part of Esperanza, an XMMS2 Client.
 *
 *  Copyright (C) 2005-2006 XMMS2 Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */


#include <xmmsclient/xmmsclient++.h>

#include <QTreeView>
#include <QErrorMessage>
#include <QHeaderView>
#include <QApplication>
#include <QImageReader>
#include <QSettings>
#include <QColor>

#include "xclient.h"
#include "playerwidget.h"
#include "serverdialog.h"
#include "preferences.h"
#include "minimode.h"
#include "mdns.h"

int
main (int argc, char **argv)
{
	QApplication app(argc, argv);

	QCoreApplication::setOrganizationName ("xmms2");
	QCoreApplication::setOrganizationDomain ("xmms.org");
	QCoreApplication::setApplicationName ("Esperanza");

	QApplication::setWindowIcon (QIcon (":images/esperanza.png"));

	PreferenceDialog::save_defaults ();
	
	XClient client (NULL, "Esperanza");
	QSettings s;

	MDNSQuery mdns (NULL);
	mdns.browse_service ("_xmms2._tcp");

	QString path;

	PlayerWidget *pw = new PlayerWidget (NULL, &client);

browser:
	if (!getenv ("XMMS_PATH")) {
		ServerDialog sd (NULL, &mdns);
		if (!s.value ("serverdialog/show").toBool ()) {
			path = sd.get_default ();
		} else {
			path = sd.get_path ();
		}
		if (path == "local") {
			path = "";
		} else if (path.isNull ()) {
			return EXIT_FAILURE;
		}
	} else {
		path = QString::fromAscii (getenv ("XMMS_PATH"));
	}

	if (!client.connect (path.toStdString ())) {
		if (!getenv ("XMMS_PATH")) {
			goto browser;
		} else {
			QErrorMessage *err = new QErrorMessage (NULL);
			err->showMessage ("Your XMMS_PATH enviroment sucks. Fix it and restart the Application");
			err->exec ();
			exit (EXIT_FAILURE);
		}
	}


	pw->show ();

	return app.exec ();
}

