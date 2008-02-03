/* Copyright (C) 2003-2006 Jesper K. Pedersen <blackie@kde.org>

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
#include "NewImageFinder.h"
#include "DB/ImageDB.h"
#include <qfileinfo.h>
#include "Settings/SettingsData.h"
#include "Browser/BrowserWidget.h"
#include "ImageManager/RawImageDecoder.h"
#include <sys/types.h>
#include <dirent.h>
#include "Utilities/Util.h"
#include <QProgressDialog>
#include <klocale.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <kmessagebox.h>
#include <config-kpa-exiv2.h>
#ifdef HAVE_EXIV2
#  include "Exif/Database.h"
#endif
#include "Utilities/Util.h"

using namespace DB;

bool NewImageFinder::findImages()
{
    // Load the information from the XML file.
    Q3Dict<void> loadedFiles( 6301 /* a large prime */ );

    QStringList images = DB::ImageDB::instance()->images();
    for( QStringList::ConstIterator it = images.begin(); it != images.end(); ++it ) {
        loadedFiles.insert( *it, (void*)0x1 /* void pointer to nothing I never need the value,
                                               just its existsance, must be != 0x0 though.*/ );
    }

    _pendingLoad.clear();
    searchForNewFiles( loadedFiles, Settings::SettingsData::instance()->imageDirectory() );
    loadExtraFiles();

    // To avoid deciding if the new images are shown in a given thumbnail view or in a given search
    // we rather just go to home.
    Browser::BrowserWidget::instance()->home();
    return (!_pendingLoad.isEmpty()); // returns if new images was found.
}

// FastDir is used in place of QDir because QDir stat()s every file in
// the directory, even if we tell it not to restrict anything.  When
// scanning for new images, we don't want to look at files we already
// have in our database, and we also don't want to look at files whose
// names indicate that we don't care about them.  So what we do is
// simply read the names from the directory and let the higher layers
// decide what to do with them.
//
// On my sample database with ~20,000 images, this improves the time
// to rescan for images on a cold system from about 100 seconds to
// about 3 seconds.
//
// -- Robert Krawitz, rlk@alum.mit.edu 2007-07-22
class FastDir
{
public:
    FastDir(const QString &path);
    QStringList entryList() const;
private:
    const QString _path;
};

FastDir::FastDir(const QString &path)
  : _path(path)
{
}

QStringList FastDir::entryList() const
{
    QStringList answer;
    DIR *dir;
    dirent *file;
    dir = opendir( QFile::encodeName(_path) );
    if ( !dir )
	return answer; // cannot read the directory

#if defined(QT_THREAD_SUPPORT) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
    union dirent_buf {
	struct dirent mt_file;
	char b[sizeof(struct dirent) + MAXNAMLEN + 1];
    } *u = new union dirent_buf;
    while ( readdir_r(dir, &(u->mt_file), &file ) == 0 && file )
#else
    while ( (file = readdir(dir)) )
#endif // QT_THREAD_SUPPORT && _POSIX_THREAD_SAFE_FUNCTIONS
	answer.append(QFile::decodeName(file->d_name));
#if defined(QT_THREAD_SUPPORT) && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && !defined(Q_OS_CYGWIN)
    delete u;
#endif
    (void) closedir(dir);
    return answer;
}

void NewImageFinder::searchForNewFiles( const Q3Dict<void>& loadedFiles, QString directory )
{
    if ( directory.endsWith( QString::fromLatin1("/") ) )
        directory = directory.mid( 0, directory.length()-1 );

    QString imageDir = Settings::SettingsData::instance()->imageDirectory();
    if ( imageDir.endsWith( QString::fromLatin1("/") ) )
        imageDir = imageDir.mid( 0, imageDir.length()-1 );

    FastDir dir( directory );
    QStringList dirList = dir.entryList( );
    ImageManager::RAWImageDecoder dec;
    for( QStringList::Iterator it = dirList.begin(); it != dirList.end(); ++it ) {
        QString file = directory + QString::fromLatin1("/") + *it;
        if ( (*it) == QString::fromLatin1(".") || (*it) == QString::fromLatin1("..") ||
             (*it) == QString::fromLatin1("ThumbNails") ||
             (*it) == QString::fromLatin1("CategoryImages") ||
	         loadedFiles.find( file ) ||
	         dec._skipThisFile(loadedFiles, file) )
            continue;

        QFileInfo fi( file );

	    if ( !fi.isReadable() )
	        continue;

        if ( fi.isFile() ) {
            QString baseName = file.mid( imageDir.length()+1 );
            if ( ! DB::ImageDB::instance()->isBlocking( baseName ) ) {
                if ( Utilities::canReadImage(file) )
                    _pendingLoad.append( qMakePair( baseName, DB::Image ) );
                else if ( Utilities::isVideo( file ) )
                    _pendingLoad.append( qMakePair( baseName, DB::Video ) );
            }
        } else if ( fi.isDir() )  {
            searchForNewFiles( loadedFiles, file );
        }
    }
}

void NewImageFinder::loadExtraFiles()
{
    QProgressDialog dialog;
    dialog.setLabelText( i18n("<p><b>Loading information from new files</b></p>"
                              "<p>Depending on the number of images, this may take some time.<br/>"
                              "However, there is only a delay when new images are found.</p>") );
    dialog.setMaximum( _pendingLoad.count() );

    int count = 0;
    ImageInfoList newImages;
    for( LoadList::Iterator it = _pendingLoad.begin(); it != _pendingLoad.end(); ++it, ++count ) {
        dialog.setValue( count ); // ensure to call setProgress(0)
        qApp->processEvents( QEventLoop::AllEvents );

        if ( dialog.wasCanceled() )
            return;
        ImageInfoPtr info = loadExtraFile( (*it).first, (*it).second );
        if ( info )
            newImages.append(info);
    }
    DB::ImageDB::instance()->addImages( newImages );
}


ImageInfoPtr NewImageFinder::loadExtraFile( const QString& relativeNewFileName, DB::MediaType type )
{
    QString absoluteNewFileName = Utilities::absoluteImageFileName( relativeNewFileName );
    MD5 sum = Utilities::MD5Sum( absoluteNewFileName );
    if ( DB::ImageDB::instance()->md5Map()->contains( sum ) ) {
        QString relativeMatchedFileName = DB::ImageDB::instance()->md5Map()->lookup(sum);
        QString absoluteMatchedFileName = Utilities::absoluteImageFileName( relativeMatchedFileName );
        QFileInfo fi( absoluteMatchedFileName );

        if ( !fi.exists() ) {
            // The file we had a collapse with didn't exists anymore so it is likely moved to this new name
            ImageInfoPtr info = DB::ImageDB::instance()->info( Settings::SettingsData::instance()->imageDirectory() + relativeMatchedFileName );
            if ( !info )
                qWarning("How did that happen? We couldn't find info for the images %s", qPrintable(relativeMatchedFileName));
            else {
                info->delaySavingChanges(true);
                fi = QFileInfo ( relativeMatchedFileName );
                if ( info->label() == fi.completeBaseName() ) {
                    fi = QFileInfo( relativeNewFileName );
                    info->setLabel( fi.completeBaseName() );
                }

                info->setFileName( relativeNewFileName );
                info->delaySavingChanges(false);

                // We need to insert the new name into the MD5 map,
                // as it is a map, the value for the moved file will automatically be deleted.
                DB::ImageDB::instance()->md5Map()->insert( sum, info->fileName(true) );

#ifdef HAVE_EXIV2
                Exif::Database::instance()->remove( absoluteMatchedFileName );
                Exif::Database::instance()->add( absoluteNewFileName );
#endif
                return DB::ImageInfoPtr();
            }
        }
    }

    ImageInfoPtr info = ImageInfoPtr( new ImageInfo( relativeNewFileName, type ) );
    info->setMD5Sum(sum);
    DB::ImageDB::instance()->md5Map()->insert( sum, info->fileName(true) );
#ifdef HAVE_EXIV2
    Exif::Database::instance()->add( absoluteNewFileName );
#endif

    return info;
}

bool  NewImageFinder::calculateMD5sums( const QStringList& list, DB::MD5Map* md5Map, bool* wasCanceled )
{
    QProgressDialog dialog;
    dialog.setLabelText( i18n("<p><b>Calculating checksum for %1 files<b></p>"
                              "<p>By storing a checksum for each image KPhotoAlbum is capable of finding images "
                              "even when you have moved them on the disk.</p>").arg( list.count() ) );
    dialog.setMaximum( list.count() );

    int count = 0;
    QStringList cantRead;
    bool dirty = false;

    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it, ++count ) {
        ImageInfoPtr info = DB::ImageDB::instance()->info( *it );
        if ( count % 10 == 0 ) {
            dialog.setValue( count ); // ensure to call setProgress(0)
            qApp->processEvents( QEventLoop::AllEvents );

            if ( dialog.wasCanceled() ) {
                if ( wasCanceled )
                    *wasCanceled = true;
                return dirty;
            }
        }

        MD5 md5 = Utilities::MD5Sum( *it );
        if (md5.isNull()) {
            cantRead << *it;
            continue;
        }

        if  ( info->MD5Sum() != md5 ) {
            info->setMD5Sum( md5 );
            dirty = true;
            Utilities::removeThumbNail( *it );
        }

        md5Map->insert( md5, info->fileName(true) );
    }
    if ( wasCanceled )
        *wasCanceled = false;

    if ( !cantRead.empty() )
        KMessageBox::informationList( 0, i18n("Following files could not be read:"), cantRead );

    return dirty;
}
