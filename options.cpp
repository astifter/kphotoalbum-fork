/* Copyright (C) 2003-2005 Jesper K. Pedersen <blackie@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "options.h"
#include <qdom.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include "util.h"
#include <stdlib.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <qapplication.h>
#include <qcursor.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kglobal.h>
#include "imagedb.h"
#include "imageconfig.h"
#include <qtextstream.h>
#include <qregexp.h>
#include <qmessagebox.h>
#include "categorycollection.h"
#include <qdatetime.h>
#include <qnamespace.h>
#include "imageinfo.h"
#include <kapplication.h>
#include <kconfig.h>
#include "options.moc"
#include "membermap.h"

#define STR(x) QString::fromLatin1(x)

Options* Options::_instance = 0;

Options* Options::instance()
{
    if ( ! _instance )
        qFatal("instance called before loading a setup!");
    return _instance;
}

Options::Options( const QDomElement& options, const QString& imageDirectory )
    : _hasAskedAboutTimeStamps( false ), _imageDirectory( imageDirectory )
{
    Util::readOptions( options, &_options, CategoryCollection::instance() );
    createSpecialCategories();
}

void Options::save( QDomElement top )
{
    QDomDocument doc = top.ownerDocument();

    QStringList grps = CategoryCollection::instance()->categoryNames();
    QDomElement options = doc.createElement( STR("options") );
    top.appendChild( options );
    (void) Util::writeOptions( doc, options, _options, CategoryCollection::instance() );
}

void Options::setOption( const QString& key, const QStringList& value )
{
    _options[key] = value;
}

void Options::removeOption( const QString& key, const QString& value )
{
    _options[key].remove( value );
    emit deletedOption( key, value );
}

void Options::renameOption( const QString& category, const QString& oldValue, const QString& newValue )
{
    _options[category].remove( oldValue );
    addOption( category, newValue );
    emit renamedOption( category, oldValue, newValue );
}

void Options::addOption( const QString& key, const QString& value )
{
    if ( _options[key].contains( value ) )
        _options[key].remove( value );
    _options[key].prepend( value );
}

QStringList Options::optionValue( const QString& key ) const
{
    return _options[key];
}

QStringList Options::optionValueInclGroups( const QString& category ) const
{
    // values including member groups

    QStringList items = optionValue( category );
    QStringList itemsAndGroups = QStringList::QStringList();
    for( QStringList::Iterator it = items.begin(); it != items.end(); ++it ) {
        itemsAndGroups << *it ;
    };
    // add the groups to the listbox too, but only if the group is not there already, which will be the case
    // if it has ever been selected once.
    QStringList groups = ImageDB::instance()->memberMap().groups( category );
    for( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it ) {
        if ( ! items.contains(  *it ) )
            itemsAndGroups << *it ;
    };
    if ( viewSortType() == SortAlpha )
        itemsAndGroups.sort();
    return itemsAndGroups;
}


bool Options::trustTimeStamps()
{
    if ( tTimeStamps() == Always )
        return true;
    else if ( tTimeStamps() == Never )
        return false;
    else {
        if (!_hasAskedAboutTimeStamps ) {
            QApplication::setOverrideCursor( Qt::ArrowCursor );
            int answer = KMessageBox::questionYesNo( 0, i18n("New images were found. Should I trust their time stamps?"),
                                                     i18n("Trust Time Stamps?") );
            QApplication::restoreOverrideCursor();
            if ( answer == KMessageBox::Yes )
                _trustTimeStamps = true;
            else
                _trustTimeStamps = false;
            _hasAskedAboutTimeStamps = true;
        }
        return _trustTimeStamps;
    }
}
void Options::setTTimeStamps( TimeStampTrust t )
{
    setValue( STR("General"), STR("trustTimeStamps"), (int) t );
}

Options::TimeStampTrust Options::tTimeStamps() const
{
    return (TimeStampTrust) value(  STR("General"), STR("trustTimeStamps"), (int) Always );
}

QString Options::imageDirectory() const
{
    if ( !_imageDirectory.endsWith( STR( "/" ) ) )
        return _imageDirectory + STR( "/" );
    else
        return _imageDirectory;
}


Options::Position Options::infoBoxPosition() const
{
    return (Position) value( STR("Viewer"), STR("infoBoxPosition"), 0 );
}

void Options::setInfoBoxPosition( Position pos )
{
    setValue( STR("Viewer"), STR("infoBoxPosition"), (int) pos );
}

/**
   Returns whether the given category is shown in the viewer.
*/
bool Options::showOption( const QString& category ) const
{
    return CategoryCollection::instance()->categoryForName(category)->doShow();
}

void Options::setShowOption( const QString& category, bool b )
{
    CategoryCollection::instance()->categoryForName(category)->setDoShow( b );
}

QString Options::HTMLBaseDir() const
{
    return value( groupForDatabase( STR("HTML Settings") ), STR("baseDir"), QString::fromLocal8Bit(getenv("HOME")) + STR( "/public_html") );
}

void Options::setHTMLBaseDir( const QString& dir )
{
    setValue( groupForDatabase( STR("HTML Settings") ), STR("baseDir"), dir );
}

QString Options::HTMLBaseURL() const
{
    return value( groupForDatabase( STR("HTML Settings") ), STR("baseUrl"),  STR( "file://" ) + HTMLBaseDir() );
}

void Options::setHTMLBaseURL( const QString& url )
{
    setValue( groupForDatabase( STR("HTML Settings") ), STR("baseUrl"), url );
}

QString Options::HTMLDestURL() const
{
    return value( groupForDatabase( STR("HTML Settings") ), STR("destUrl"),  STR( "file://" ) + HTMLBaseDir() );
}

void Options::setHTMLDestURL( const QString& url )
{
    setValue( groupForDatabase( STR("HTML Settings") ), STR("destUrl"), url );
}


void Options::setup( const QDomElement& options, const QString& imageDirectory )
{
    _instance = new Options( options, imageDirectory );
}

void Options::setCurrentLock( const ImageSearchInfo& info, bool exclude )
{
    info.saveLock();
    setValue( groupForDatabase( STR("Privacy Settings") ), STR("exclude"), exclude );
}

ImageSearchInfo Options::currentLock() const
{
    return ImageSearchInfo::loadLock();
}

void Options::setLocked( bool lock )
{
    bool changed = ( lock != isLocked() );
    setValue( groupForDatabase( STR("Privacy Settings") ), STR("locked"), lock );
    if (changed)
        emit locked( lock, lockExcludes() );
}

bool Options::isLocked() const
{
    return value( groupForDatabase( STR("Privacy Settings") ), STR("locked"), false );
}

bool Options::lockExcludes() const
{
    return value( groupForDatabase( STR("Privacy Settings") ), STR("exclude"), false );
}

void Options::setPassword( const QString& passwd )
{
    setValue( groupForDatabase( STR("Privacy Settings") ), STR("password"), passwd );
}

QString Options::password() const
{
    return value( groupForDatabase( STR("Privacy Settings") ), STR("password"), STR("") );
}

QString Options::fileForCategoryImage( const QString& category, QString member ) const
{
    QString dir = imageDirectory() + STR("CategoryImages" );
    member.replace( ' ', '_' );
    QString fileName = dir + STR("/%1-%2.jpg").arg( category ).arg( member );
    return fileName;
}


void Options::setOptionImage( const QString& category, QString member, const QImage& image )
{
    QString dir = imageDirectory() + STR("CategoryImages" );
    QFileInfo fi( dir );
    bool ok;
    if ( !fi.exists() ) {
        bool ok = QDir().mkdir( dir );
        if ( !ok ) {
            QMessageBox::warning( 0, i18n("Unable to Create Directory"), i18n("Unable to create directory '%1'.").arg( dir ), QMessageBox::Ok, 0 );
            return;
        }
    }
    QString fileName = fileForCategoryImage( category, member );
    ok = image.save( fileName, "JPEG" );
    if ( !ok ) {
        QMessageBox::warning( 0, i18n("Error Saving Image"), i18n("Error when saving image '%1'.").arg(fileName), QMessageBox::Ok, 0 );
        return;
    }
}

QImage Options::optionImage( const QString& category, QString member, int size ) const
{
    QString fileName = fileForCategoryImage( category, member );
    QImage img;
    bool ok = img.load( fileName, "JPEG" );
    if ( ! ok ) {
        if ( ImageDB::instance()->memberMap().isGroup( category, member ) )
            img = KGlobal::iconLoader()->loadIcon( STR( "kuser" ), KIcon::Desktop, size );
        else
            img = CategoryCollection::instance()->categoryForName( category )->icon( size );
    }
    return img.smoothScale( size, size, QImage::ScaleMin );
}

void Options::setViewSortType( ViewSortType tp )
{
    bool changed = ( viewSortType() != tp );
    setValue( STR("General"), STR("viewSortType"), (int) tp );
    if ( changed )
        emit viewSortTypeChanged( tp );
}

Options::ViewSortType Options::viewSortType() const
{
    return (ViewSortType) value( STR("General"), STR("viewSortType"), 0 );
}

void Options::setFromDate( const QDate& date)
{
    if (date.isValid())
        setValue( STR("Miscellaneous"), STR("fromDate"), date.toString( Qt::ISODate ) );
}

QDate Options::fromDate() const
{
    QString date = value( STR("Miscellaneous"), STR("fromDate"), STR("") );
    if ( date.isEmpty() )
        return QDate( QDate::currentDate().year(), 1, 1 );
    else
        return QDate::fromString( date, ISODate );
}

void  Options::setToDate( const QDate& date)
{
    if (date.isValid())
        setValue( STR("Miscellaneous"), STR("toDate"), date.toString( Qt::ISODate ) );
}

QDate Options::toDate() const
{
    QString date = value( STR("Miscellaneous"), STR("toDate"), STR("") );
    if ( date.isEmpty() )
        return QDate( QDate::currentDate().year()+1, 1, 1 );
    else
        return QDate::fromString( date, ISODate );
}

QString Options::albumCategory() const
{
    QString category = value( STR("General"), STR("albumCategory"), STR("") );

    if ( !CategoryCollection::instance()->categoryNames().contains( category ) ) {
        category = CategoryCollection::instance()->categoryNames()[0];
        const_cast<Options*>(this)->setAlbumCategory( category );
    }

    return category;
}

void Options::setAlbumCategory( const QString& category )
{
    setValue( STR("General"), STR("albumCategory"), category );
}

void Options::setWindowGeometry( WindowType win, const QRect& geometry )
{
    KConfig* config = kapp->config();
    config->setGroup( "Window Geometry" );
    config->writeEntry( windowTypeToString( win ), geometry );
}

QRect Options::windowGeometry( WindowType win ) const
{
    KConfig* config = kapp->config();
    config->setGroup( "Window Geometry" );
    QRect rect( 0,0, 800, 600 );
    return config->readRectEntry( windowTypeToString( win ), &rect );
}

bool Options::ready()
{
    return _instance != 0;
}

void Options::createSpecialCategories()
{
    Category* folderCat = CategoryCollection::instance()->categoryForName( STR( "Folder" ) );
    if( folderCat == 0 ) {
        _options.insert( STR("Folder"), QStringList() );
        folderCat = new Category( STR("Folder"), STR("folder"), Category::Small, Category::ListView, false );
        CategoryCollection::instance()->addCategory( folderCat );
    }
    folderCat->setSpecialCategory( true );


    Category* tokenCat = CategoryCollection::instance()->categoryForName( STR( "Tokens" ) );
    if ( !tokenCat ) {
        _options.insert( STR("Tokens"), QStringList() );
        tokenCat = new Category( STR("Tokens"), STR("cookie"), Category::Small, Category::ListView, true );
        CategoryCollection::instance()->addCategory( tokenCat );
    }
    tokenCat->setSpecialCategory( true );
}


int Options::value( const QString& group, const QString& option, int defaultValue ) const
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    return config->readNumEntry( option, defaultValue );
}

QString Options::value( const QString& group, const QString& option, const QString& defaultValue ) const
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    return config->readEntry( option, defaultValue );
}

bool Options::value( const QString& group, const QString& option, bool defaultValue ) const
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    return config->readBoolEntry( option, defaultValue );
}

QColor Options::value( const QString& group, const QString& option, const QColor& defaultValue ) const
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    return config->readColorEntry( option, &defaultValue );
}

QSize Options::value( const QString& group, const QString& option, const QSize& defaultValue ) const
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    return config->readSizeEntry( option, &defaultValue );
}

void Options::setValue( const QString& group, const QString& option, int value )
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    config->writeEntry( option, value );
}

void Options::setValue( const QString& group, const QString& option, const QString& value )
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    config->writeEntry( option, value );
}

void Options::setValue( const QString& group, const QString& option, bool value )
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    config->writeEntry( option, value );
}

void Options::setValue( const QString& group, const QString& option, const QColor& value )
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    config->writeEntry( option, value );
}

void Options::setValue( const QString& group, const QString& option, const QSize& value )
{
    KConfig* config = kapp->config();
    config->setGroup( group );
    config->writeEntry( option, value );
}

QSize Options::histogramSize() const
{
    return value( STR("General"), STR("histogramSize"), QSize( 15, 30 ) );
}

void Options::setHistogramSize( const QSize& size )
{
    bool changed = (size != histogramSize() );
    setValue( STR("General"), STR("histogramSize"), size );
    if (changed)
        emit histogramSizeChanged( size );
}

QString Options::windowTypeToString( WindowType tp ) const
{
    switch (tp) {
    case MainWindow: return STR("MainWindow");
    case ConfigWindow: return STR("ConfigWindow");
    }
    return STR("");
}

QString Options::groupForDatabase( const QString& setting ) const
{
    return STR("%1 - %2").arg( setting ).arg( imageDirectory() );
}


#undef STR
