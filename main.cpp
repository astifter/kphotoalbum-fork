/*
 *  Copyright (c) 2003 Jesper K. Pedersen <blackie@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/

#include "thumbnailview.h"
#include "options.h"
#include <qdir.h>
#include "mainview.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kimageio.h>

static const KCmdLineOptions options[] =
{
	{ "c ", I18N_NOOP("Config file"), 0 },
    { "demo", I18N_NOOP( "Starts KimDaBa with a prebuilt set of demo images" ), 0 },
	{ 0, 0, 0}
};

int main( int argc, char** argv ) {
    KAboutData aboutData( "kimdaba", I18N_NOOP("KimDaba"), "0.01",
                          I18N_NOOP("KDE Image Database"), KAboutData::License_GPL );
    aboutData.addAuthor( "Jesper K. Pedersen", I18N_NOOP("Development"), "blackie@kde.org" );

    KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    KImageIO::registerFormats();
    MainView* view = new MainView( 0, "view" );

    qApp->setMainWidget( view );
    view->resize(800, 600);
    view->show();

    return app.exec();
}
