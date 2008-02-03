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

#include "Import.h"
#include <kfiledialog.h>
#include <qlabel.h>
#include <QHBoxLayout>
#include <Q3ValueList>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QProgressDialog>
#include <klocale.h>
#include <qpushbutton.h>
#include <qdom.h>
#include <qfile.h>
#include <kmessagebox.h>
#include <kzip.h>
#include <karchive.h>
#include <qlayout.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include "Settings/SettingsData.h"
#include "ImportMatcher.h"
#include <qcheckbox.h>
#include "Utilities/Util.h"
#include "DB/ImageDB.h"
#include <qimage.h>
#include "Browser/BrowserWidget.h"
#include <kstandarddirs.h>
#include <kurl.h>
#include <q3progressdialog.h>
#include <kio/netaccess.h>
#include <kio/jobuidelegate.h>
#include "MainWindow/Window.h"
#include <kapplication.h>
#include <ktoolinvocation.h>
#include "DB/CategoryCollection.h"
#include "DB/ImageInfo.h"
#include "MiniViewer.h"
#include "XMLDB/Database.h"
#include <kdebug.h>
#include <QComboBox>
#include <KTemporaryFile>
#include <QScrollArea>
#include <KMessageBox>

using Utilities::StringSet;

class KPushButton;
using namespace ImportExport;

void Import::imageImport()
{
    KUrl url = KFileDialog::getOpenUrl( KUrl(), QString::fromLatin1( "*.kim|KPhotoAlbum Export Files" ) );
    if ( url.isEmpty() )
        return;
    imageImport( url );
}

void Import::imageImport( const KUrl& url )
{
    bool ok;
    if ( !url.isLocalFile() ) {
        new Import( url, 0 );
        // The dialog will start the download, and in the end show itself
    }
    else {
        Import* dialog = new Import( url.path(), &ok, 0 );
        dialog->resize( 800, 600 );
        if ( ok )
            dialog->show();
        else
            delete dialog;
    }
}

Import::Import( const KUrl& url, QWidget* parent )
    :KAssistantDialog( parent ), _zip( 0 ), _hasFilled( false ), _reportUnreadableFiles( true )
{
    _kimFile = url;
    _tmp = new KTemporaryFile;
    _tmp->setSuffix(QString::fromLatin1(".kim"));
    QString path = _tmp->fileName();
    _tmp->setAutoRemove( true );

    KUrl dest;
    dest.setPath( path );
    KIO::FileCopyJob* job = KIO::file_copy( url, dest, -1, KIO::Overwrite );
    connect( job, SIGNAL( result( KIO::Job* ) ), this, SLOT( downloadKimJobCompleted( KIO::Job* ) ) );
}

void Import::downloadKimJobCompleted( KIO::Job* job )
{
    if ( !job->error() ) {
        resize( 800, 600 );
        init( _tmp->fileName() );
        show();
    }
    else {
        job->ui()->showErrorMessage();
        delete this;
    }
}

Import::Import( const QString& fileName, bool* ok, QWidget* parent )
    :KAssistantDialog( parent ), _zipFile( fileName ), _tmp(0), _hasFilled( false )
{
    _kimFile.setPath( fileName );
    *ok = init( fileName );
    connect( this, SIGNAL( failedToCopy( QStringList ) ), this, SLOT( aCopyFailed( QStringList ) ) );
}

bool Import::init( const QString& fileName )
{
    _finishedPressed = false;
    _zip = new KZip( fileName );
    if ( !_zip->open( QIODevice::ReadOnly ) ) {
        KMessageBox::error( this, i18n("Unable to open '%1' for reading.", fileName ), i18n("Error Importing Data") );
        _zip =0;
        return false;
    }
    _dir = _zip->directory();
    if ( _dir == 0 ) {
        KMessageBox::error( this, i18n( "Error reading directory contents of file %1; it is likely that the file is broken." , fileName ) );
        return false;
    }

    const KArchiveEntry* indexxml = _dir->entry( QString::fromLatin1( "index.xml" ) );
    if ( indexxml == 0 || ! indexxml->isFile() ) {
        KMessageBox::error( this, i18n( "Error reading index.xml file from %1; it is likely that the file is broken." , fileName ) );
        return false;
    }

    const KArchiveFile* file = static_cast<const KArchiveFile*>( indexxml );
    QByteArray data = file->data();

    bool ok = readFile( data, fileName );
    if ( !ok )
        return false;

    setupPages();
    return true;
}

Import::~Import()
{
    delete _zip;
    delete _tmp;
}

bool Import::readFile( const QByteArray& data, const QString& fileName )
{
    QDomDocument doc;
    QString errMsg;
    int errLine;
    int errCol;

    if ( !doc.setContent( data, false, &errMsg, &errLine, &errCol )) {
        KMessageBox::error( this, i18n( "Error in file %1 on line %2 col %3: %4" ,fileName,errLine,errCol,errMsg) );
        return false;
    }

    QDomElement top = doc.documentElement();
    if ( top.tagName().toLower() != QString::fromLatin1( "kimdaba-export" ) &&
        top.tagName().toLower() != QString::fromLatin1( "kphotoalbum-export" ) ) {
        KMessageBox::error( this, i18n("Unexpected top element while reading file %1. Expected KPhotoAlbum-export found %2",
                            fileName ,top.tagName() ) );
        return false;
    }

    // Read source
    QString source = top.attribute( QString::fromLatin1( "location" ) ).toLower();
    if ( source != QString::fromLatin1( "inline" ) && source != QString::fromLatin1( "external" ) ) {
        KMessageBox::error( this, i18n("<p>XML file did not specify the source of the images, "
                                       "this is a strong indication that the file is corrupted</p>" ) );
        return false;
    }

    _externalSource = ( source == QString::fromLatin1( "external" ) );

    // Read base url
    _baseUrl = top.attribute( QString::fromLatin1( "baseurl" ) );

    for ( QDomNode node = top.firstChild(); !node.isNull(); node = node.nextSibling() ) {
        if ( !node.isElement() || ! (node.toElement().tagName().toLower() == QString::fromLatin1( "image" ) ) ) {
            KMessageBox::error( this, i18n("Unknown element while reading %1, expected image.", fileName ) );
            return false;
        }
        QDomElement elm = node.toElement();

        DB::ImageInfoPtr info = XMLDB::Database::createImageInfo( elm.attribute( QString::fromLatin1( "file" ) ), elm );
        _images.append( info );
    }

    return true;
}

void Import::setupPages()
{
    createIntroduction();
    createImagesPage();
    createDestination();
    createCategoryPages();
    connect( this, SIGNAL( selected( const QString& ) ), this, SLOT( updateNextButtonState() ) );
    connect( this, SIGNAL( user1Clicked() ), this, SLOT( slotFinish() ) );
    connect( this, SIGNAL( helpClicked() ), this, SLOT( slotHelp() ) );
}

void Import::createIntroduction()
{
    QString txt = i18n( "<h1><font size=\"+2\">Welcome to KPhotoAlbum Import</font></h1>"
                        "This wizard will take you through the steps of an import operation. The steps are: "
                        "<ol><li>First you must select which images you want to import from the export file. "
                        "You do so by selecting the checkbox next to the image.</li>"
                        "<li>Next you must tell KPhotoAlbum in which directory to put the images. This directory must "
                        "of course be below the directory root KPhotoAlbum uses for images. "
                        "KPhotoAlbum will take care to avoid name clashes</li>"
                        "<li>The next step is to specify which categories you want to import (People, Places, ... ) "
                        "and also tell KPhotoAlbum how to match the categories from the file to your categories. "
                        "Imagine you load from a file, where a category is called <b>Blomst</b> (which is the "
                        "Danish word for flower), then you would likely want to match this with your category, which might be "
                        "called <b>Blume</b> (which is the German word for flower) - of course given you are German.</li>"
                        "<li>The final steps, is matching the individual tokens from the categories. I may call myself <b>Jesper</b> "
                        "in my image database, while you want to call me by my full name, namely <b>Jesper K. Pedersen</b>. "
                        "In this step non matches will be highlighted in red, so you can see which tokens was not found in your "
                        "database, or which tokens was only a partial match.</li></ol>");

    QLabel* intro = new QLabel( txt, this );
    intro->setWordWrap(true);
    addPage( intro, i18n("Introduction") );
}

void Import::createImagesPage()
{
    QScrollArea* top = new QScrollArea;
    top->setWidgetResizable(true);

    QWidget* container = new QWidget;
    QVBoxLayout* lay1 = new QVBoxLayout( container );
    top->setWidget( container );

    // Select all and Deselect All buttons
    QHBoxLayout* lay2 = new QHBoxLayout;
    lay1->addLayout(lay2);

    QPushButton* selectAll = new QPushButton( i18n("Select All"), container );
    lay2->addWidget( selectAll );
    QPushButton* selectNone = new QPushButton( i18n("Deselect All"), container );
    lay2->addWidget( selectNone );
    lay2->addStretch( 1 );
    connect( selectAll, SIGNAL( clicked() ), this, SLOT( slotSelectAll() ) );
    connect( selectNone, SIGNAL( clicked() ), this, SLOT( slotSelectNone() ) );

    QGridLayout* lay3 = new QGridLayout;
    lay1->addLayout( lay3 );

    lay3->setColumnStretch( 2, 1 );

    int row = 0;
    for( DB::ImageInfoListConstIterator it = _images.constBegin(); it != _images.constEnd(); ++it, ++row ) {
        DB::ImageInfoPtr info = *it;
        ImageRow* ir = new ImageRow( info, this, container );
        lay3->addWidget( ir->_checkbox, row, 0 );

        const KArchiveEntry* thumbnails = _dir->entry( QString::fromLatin1( "Thumbnails" ) );
        if ( thumbnails ) {
            QPushButton* but = new QPushButton( container );
            const QPixmap pixmap = loadThumbnail( info->fileName( true ) );
            but->setIcon( pixmap );
            but->setIconSize( pixmap.size() );
            lay3->addWidget( but, row, 1 );
            connect( but, SIGNAL( clicked() ), ir, SLOT( showImage() ) );
        }
        else {
            QLabel* label = new QLabel( info->label() );
            lay3->addWidget( label, row, 1 );
        }

        QLabel* label = new QLabel( QString::fromLatin1("<p>%1</p>").arg(info->description()) );
        lay3->addWidget( label, row, 2 );
        _imagesSelect.append( ir );
    }

    addPage( top, i18n("Select Which Images to Import") );
}

ImageRow::ImageRow( DB::ImageInfoPtr info, Import* import, QWidget* parent )
    : QObject( parent ), _info( info ), _import( import )
{
    _checkbox = new QCheckBox( QString::null, parent );
    _checkbox->setChecked( true );
}

void ImageRow::showImage()
{
    if ( _import->_externalSource ) {
        KUrl src1 =_import->_kimFile;
        KUrl src2 = _import->_baseUrl + QString::fromLatin1( "/" );
        for ( int i = 0; i < 2; ++i ) {
            // First try next to the .kim file, then the external URL
            KUrl src = src1;
            if ( i == 1 )
                src = src2;
            src.setFileName( _info->fileName( true ) );
            QString tmpFile;

            if( KIO::NetAccess::download( src, tmpFile, MainWindow::Window::theMainWindow() ) ) {
                QImage img( tmpFile );
                MiniViewer::show( img, _info, static_cast<QWidget*>( parent() ) );
                KIO::NetAccess::removeTempFile( tmpFile );
                break;
            }
        }
    }
    else {
        QImage img = QImage::fromData(_import->loadImage( _info->fileName(true) ) );
        MiniViewer::show( img, _info, static_cast<QWidget*>( parent() ) );
    }
}

void Import::createDestination()
{
    QWidget* top = new QWidget( this );
    QVBoxLayout* topLay = new QVBoxLayout( top );
    QHBoxLayout* lay = new QHBoxLayout;
    topLay->addLayout(lay);

    topLay->addStretch( 1 );

    QLabel* label = new QLabel( i18n( "Destination of images: " ), top );
    lay->addWidget( label );

    _destinationEdit = new KLineEdit( top );
    lay->addWidget( _destinationEdit, 1 );

    KPushButton* but = new KPushButton( QString::fromLatin1("..." ), top );
    but->setFixedWidth( 30 );
    lay->addWidget( but );


    _destinationEdit->setText( Settings::SettingsData::instance()->imageDirectory());
    connect( but, SIGNAL( clicked() ), this, SLOT( slotEditDestination() ) );
    connect( _destinationEdit, SIGNAL( textChanged( const QString& ) ), this, SLOT( updateNextButtonState() ) );
    _destinationPage = addPage( top, i18n("Destination of Images" ) );
}

void  Import::slotEditDestination()
{
    QString file = KFileDialog::getExistingDirectory( _destinationEdit->text(), this );
    if ( !file.isNull() ) {
        if ( ! QFileInfo(file).absoluteFilePath().startsWith( QFileInfo(Settings::SettingsData::instance()->imageDirectory()).absoluteFilePath()) ) {
            KMessageBox::error( this, i18n("The directory must be a subdirectory of %1", Settings::SettingsData::instance()->imageDirectory() ) );
        }
        else {
            _destinationEdit->setText( file );
            updateNextButtonState();
        }
    }
}

void Import::updateNextButtonState()
{
    bool enabled = true;
    if ( currentPage() == _destinationPage ) {
        QString dest = _destinationEdit->text();
        if ( QFileInfo( dest ).isFile() )
            enabled = false;
        else if ( ! QFileInfo(dest).absoluteFilePath().startsWith( QFileInfo(Settings::SettingsData::instance()->imageDirectory()).absoluteFilePath()) )
            enabled = false;
    }

    setValid( currentPage(), enabled );
}

void Import::createCategoryPages()
{
    QStringList categories;
    DB::ImageInfoList images = selectedImages();
    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        DB::ImageInfoPtr info = *it;
        QStringList categoriesForImage = info->availableCategories();
        for( QStringList::Iterator categoryIt = categoriesForImage.begin(); categoryIt != categoriesForImage.end(); ++categoryIt ) {
            if ( !categories.contains( *categoryIt ) &&
                 (*categoryIt) != QString::fromLatin1( "Folder" ) &&
                 (*categoryIt) != QString::fromLatin1( "Tokens" ) )
                categories.append( *categoryIt );
        }
    }

    _categoryMatcher = new ImportMatcher( QString::null, QString::null, categories, DB::ImageDB::instance()->categoryCollection()->categoryNames(),
                                          false, this, "import matcher" );
    _categoryMatcherPage = addPage( _categoryMatcher, i18n("Match Categories") );

    QWidget* dummy = new QWidget;
    _dummy = addPage( dummy, QString::null );
}

ImportMatcher* Import::createCategoryPage( const QString& myCategory, const QString& otherCategory )
{
    StringSet otherItems;
    DB::ImageInfoList images = selectedImages();
    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        otherItems += (*it)->itemsOfCategory( otherCategory );
    }

    QStringList myItems = DB::ImageDB::instance()->categoryCollection()->categoryForName( myCategory )->itemsInclCategories();
    myItems.sort();

    ImportMatcher* matcher = new ImportMatcher( otherCategory, myCategory, otherItems.toList(), myItems, true, this, "import matcher" );
    addPage( matcher, myCategory );
    return matcher;
}

void Import::next()
{
    if ( currentPage() == _destinationPage ) {
        QString dir = _destinationEdit->text();
        if ( !QFileInfo( dir ).exists() ) {
            int answer = KMessageBox::questionYesNo( this, i18n("Directory %1 does not exists. Should it be created?", dir ) );
            if ( answer == KMessageBox::Yes ) {
                bool ok = KStandardDirs::makeDir( dir );
                if ( !ok ) {
                    KMessageBox::error( this, i18n("Error creating directory %1", dir ) );
                    return;
                }
            }
            else
                return;
        }
    }
    if ( !_hasFilled && currentPage() == _categoryMatcherPage ) {
        _hasFilled = true;
        _categoryMatcher->setEnabled( false );
        removePage(_dummy);

        ImportMatcher* matcher = 0;
        for( Q3ValueList<CategoryMatch*>::Iterator it = _categoryMatcher->_matchers.begin();
             it != _categoryMatcher->_matchers.end();
             ++it )
        {
            CategoryMatch* match = *it;
            if ( match->_checkbox->isChecked() ) {
                matcher = createCategoryPage( match->_combobox->currentText(), match->_text );
                _matchers.append( matcher );
            }
        }
    }

    KAssistantDialog::next();
}

bool Import::copyFilesFromZipFile()
{
    DB::ImageInfoList images = selectedImages();

    _totalCopied = 0;
    _progress = new QProgressDialog( i18n("Copying Images"), i18n("&Cancel"), 0,2 * images.count(), this );
    _progress->setValue( 0 );
    _progress->show();

    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        QString fileName = (*it)->fileName( true );
        QByteArray data = loadImage( fileName );
        if ( data.isNull() )
            return false;
        QString newName = Settings::SettingsData::instance()->imageDirectory() + _nameMap[fileName];

        QString relativeName = newName.mid( Settings::SettingsData::instance()->imageDirectory().length() );
        if ( relativeName.startsWith( QString::fromLatin1( "/" ) ) )
            relativeName= relativeName.mid(1);

        QFile out( newName );
        if ( !out.open( QIODevice::WriteOnly ) ) {
            KMessageBox::error( this, i18n("Error when writing image %1", newName ) );
            delete _progress;
            _progress = 0;
            return false;
        }
        out.write( data, data.size() );
        out.close();

        qApp->processEvents();
        _progress->setValue( ++_totalCopied );
        if ( _progress->wasCanceled() ) {
            delete _progress;
            _progress = 0;
            return false;
        }
    }
    return true;
}

void Import::copyFromExternal()
{
    _pendingCopies = selectedImages();
    _totalCopied = 0;
    _progress = new QProgressDialog( i18n("Copying Images"), i18n("&Cancel"), 0,2 * _pendingCopies.count(), this );
    _progress->setValue( 0 );
    _progress->show();
    connect( _progress, SIGNAL( canceled() ), this, SLOT( stopCopyingImages() ) );
    copyNextFromExternal();
}

void Import::copyNextFromExternal()
{
    DB::ImageInfoPtr info = _pendingCopies[0];
    _pendingCopies.pop_front();
    QString fileName = info->fileName( true );
    KUrl src1 = _kimFile;
    KUrl src2 = _baseUrl + QString::fromLatin1( "/" );
    bool succeeded = false;
    QStringList tried;

    for ( int i = 0; i < 2; ++i ) {
        KUrl src = src1;
        if ( i == 1 )
            src = src2;

        src.setFileName( fileName );
        if ( KIO::NetAccess::exists( src, KIO::NetAccess::SourceSide, MainWindow::Window::theMainWindow() ) ) {
            KUrl dest;
            dest.setPath( Settings::SettingsData::instance()->imageDirectory() + _nameMap[fileName] );
            _job = KIO::file_copy( src, dest, -1, KIO::HideProgressInfo );
            connect( _job, SIGNAL( result( KIO::Job* ) ), this, SLOT( aCopyJobCompleted( KIO::Job* ) ) );
            succeeded = true;
            break;
        } else
            tried << src.prettyUrl();
    }

    if (!succeeded)
        emit failedToCopy( tried );
}

void Import::aCopyFailed( QStringList files )
{
    int result = _reportUnreadableFiles ?
        KMessageBox::warningYesNoCancelList( _progress,
            i18n("Can't copy file from any of the following locations:"),
            files, QString::null, KStandardGuiItem::cont(), KGuiItem( i18n("Continue without Asking") )) : KMessageBox::Yes;

    switch (result) {
        case KMessageBox::Cancel:
            // This might be late -- if we managed to copy some files, we will
            // just throw away any changes to the DB, but some new image files
            // might be in the image directory...
            deleteLater();
            delete _progress;
            _progress = 0;
            break;

        case KMessageBox::No:
            _reportUnreadableFiles = false;
            // fall through
        default:
            aCopyJobCompleted( 0 );
    }
}

void Import::aCopyJobCompleted( KIO::Job* job )
{
    if ( job && job->error() ) {
        job->ui()->showErrorMessage();
        deleteLater();
        delete _progress;
    }
    else if ( _pendingCopies.count() == 0 ) {
        updateDB();
        deleteLater();
        delete _progress;
    }
    else if ( _progress->wasCanceled() ) {
        deleteLater();
        delete _progress;
    }
    else {
        _progress->setValue( ++_totalCopied );
        copyNextFromExternal();
    }
}

void Import::stopCopyingImages()
{
    _job->kill();
}

void Import::slotFinish()
{
    _finishedPressed = true;
    _nameMap = Utilities::createUniqNameMap( Utilities::infoListToStringList(selectedImages()), true, _destinationEdit->text() );
    bool ok;
    if ( _externalSource ) {
        hide();
        copyFromExternal();
    }
    else {
        ok = copyFilesFromZipFile();
        if ( ok )
            updateDB();
        deleteLater();
    }
}

void Import::updateDB()
{
    disconnect( _progress, SIGNAL( canceled() ), this, SLOT( stopCopyingImages() ) );
    _progress->setLabelText( i18n("Updating Database") );

    // Run though all images
    DB::ImageInfoList images = selectedImages();
    for( DB::ImageInfoListConstIterator it = images.constBegin(); it != images.constEnd(); ++it ) {
        DB::ImageInfoPtr info = *it;

        DB::ImageInfoPtr newInfo( new DB::ImageInfo( _nameMap[info->fileName(true)] ) );
        newInfo->setLabel( info->label() );
        newInfo->setDescription( info->description() );
        newInfo->setDate( info->date() );
        newInfo->rotate( info->angle() );
        newInfo->setMD5Sum( Utilities::MD5Sum( newInfo->fileName(false) ) );
        DB::ImageInfoList list;
        list.append(newInfo);
        DB::ImageDB::instance()->addImages( list );

        // Run though the categories
        for( Q3ValueList<ImportMatcher*>::Iterator grpIt = _matchers.begin(); grpIt != _matchers.end(); ++grpIt ) {
            QString otherGrp = (*grpIt)->_otherCategory;
            QString myGrp = (*grpIt)->_myCategory;

            // Run through each option
            Q3ValueList<CategoryMatch*>& matcher = (*grpIt)->_matchers;
            for( Q3ValueList<CategoryMatch*>::Iterator optionIt = matcher.begin(); optionIt != matcher.end(); ++optionIt ) {
                if ( !(*optionIt)->_checkbox->isChecked() )
                    continue;
                QString otherOption = (*optionIt)->_text;
                QString myOption = (*optionIt)->_combobox->currentText();

                if ( info->hasCategoryInfo( otherGrp, otherOption ) ) {
                    newInfo->addCategoryInfo( myGrp, myOption );
                    DB::ImageDB::instance()->categoryCollection()->categoryForName( myGrp )->addItem( myOption );
                }

            }
        }

        _progress->setValue( ++_totalCopied );
        if ( _progress->wasCanceled() )
            break;
    }
    Browser::BrowserWidget::instance()->home();
}

QPixmap Import::loadThumbnail( QString fileName )
{
    const KArchiveEntry* thumbnails = _dir->entry( QString::fromLatin1( "Thumbnails" ) );
    Q_ASSERT( thumbnails ); // We already tested for this.

    if ( !thumbnails->isDirectory() ) {
        KMessageBox::error( this, i18n("Thumbnail item in export file was not a directory, this indicates that the file is broken.") );
        return QPixmap();
    }

    const KArchiveDirectory* thumbnailDir = static_cast<const KArchiveDirectory*>( thumbnails );

    const QString ext = Utilities::isVideo( fileName ) ? QString::fromLatin1( "jpg" ) : QFileInfo( fileName ).completeSuffix();
    fileName = QString::fromLatin1("%1.%2").arg( Utilities::stripSlash( QFileInfo( fileName ).baseName() ) ).arg(ext);
    const KArchiveEntry* fileEntry = thumbnailDir->entry( fileName );
    if ( fileEntry == 0 || !fileEntry->isFile() ) {
        KMessageBox::error( this, i18n("No thumbnail existed in export file for %1", fileName ) );
        return QPixmap();
    }

    const KArchiveFile* file = static_cast<const KArchiveFile*>( fileEntry );
    QByteArray data = file->data();
    QPixmap pixmap;
    pixmap.loadFromData( data );
    return pixmap;
}

void Import::slotSelectAll()
{
    selectImage( true );
}

void Import::slotSelectNone()
{
    selectImage( false );
}

void Import::selectImage( bool on )
{
    for( Q3ValueList<ImageRow*>::Iterator it = _imagesSelect.begin(); it != _imagesSelect.end(); ++it ) {
        (*it)->_checkbox->setChecked( on );
    }
}

QByteArray Import::loadImage( const QString& fileName )
{
    const KArchiveEntry* images = _dir->entry( QString::fromLatin1( "Images" ) );
    if ( !images ) {
        KMessageBox::error( this, i18n("export file did not contain a Images subdirectory, this indicates that the file is broken") );
        return QByteArray();
    }

    if ( !images->isDirectory() ) {
        KMessageBox::error( this, i18n("Images item in export file was not a directory, this indicates that the file is broken") );
        return QByteArray();
    }

    const KArchiveDirectory* imagesDir = static_cast<const KArchiveDirectory*>( images );

    const KArchiveEntry* fileEntry = imagesDir->entry( fileName );
    if ( fileEntry == 0 || !fileEntry->isFile() ) {
        KMessageBox::error( this, i18n("No image existed in export file for %1", fileName ) );
        return QByteArray();
    }

    const KArchiveFile* file = static_cast<const KArchiveFile*>( fileEntry );
    QByteArray data = file->data();
    return data;
}

DB::ImageInfoList Import::selectedImages()
{
    DB::ImageInfoList res;
    for( Q3ValueList<ImageRow*>::Iterator it = _imagesSelect.begin(); it != _imagesSelect.end(); ++it ) {
        if ( (*it)->_checkbox->isChecked() )
            res.append( (*it)->_info );
    }
    return res;
}

void Import::closeEvent( QCloseEvent* e )
{
    // If the user presses the finish button, then we have to postpone the delete operations, as we have pending copies.
    if ( !_finishedPressed )
        deleteLater();
    KAssistantDialog::closeEvent( e );
}



void Import::slotHelp()
{
    KToolInvocation::invokeHelp( QString::fromLatin1( "kphotoalbum#chp-exportDialog" ) );
}

#include "Import.moc"
