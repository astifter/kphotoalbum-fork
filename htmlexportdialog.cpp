/*
 *  Copyright (c) 2003-2004 Jesper K. Pedersen <blackie@kde.org>
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

#include "kdeversion.h"
#include "htmlexportdialog.h"
#include <klocale.h>
#include <qlayout.h>
#include <klineedit.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qdir.h>
#include <qfile.h>
#include <qapplication.h>
#include <qeventloop.h>
#include "imagemanager.h"
#include <qcheckbox.h>
#include <kfiledialog.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include "options.h"
#include <qprogressdialog.h>
#include <qslider.h>
#include <qlcdnumber.h>
#include <qhgroupbox.h>
#include <kstandarddirs.h>
#include <krun.h>
#include <kio/job.h>
#include <ktempdir.h>
#include <kmessagebox.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kio/jobclasses.h>
#include <qtextedit.h>
#include <qregexp.h>
#include <unistd.h>
#include "util.h"
#include <kdebug.h>
#include <qdir.h>
#include <ksimpleconfig.h>
#include <qvgroupbox.h>
#include <kglobal.h>
#include <kiconloader.h>

class ImageSizeCheckBox :public QCheckBox {

public:
    ImageSizeCheckBox( int width, int height, QWidget* parent )
        :QCheckBox( QString::fromLatin1("%1x%2").arg(width).arg(height), parent ),
         _width( width ), _height( height )
        {
        }

    ImageSizeCheckBox( const QString& text, QWidget* parent )
        :QCheckBox( text, parent ), _width( -1 ), _height( -1 )
        {
        }

    int width() const {
        return _width;
    }
    int height() const {
        return _height;
    }
    QString text( bool withOutSpaces ) const {
        return text( _width, _height, withOutSpaces );
    }
    static QString text( int width, int height, bool withOutSpaces ) {
        if ( width == -1 )
            if ( withOutSpaces )
                return QString::fromLatin1("fullsize");
            else
                return QString::fromLatin1("full size");

        else
            return QString::fromLatin1("%1x%2").arg(width).arg(height);
    }


private:
    int _width;
    int _height;
};

HTMLExportDialog::HTMLExportDialog( QWidget* parent, const char* name )
    :KDialogBase( IconList, i18n("HTML Export"), Ok|Cancel, Ok, parent, name )
{
    enableButtonOK( false );
    createContentPage();
    createLayoutPage();
    createDestinationPage();
}

void HTMLExportDialog::createContentPage()
{
    QWidget* contentPage = addPage( i18n("Content" ), i18n("Content" ),
                                    KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "edit" ),
                                                                     KIcon::Desktop, 32 ));
    QVBoxLayout* lay1 = new QVBoxLayout( contentPage, 6 );
    QGridLayout* lay2 = new QGridLayout( lay1, 2 );

    QLabel* label = new QLabel( i18n("Page title:"), contentPage );
    lay2->addWidget( label, 0, 0 );
    _title = new KLineEdit( contentPage );
    lay2->addWidget( _title, 0, 1 );

    // Description
    label = new QLabel( i18n("Description:"), contentPage );
    label->setAlignment( Qt::AlignTop );
    lay2->addWidget( label, 1, 0 );
    _description = new QTextEdit( contentPage );
    lay2->addWidget( _description, 1, 1 );

    // What to include
    QVGroupBox* whatToInclude = new QVGroupBox( i18n( "What to Include" ), contentPage );
    lay1->addWidget( whatToInclude );
    QWidget* w = new QWidget( whatToInclude );
    QGridLayout* lay3 = new QGridLayout( w, 1, 2, 6 );
    lay3->setAutoAdd( true );

    QStringList optionGroups = Options::instance()->optionGroups();
    for( QStringList::Iterator it = optionGroups.begin(); it != optionGroups.end(); ++it ) {
        QCheckBox* cb = new QCheckBox( *it, w );
        _whatToIncludeMap.insert( *it, cb );
    }
    QCheckBox* cb = new QCheckBox( i18n("Description"), w );
    _whatToIncludeMap.insert( QString::fromLatin1("**DESCRIPTION**"), cb );
}

void HTMLExportDialog::createLayoutPage()
{
    QWidget* layoutPage = addPage( i18n("Layout" ), i18n("Layout" ),
                                   KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "matrix" ),
                                                                    KIcon::Desktop, 32 ));
    QVBoxLayout* lay1 = new QVBoxLayout( layoutPage, 6 );
    QGridLayout* lay2 = new QGridLayout( lay1, 2, 2, 6 );

    // Thumbnail size
    QLabel* label = new QLabel( i18n("Thumbnail size:"), layoutPage );
    lay2->addWidget( label, 0, 0 );

    QHBoxLayout* lay3 = new QHBoxLayout( 0 );
    lay2->addLayout( lay3, 0, 1 );

    _thumbSize = new QSpinBox( 16, 256, 1, layoutPage );
    _thumbSize->setValue( 128 );
    lay3->addWidget( _thumbSize );
    lay3->addStretch(1);

    // Number of columns
    label = new QLabel( i18n("Number of columns:"), layoutPage );
    lay2->addWidget( label, 1, 0 );

    QHBoxLayout* lay4 = new QHBoxLayout( 0 );
    lay2->addLayout( lay4, 1, 1 );
    _numOfCols = new QSpinBox( 1, 10, 1, layoutPage );
    _numOfCols->setValue( 5 );
    lay4->addWidget( _numOfCols );
    lay4->addStretch( 1 );

    // Theme box
    label = new QLabel( i18n("Theme:"), layoutPage );
    lay2->addWidget( label, 2, 0 );
    lay4 = new QHBoxLayout( 0 );
    lay2->addLayout( lay4, 2, 1 );
    _themeBox = new QComboBox( layoutPage, "theme_combobox" );
    lay4->addWidget( _themeBox );
    lay4->addStretch( 1 );
    populateThemesCombo();

    // Image sizes
    QHGroupBox* sizes = new QHGroupBox( i18n("Image Sizes"), layoutPage );
    lay1->addWidget( sizes );
    QWidget* content = new QWidget( sizes );
    QGridLayout* lay5 = new QGridLayout( content, 2, 4 );
    lay5->setAutoAdd( true );
    ImageSizeCheckBox* size320  = new ImageSizeCheckBox( 320, 200, content );
    ImageSizeCheckBox* size640  = new ImageSizeCheckBox( 640, 480, content );
    ImageSizeCheckBox* size800  = new ImageSizeCheckBox( 800, 600, content );
    ImageSizeCheckBox* size1024 = new ImageSizeCheckBox( 1024, 768, content );
    ImageSizeCheckBox* size1280 = new ImageSizeCheckBox( 1280, 1024, content );
    ImageSizeCheckBox* size1600 = new ImageSizeCheckBox( 1600, 1200, content );
    ImageSizeCheckBox* sizeOrig = new ImageSizeCheckBox( i18n("Full size"), content );
    size800->setChecked( 1 );

    _cbs << size320 << size640 << size800 << size1024 << size1280 << size1600 << sizeOrig;
    _preferredSizes << size800 << size1024 << size1280 << size640 << size1600 << size320 << sizeOrig;

    lay1->addStretch(1);
}

void HTMLExportDialog::createDestinationPage()
{
    QWidget* destinationPage = addPage( i18n("Destination" ), i18n("Destination" ),
                                        KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "hdd_unmount" ),
                                                                         KIcon::Desktop, 32 ));
    QVBoxLayout* lay1 = new QVBoxLayout( destinationPage, 6 );
    QGridLayout* lay2 = new QGridLayout( lay1, 2 );

    // Base Directory
    QLabel* label = new QLabel( i18n("Base directory:"), destinationPage );
    lay2->addWidget( label, 5, 0 );

    QHBoxLayout* lay3 = new QHBoxLayout( (QWidget*)0, 0, 6 );
    lay2->addLayout( lay3, 5, 1 );

    _baseDir = new KLineEdit( destinationPage );
    lay3->addWidget( _baseDir );

    QPushButton* but = new QPushButton( QString::fromLatin1( ".." ), destinationPage );
    lay3->addWidget( but );
    but->setFixedWidth( 25 );

    connect( but, SIGNAL( clicked() ), this, SLOT( selectDir() ) );
    _baseDir->setText( Options::instance()->HTMLBaseDir() );

    // Base URL
    label = new QLabel( i18n("Base URL:"), destinationPage );
    lay2->addWidget( label, 6, 0 );

    _baseURL = new KLineEdit( destinationPage );
    _baseURL->setText( Options::instance()->HTMLBaseURL() );
    lay2->addWidget( _baseURL, 6, 1 );

    // Output Directory
    label = new QLabel( i18n("Output directory:"), destinationPage );
    lay2->addWidget( label, 7, 0 );
    _outputDir = new KLineEdit( destinationPage );
    lay2->addWidget( _outputDir, 7, 1 );

    lay1->addStretch( 1 );
}

bool HTMLExportDialog::generate()
{
    if ( !checkVars() )
        return false;

    // prepare the progress dialog
    _total = _waitCounter = calculateSteps();
    _progress->setTotalSteps( _total );
    _progress->setProgress( 0 );
    connect( _progress, SIGNAL( cancelled() ), this, SLOT( slotCancelGenerate() ) );

    _tempDir = KTempDir().name();

    hide();

    // Itertate over each of the image sizes needed.
    for( QValueList<ImageSizeCheckBox*>::Iterator sizeIt = _cbs.begin(); sizeIt != _cbs.end(); ++sizeIt ) {
        if ( (*sizeIt)->isChecked() ) {
            bool ok = generateIndexPage( (*sizeIt)->width(), (*sizeIt)->height() );
            if ( !ok )
                return false;
            for ( uint index = 0; index < _list.count(); ++index ) {
                ImageInfo* info = _list.at(index);
                ImageInfo* prev = 0;
                ImageInfo* next = 0;
                if ( index != 0 )
                    prev = _list.at(index-1);
                if ( index != _list.count() -1 )
                    next = _list.at(index+1);
                ok = generateContextPage( (*sizeIt)->width(), (*sizeIt)->height(), prev, info, next );
                if (!ok)
                    return false;
            }
        }
    }

    // Now generate the thumbnail images
    for( ImageInfoListIterator it( _list ); *it; ++it ) {

#if QT_VERSION < 0x030104
        if ( _progress->wasCancelled() )
            return false;
#else
        if ( _progress->wasCanceled() )
            return false;
#endif

        createImage( *it, _thumbSize->value() );
    }


    bool ok = linkIndexFile();
    if ( !ok )
        return false;

    if ( _waitCounter > 0 )
        qApp->eventLoop()->enterLoop();

    // Copy over the mainpage.css, indepage.css
    QString themeDir, themeAuthor, themeName;
    getThemeInfo( &themeDir, &themeName, &themeAuthor );
    QDir dir( themeDir );
    QStringList files = dir.entryList( QDir::Files );
    if( files.count() < 1 )
        kdDebug() << QString::fromLatin1("theme '%1' doesn't have enough files to be a theme").arg( themeDir ) << endl;

    for( QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
        if( *it == QString::fromLatin1("kimdaba.theme") ||
            *it == QString::fromLatin1("mainpage.html") ||
            *it == QString::fromLatin1("imagepage.html")) continue;
        QString from = QString::fromLatin1("%1%2").arg( themeDir ).arg(*it);
        QString to = _tempDir+QString::fromLatin1("/") + *it;
        ok = Util::copy( from, to );
        if ( !ok ) {
            KMessageBox::error( this, i18n("Error copying %1 to %2").arg( from ).arg( to ) );
        }
    }


    // Copy files over to destination.
    QString outputDir = _baseDir->text() + QString::fromLatin1( "/" ) + _outputDir->text();
    KIO::CopyJob* job = KIO::move( _tempDir, outputDir );
    connect( job, SIGNAL( result( KIO::Job* ) ), this, SLOT( showBrowser() ) );

    return true;
}

bool HTMLExportDialog::generateIndexPage( int width, int height )
{
    QString themeDir, themeAuthor, themeName;
    getThemeInfo( &themeDir, &themeName, &themeAuthor );
    QString content = Util::readFile( QString::fromLatin1( "%1mainpage.html" ).arg( themeDir ) );
    if ( content.isNull() )
        return false;

    content = QString::fromLatin1("<!--\nMade with KimDaba. (http://ktown.kde.org/kimdaba/)\nCopyright &copy; Jesper K. Pedersen\nTheme %1 by %2\n-->\n").arg( themeName ).arg( themeAuthor ) + content;

    if ( _whatToIncludeMap[QString::fromLatin1( "**DESCRIPTION**" )]->isChecked() )
        content.replace( QString::fromLatin1( "**DESCRIPTION**" ), _description->text() );
    else
        content.replace( QString::fromLatin1( "**DESCRIPTION**" ), QString::fromLatin1("") );
    content.replace( QString::fromLatin1( "**TITLE**" ), _title->text() );

    QDomDocument doc;

    QDomElement elm;
    QDomElement col;

    // -------------------------------------------------- Thumbnails
    // Initially all of the HTML generation was done using QDom, but it turned out in the end
    // to be much less code simply concatenating strings. This part, however, is easier using QDom
    // so we keep it using QDom.
    int count = 0;
    int cols = _numOfCols->value();
    QDomElement row;
    for( ImageInfoListIterator it( _list ); *it; ++it ) {

#if QT_VERSION < 0x030104
        if ( _progress->wasCancelled() )
            return false;
#else
        if ( _progress->wasCanceled() )
            return false;
#endif

        if ( count % cols == 0 ) {
            row = doc.createElement( QString::fromLatin1( "tr" ) );
            row.setAttribute( QString::fromLatin1( "class" ), QString::fromLatin1( "thumbnail-row" ) );
            doc.appendChild( row );
            count = 0;
        }

        col = doc.createElement( QString::fromLatin1( "td" ) );
        col.setAttribute( QString::fromLatin1( "class" ), QString::fromLatin1( "thumbnail-col" ) );
        row.appendChild( col );

        QDomElement href = doc.createElement( QString::fromLatin1( "a" ) );
        href.setAttribute( QString::fromLatin1( "href" ),
                           namePage( width, height, (*it)->fileName(false) ) );
        col.appendChild( href );

        QDomElement img = doc.createElement( QString::fromLatin1( "img" ) );
        img.setAttribute( QString::fromLatin1( "src" ),
                          nameImage( (*it)->fileName(), _thumbSize->value() ) );
        img.setAttribute( QString::fromLatin1( "alt" ),
                          nameImage( (*it)->fileName(), _thumbSize->value() ) );
        href.appendChild( img );
        ++count;
    }

    content.replace( QString::fromLatin1( "**THUMBNAIL-TABLE**" ), doc.toString() );

    // -------------------------------------------------- Resolutions
    QString resolutions;
    QValueList<ImageSizeCheckBox*> actRes = activeResolutions();
    if ( actRes.count() > 1 ) {
        resolutions += QString::fromLatin1( "Resolutions: " );
        for( QValueList<ImageSizeCheckBox*>::Iterator sizeIt = actRes.begin();
             sizeIt != actRes.end(); ++sizeIt ) {

            int w = (*sizeIt)->width();
            int h = (*sizeIt)->height();
            QString page = QString::fromLatin1( "index-%1.html" )
                           .arg( ImageSizeCheckBox::text( w, h, true ) );
            QString text = (*sizeIt)->text(false);

            resolutions += QString::fromLatin1( " " );
            if ( width == w && height == h ) {
                resolutions += text;
            }
            else {
                resolutions += QString::fromLatin1( "<a href=\"%1\">%2</a>" ).arg( page ).arg( text );
            }
        }
    }

    content.replace( QString::fromLatin1( "**RESOLUTIONS**" ), resolutions );

#if QT_VERSION < 0x030104
    if ( _progress->wasCancelled() )
        return false;
#else
    if ( _progress->wasCanceled() )
        return false;
#endif

    // -------------------------------------------------- write to file
    QString fileName = _tempDir + QString::fromLatin1("/index-%1.html" )
                       .arg(ImageSizeCheckBox::text(width,height,true));
    bool ok = writeToFile( fileName, content );
    if ( !ok )
        return false;

    return true;
}

bool HTMLExportDialog::generateContextPage( int width, int height, ImageInfo* prevInfo,
                                            ImageInfo* info, ImageInfo* nextInfo )
{
    QString themeDir, themeAuthor, themeName;
    getThemeInfo( &themeDir, &themeName, &themeAuthor );
    QString content = Util::readFile( QString::fromLatin1( "%1imagepage.html" ).arg( themeDir ));
    if ( content.isNull() )
        return false;

    content = QString::fromLatin1("<!--\nMade with KimDaba. (http://ktown.kde.org/kimdaba/)\nCopyright &copy; Jesper K. Pedersen\nTheme %1 by %2\n-->\n").arg( themeName ).arg( themeAuthor ) + content;

    content.replace( QString::fromLatin1( "**TITLE**" ), info->label() );
    content.replace( QString::fromLatin1( "**IMAGE**" ), createImage( info, width ) );


    // -------------------------------------------------- Links
    QString link;

    // prev link
    if ( prevInfo )
        link = QString::fromLatin1( "<a href=\"%1\">prev</a>" ).arg( namePage( width, height, prevInfo->fileName() ) );
    else
        link = QString::fromLatin1( "prev" );
    content.replace( QString::fromLatin1( "**PREV**" ), link );


    // index link
    link = QString::fromLatin1( "<a href=\"index-%1.html\">index</a>" ).arg(ImageSizeCheckBox::text(width,height,true));
    content.replace( QString::fromLatin1( "**INDEX**" ), link );

    // Next Link
    if ( nextInfo )
        link = QString::fromLatin1( "<a href=\"%1\">next</a>" ).arg( namePage( width, height, nextInfo->fileName() ) );
    else
        link = QString::fromLatin1( "next" );
    content.replace( QString::fromLatin1( "**NEXT**" ), link );

    if ( nextInfo )
        link = namePage( width, height, nextInfo->fileName() );
    else
        link = QString::fromLatin1( "index-%1.html" ).arg(ImageSizeCheckBox::text(width,height,true));

    content.replace( QString::fromLatin1( "**NEXTPAGE**" ), link );


    // -------------------------------------------------- Resolutions
    QString resolutions;
    QValueList<ImageSizeCheckBox*> actRes = activeResolutions();
    if ( actRes.count() > 1 ) {
        for( QValueList<ImageSizeCheckBox*>::Iterator sizeIt = actRes.begin();
             sizeIt != actRes.end(); ++sizeIt ) {
            int w = (*sizeIt)->width();
            int h = (*sizeIt)->height();
            QString page = namePage( w, h, info->fileName() );
            QString text = (*sizeIt)->text(false);
            resolutions += QString::fromLatin1( " " );

            if ( width == w && height == h )
                resolutions += text;
            else
                resolutions += QString::fromLatin1( "<a href=\"%1\">%2</a>" ).arg( page ).arg( text );
        }
    }
    content.replace( QString::fromLatin1( "**RESOLUTIONS**" ), resolutions );

    // -------------------------------------------------- Description
    QString description;

    QStringList optionGroups = Options::instance()->optionGroups();
    for( QStringList::Iterator it = optionGroups.begin(); it != optionGroups.end(); ++it ) {
        if ( info->optionValue( *it ).count() != 0 && _whatToIncludeMap[*it]->isChecked() ) {
            QString val = info->optionValue( *it ).join( QString::fromLatin1(", ") );
            description += QString::fromLatin1("  <li> <b>%1:</b> %2\n").arg( *it ).arg( val );
        }
    }

    if ( !info->description().isEmpty() ) {
        description += QString::fromLatin1( "  <li> <b>Description:</b> %1\n" ).arg( info->description() );
    }

    content.replace( QString::fromLatin1( "**DESCRIPTION**" ), QString::fromLatin1( "<ul>\n%1\n</ul>" ).arg( description ) );


    // -------------------------------------------------- write to file
    QString fileName = _tempDir + namePage( width, height, info->fileName() );
    bool ok = writeToFile( fileName, content );
    if ( !ok )
        return false;

    return true;
}



bool HTMLExportDialog::writeToFile( const QString& fileName, const QString& str )
{
    QFile file(fileName);
    if ( !file.open(IO_WriteOnly) ) {
        KMessageBox::error( this, i18n("Couldn't create file '%1'.").arg(fileName),
                            i18n("Couldn't Create File") );
        return false;
    }

    QCString cstr = str.utf8();
    file.writeBlock( cstr.data(), cstr.size() - 1);
    file.close();
    return true;
}


QString HTMLExportDialog::createImage( ImageInfo* info, int size )
{
    ImageManager::instance()->load( info->fileName(),  this, info->angle(), size, size, false, true );
    return nameImage( info->fileName(), size );
}

void HTMLExportDialog::pixmapLoaded( const QString& fileName, int size, int /*height*/, int /*angle*/, const QImage& image )
{

    _waitCounter--;

    QString dir = _tempDir;
    if ( !dir.isNull() ) {
        QString file = dir + QString::fromLatin1( "/" ) + nameImage( fileName, size );

        bool success = image.save( file, "JPEG" );
        if ( !success ) {
            QMessageBox::warning( this, i18n("Unable to Write Image"), i18n("Unable to write image '%1'.").arg(file), QMessageBox::Ok, 0 );
        }
    }
    else
        Q_ASSERT( false );


    _progress->setProgress( _total - _waitCounter );
    qApp->eventLoop()->processEvents( QEventLoop::AllEvents );

    if ( _waitCounter == 0 ) {
        qApp->eventLoop()->exitLoop();
    }
}

void HTMLExportDialog::slotOk()
{
    if( activeResolutions().count() < 1 ) {
        KMessageBox::error( 0, i18n( "You must select at least one resolution!" ) );
        return;
    }
    // Progress dialog
    _progress = new QProgressDialog( i18n("Generating images for HTML page "), i18n("&Cancel"), 0, this );

    bool ok = generate();
    if ( ok ) {
        Options::instance()->setHTMLBaseDir( _baseDir->text() );
        Options::instance()->setHTMLBaseURL( _baseURL->text() );
        accept();
    }
    delete _progress;
    _progress = 0;
}

void HTMLExportDialog::selectDir()
{
    KURL dir = KFileDialog::getExistingURL( _baseDir->text(), this );
    if ( !dir.url().isNull() )
        _baseDir->setText( dir.url() );
}

void HTMLExportDialog::slotCancelGenerate()
{
    ImageManager::instance()->stop( this );
    _waitCounter = 0;
}

void HTMLExportDialog::showBrowser()
{
    if ( ! _baseURL->text().isEmpty() )
        new KRun( QString::fromLatin1( "%1/%2/index.html" ).arg( _baseURL->text() ).arg( _outputDir->text() ) );
}

bool HTMLExportDialog::checkVars()
{
    QString outputDir = _baseDir->text() + QString::fromLatin1( "/" ) + _outputDir->text();


    // Ensure base dir is specified
    QString baseDir = _baseDir->text();
    if ( baseDir.isEmpty() ) {
        KMessageBox::error( this, i18n("<qt>You did not specify a base directory. "
                                       "This is the topmost directory for your images. "
                                       "Under this directory you will find each generated collection "
                                       "in separate directories.</qt>"),
                            i18n("No Base Directory Specified") );
        return false;
    }

    // ensure output directory is specified
    if ( _outputDir->text().isEmpty() ) {
        KMessageBox::error( this, i18n("<qt>You did not specify an output directory. "
                                       "This is a directory containing the actual images. "
                                       "The directory will be in the base directory specified above.</qt>"),
                            i18n("No Output Directory Specified") );
        return false;
    }

    // ensure base dir exists
    KIO::UDSEntry result;
#if KDE_IS_VERSION( 3,1,90 )
    bool ok = KIO::NetAccess::stat( baseDir, result, this );
#else
    bool ok = KIO::NetAccess::stat( baseDir, result );
#endif
    if ( !ok ) {
        KMessageBox::error( this, i18n("<qt>Error while reading information about %1. "
                                       "This is most likely because the directory does not exist.</qt>")
                            .arg( baseDir ) );
        return false;
    }

    KFileItem fileInfo( result, baseDir );
    if ( !fileInfo.isDir() ) {
        KMessageBox::error( this, i18n("<qt>%1 does not exist, is not a directory or "
                                       "cannot be written to.</qt>").arg( baseDir ) );
        return false;
    }


    // test if destination directory exists.
#if KDE_IS_VERSION( 3, 1, 90 )
    bool exists = KIO::NetAccess::exists( outputDir, false, this );
#else
    bool exists = KIO::NetAccess::exists( outputDir );
#endif
    if ( exists ) {
        int answer = QMessageBox::warning( this, i18n("Directory Exists"),
                                           i18n("<qt>Output directory %1 already exists. "
                                                "Usually you should specify a new directory. "
                                                "Continue?</qt>").arg( outputDir ),
                                           QMessageBox::No, QMessageBox::Yes );
        if ( answer == QMessageBox::No )
            return false;
    }
    return true;
}

int HTMLExportDialog::calculateSteps()
{
    int count = 0;
    for( QValueList<ImageSizeCheckBox*>::Iterator it2 = _cbs.begin(); it2 != _cbs.end(); ++it2 ) {
        if ( (*it2)->isChecked() )
            count++;
    }

    return _list.count() * ( 1 + count ); // 1 thumbnail + 1 real image
}

QString HTMLExportDialog::namePage( int width, int height, const QString& fileName )
{
    QPair<QString,int> key = qMakePair(fileName,width);
    if ( _pageNames.contains( key ) )
        return _pageNames[key];

    QFileInfo fi(fileName);
    QString baseName = fi.baseName();
    bool conflict = true;
    int i = 0;
    QString name;
    while ( conflict ) {
        name = QString::fromLatin1( "%1%2-%3.html" )
               .arg( baseName )
               .arg( (i == 0) ? QString::fromLatin1("") : QString::number(i) )
               .arg(ImageSizeCheckBox::text(width,height,true));
        ++i;
        conflict = _pageNames.values().contains( name );
    }
    _pageNames.insert( key, name );
    return name;
}

QString HTMLExportDialog::nameImage( const QString& fileName, int size )
{
    QPair<QString,int> key = qMakePair( fileName, size );
    if ( _imageNames.contains( key ) )
        return _imageNames[ key ];

    QFileInfo finfo( fileName );
    QString baseName = finfo.baseName();
    bool conflict = true;
    int i = 0;
    QString name;

    while ( conflict ){
        name = QString::fromLatin1( "%1%2" ).arg( baseName ).arg( (i == 0) ? QString::fromLatin1("") : QString::number(i) );
        if ( size != -1 )
            name += QString::fromLatin1( "-" ) + QString::number( size );
        name += QString::fromLatin1( ".jpg" ) ;

        ++i;
        conflict = _imageNames.values().contains( name );
    }
    _imageNames.insert( key, name );

    return name;
}


bool HTMLExportDialog::linkIndexFile()
{
    for( QValueList<ImageSizeCheckBox*>::Iterator it = _preferredSizes.begin();
         it != _preferredSizes.end(); ++it ) {
        if ( (*it)->isChecked() ) {
            QString fromFile = QString::fromLatin1("index-%1.html" )
                               .arg((*it)->text(true));
            QString destFile = _tempDir + QString::fromLatin1("/index.html");
            bool ok = ( symlink( fromFile.latin1(), destFile.latin1() ) == 0 );
            if ( !ok ) {
                KMessageBox::error( this, i18n("<qt>Unable to make a symlink from %1 to %2</qt>")
                                    .arg( fromFile ).arg( destFile ) );

                return false;
            }
            return ok;
        }
    }
    return false;
}


QValueList<ImageSizeCheckBox*> HTMLExportDialog::activeResolutions()
{
    QValueList<ImageSizeCheckBox*> res;
    for( QValueList<ImageSizeCheckBox*>::Iterator sizeIt = _cbs.begin(); sizeIt != _cbs.end(); ++sizeIt ) {
        if ( (*sizeIt)->isChecked() ) {
            res << *sizeIt;
        }
    }
    return res;
}

void HTMLExportDialog::populateThemesCombo()
{
    QStringList dirs = KGlobal::dirs()->findDirs( "data", QString::fromLocal8Bit("kimdaba/themes/") );
    int i = 0;
    for(QStringList::Iterator it = dirs.begin(); it != dirs.end(); ++it) {
        QDir dir(*it);
        QStringList themes = dir.entryList( QDir::Dirs | QDir::Readable );
        for(QStringList::Iterator it = themes.begin(); it != themes.end(); ++it) {
            if(*it == QString::fromLatin1(".") || *it == QString::fromLatin1("..")) continue;
            QString themePath = QString::fromLatin1("%1/%2/").arg(dir.path()).arg(*it);

            KSimpleConfig themeConfig( QString::fromLatin1( "%1kimdaba.theme" ).arg( themePath ), true );
            if( !themeConfig.hasGroup( QString::fromLatin1( "theme" ) ) ) {
                kdDebug() << QString::fromLatin1("invalid theme: %1 (missing theme section)").arg( *it )
                          << endl;
                continue;
            }
            themeConfig.setGroup( QString::fromLatin1( "theme" ) );
            QString themeName = themeConfig.readEntry( "Name" );
            QString themeAuthor = themeConfig.readEntry( "Author" );

            enableButtonOK( true );
            _themeBox->insertItem( i18n( "%1 (by %2)" ).arg( themeName ).arg( themeAuthor ), i );
            _themes.insert( i, themePath );
            i++;
        }
    }
    if(_themeBox->count() < 1) {
        KMessageBox::error( this, i18n("Couldn't find any themes - this is very likely an installation error" ) );
    }
}

void HTMLExportDialog::getThemeInfo( QString* baseDir, QString* name, QString* author )
{
    *baseDir = _themes[_themeBox->currentItem()];
    KSimpleConfig themeConfig( QString::fromLatin1( "%1kimdaba.theme" ).arg( *baseDir ), true );
    themeConfig.setGroup( QString::fromLatin1( "theme" ) );
    *name = themeConfig.readEntry( "Name" );
    *author = themeConfig.readEntry( "Author" );
}

int HTMLExportDialog::exec( const ImageInfoList& list )
{
    _list = list;
    _pageNames.clear();
    _imageNames.clear();
    return KDialogBase::exec();
}

#include "htmlexportdialog.moc"
