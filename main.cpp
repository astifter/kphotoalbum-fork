/* Copyright (C) 2003-2010 Jesper K. Pedersen <blackie@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifdef KDAB_TEMPORARILY_REMOVED
#include "Threading/Test.h"
#endif //KDAB_TEMPORARILY_REMOVED
#include <QTemporaryFile>
#include "Settings/SettingsData.h"
#include "MainWindow/Window.h"
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kimageio.h>
#include "MainWindow/SplashScreen.h"
#include <klocale.h>
#include <kdebug.h>
#include "RemoteControl/RemoteInterface.h"

#include "version.h"

int main( int argc, char** argv ) {
    KAboutData aboutData( "kphotoalbum", 0, ki18n("KPhotoAlbum"), KPA_VERSION,
                          ki18n("KDE Photo Album"), KAboutData::License_GPL,
                          KLocalizedString(), KLocalizedString(), "http://www.kphotoalbum.org");
    aboutData.addAuthor( ki18n("Jesper K. Pedersen"), ki18n("Development"), "blackie@kde.org" );
    aboutData.addAuthor( ki18n("Hassan Ibraheem"),ki18n("Development"), "hasan.ibraheem@gmail.com");
    aboutData.addAuthor( ki18n("Miika Turkia"),ki18n("Development"), "miika.turkia@gmail.com");
    aboutData.addAuthor( ki18n("Tuomas Suutari"), ki18n("SQL backend and numerous features"), "thsuut@utu.fi" );
    aboutData.addAuthor( ki18n("Jan Kundr&aacute;t"), ki18n("Development"), "jkt@gentoo.org");
    aboutData.addAuthor( ki18n("Henner Zeller"),ki18n("Development"), "h.zeller@acm.org");
    aboutData.addAuthor( ki18n("Andreas Neustifter"),ki18n("Development"), "andreas.neustifter@gmail.com");
    aboutData.addAuthor( ki18n("Johannes Zarl"),ki18n("Development"), "isilmendil@gmx.net");
    aboutData.addAuthor( ki18n("Tobias Leupold"),ki18n("Development"), "tobias.leupold@web.de");


    aboutData.addCredit( ki18n("Will Stephenson"), ki18n("Developing an Icon for KPhotoAlbum"), "will@stevello.free-online.co.uk" );
    aboutData.addCredit( ki18n("Teemu Rytilahti"),
                         ki18n("Sending patches implementing (.) the \"Set As Wallpaper\" menu in the viewer."
                         "(.) Theme support for HTML generation"), "teemu.rytilahti@kde-fi.org" );
    aboutData.addCredit( ki18n("Reimar Imhof"), ki18n("Patch to sort items in option listboxes"), "Reimar.Imhof@netCologne.de" );
    aboutData.addCredit( ki18n("Thomas Schwarzgruber"), ki18n("Patch to sort images in the thumbnail view, plus reading time info out of EXIF images for existing images"), "possebaer@gmx.at" );
    aboutData.addCredit( ki18n("Marcel Wiesweg"), ki18n("Patch which speed up loading of thumbnails plus preview in image property dialog."), "marcel.wiesweg@gmx.de" );
    aboutData.addCredit( ki18n("Marco Caldarelli"), ki18n("Patch for making it possible to reread EXIF info using a nice dialog."), "caldarel@yahoo.it" );
    aboutData.addCredit( ki18n("Jean-Michel FAYARD"), ki18n("(.) Patch with directory info made available through the browser. (.) Patch for adding a check box for \"and/or\" searches in the search page."), "jmfayard@gmail.com" );
    aboutData.addCredit( ki18n("Robert L Krawitz"), ki18n("Numerous patches plus profiling KPhotoAlbum again and again."), "rlk@alum.mit.edu" );
    aboutData.addCredit( ki18n("Christoph Moseler"), ki18n("Numerous patches for lots of bugs plus patches for a few new features"), "forums@moseler.net" );
    aboutData.addCredit( ki18n("Clytie Siddall"), ki18n("Tremendous help with the English text in the application."), "clytie@riverland.net.au" );
    aboutData.addCredit( ki18n("Wes Hardaker"),ki18n("Some very useful features to improve workflow"), "kpa@capturedonearth.com");

    KCmdLineArgs::init( argc, argv, &aboutData );

    KCmdLineOptions options;
    options.add("c ", ki18n("Config file"));
    options.add("demo", ki18n( "Starts KPhotoAlbum with a prebuilt set of demo images" ));
    options.add("import ", ki18n( "Import file" ));
    options.add("nolisten-network", ki18n( "Don't start listening for android devices on startup." ));
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication app;

    new MainWindow::SplashScreen();

    // FIXME: There is no point in using try here, because exceptions
    // and Qt event loop don't mix. Rather exceptions should be
    // caught earlier and not passed through Qt code.
    try {
        MainWindow::Window* view = new MainWindow::Window( 0 );

        // qApp->setMainWidget( view );
        view->setGeometry( Settings::SettingsData::instance()->windowGeometry( Settings::MainWindow ) );

        (void) RemoteControl::RemoteInterface::instance();

        int code = app.exec();
        // I've heard multiple people complain about a crash in this line.
        // unfortunately valgrind doesn't tell me why that should be, and I haven't seen it myself.
        // Anyway, the line is really only needed when searching for memory leaks.
        // delete view;
        return code;
    }
    catch (...) {
        qFatal("Unknown exception caught");
    }
}
// vi:expandtab:tabstop=4 shiftwidth=4:
