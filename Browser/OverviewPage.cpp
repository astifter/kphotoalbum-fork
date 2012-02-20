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

#include "OverviewPage.h"
#include <AnnotationDialog/Dialog.h>
#include <Utilities/ShowBusyCursor.h>
#include "enums.h"
#include <KMessageBox>
#include <Exif/SearchDialog.h>
#include "ImageViewPage.h"
#include "CategoryPage.h"
#include "BrowserWidget.h"
#include <MainWindow/Window.h>
#include <klocale.h>
#include <DB/ImageDB.h>
#include <kicon.h>
#include <config-kpa-exiv2.h>
#include "DB/CategoryCollection.h"

const int THUMBNAILSIZE = 70;

AnnotationDialog::Dialog* Browser::OverviewPage::_config = 0;
Browser::OverviewPage::OverviewPage( const Breadcrumb& breadcrumb, const DB::ImageSearchInfo& info, BrowserWidget* browser )
    : BrowserPage( info, browser), _breadcrumb( breadcrumb )
{
    int row = 0;
    Q_FOREACH( const DB::CategoryPtr& category, categories() ) {
        QMap<QString, uint> images = DB::ImageDB::instance()->classify( BrowserPage::searchInfo(), category->name(), DB::Image );
        QMap<QString, uint> videos = DB::ImageDB::instance()->classify( BrowserPage::searchInfo(), category->name(), DB::Video );
        DB::MediaCount count( images.count(), videos.count() );
        _count[row] = count;
        ++row;
    }
}

int Browser::OverviewPage::rowCount( const QModelIndex& parent ) const
{
    if ( parent != QModelIndex() )
        return 0;

    return categories().count() +
#ifdef HAVE_EXIV2
        1 +
#endif
        3; // Search info + Untagged Images + Show Image
}

QVariant Browser::OverviewPage::data( const QModelIndex& index, int role) const
{
    if ( role == ValueRole )
        return index.row();

    const int row = index.row();
    if ( isCategoryIndex(row) )
        return categoryInfo( row, role );
    else if ( isExivIndex( row ) )
        return exivInfo( role );
    else if ( isSearchIndex( row ) )
        return searchInfo( role );
    else if ( isUntaggedImagesIndex( row ) )
        return untaggedImagesInfo( role );
    else if ( isImageIndex( row ) )
        return imageInfo( role );
    return QVariant();
}

bool Browser::OverviewPage::isCategoryIndex( int row ) const
{
    return row < categories().count() && row >= 0;
}

bool Browser::OverviewPage::isExivIndex( int row ) const
{
#ifdef HAVE_EXIV2
    return row == categories().count();
#else
    Q_UNUSED(row);
    return false;
#endif
}

bool Browser::OverviewPage::isSearchIndex( int row ) const
{
    return rowCount()-3 == row;
}

bool Browser::OverviewPage::isUntaggedImagesIndex( int row ) const
{
    return rowCount()-2 == row;

}

bool Browser::OverviewPage::isImageIndex( int row ) const
{
    return rowCount()-1 == row;
}


QList<DB::CategoryPtr> Browser::OverviewPage::categories() const
{
    return DB::ImageDB::instance()->categoryCollection()->categories();
}

QVariant Browser::OverviewPage::categoryInfo( int row, int role ) const
{
    if ( role == Qt::DisplayRole )
        return categories()[row]->text();
    else if ( role == Qt::DecorationRole )
        return categories()[row]->icon(THUMBNAILSIZE);

    return QVariant();
}

QVariant Browser::OverviewPage::exivInfo( int role ) const
{
    if ( role == Qt::DisplayRole )
        return i18n("Exif Info");
    else if ( role == Qt::DecorationRole ) {
        return KIcon( QString::fromLatin1( "text-plain" ) ).pixmap(THUMBNAILSIZE);
    }

    return QVariant();
}

QVariant Browser::OverviewPage::searchInfo( int role ) const
{
    if ( role == Qt::DisplayRole )
        return i18n("Search");
    else if ( role == Qt::DecorationRole )
        return KIcon( QString::fromLatin1( "system-search" ) ).pixmap(THUMBNAILSIZE);
    return QVariant();
}

QVariant Browser::OverviewPage::untaggedImagesInfo( int role ) const
{
    if ( role == Qt::DisplayRole )
        return i18n("Untagged Images");
    else if ( role == Qt::DecorationRole )
        return KIcon( QString::fromLatin1( "button_ok" ) ).pixmap(THUMBNAILSIZE);
    return QVariant();

}

QVariant Browser::OverviewPage::imageInfo( int role ) const
{
    if ( role == Qt::DisplayRole )
        return i18n("Show Thumbnails");
    else if ( role == Qt::DecorationRole )
        return KIcon( QString::fromLatin1( "kphotoalbum" ) ).pixmap(THUMBNAILSIZE);
    return QVariant();
}

Browser::BrowserPage* Browser::OverviewPage::activateChild( const QModelIndex& index )
{
    const int row = index.row();

    if ( isCategoryIndex(row) )
        return new Browser::CategoryPage( categories()[row], BrowserPage::searchInfo(), browser() );
    else if ( isExivIndex( row ) )
        return activateExivAction();
    else if ( isSearchIndex( row ) )
        return activateSearchAction();
    else if ( isUntaggedImagesIndex( row ) ) {
        return activateUntaggedImagesAction();
    }
    else if ( isImageIndex( row ) )
        return new ImageViewPage( BrowserPage::searchInfo(), browser()  );

    return 0;
}

void Browser::OverviewPage::activate()
{
    browser()->setModel( this );
}

Qt::ItemFlags Browser::OverviewPage::flags( const QModelIndex & index ) const
{
    if ( isCategoryIndex(index.row() ) && _count[index.row()].total() <= 1 )
        return QAbstractListModel::flags(index) & ~Qt::ItemIsEnabled;
    else
        return QAbstractListModel::flags(index);
}

bool Browser::OverviewPage::isSearchable() const
{
    return true;
}

Browser::BrowserPage* Browser::OverviewPage::activateExivAction()
{
#ifdef HAVE_EXIV2
    QPointer<Exif::SearchDialog> dialog = new Exif::SearchDialog( browser() );

    {
        Utilities::ShowBusyCursor undoTheBusyWhileShowingTheDialog( Qt::ArrowCursor );

        if ( dialog->exec() == QDialog::Rejected )
            return 0;
    }


    Exif::SearchInfo result = dialog->info();

    DB::ImageSearchInfo info = BrowserPage::searchInfo();

    info.addExifSearchInfo( dialog->info() );

    if ( DB::ImageDB::instance()->count( info ).total() == 0 ) {
        KMessageBox::information( browser(), i18n( "Search did not match any images or videos." ), i18n("Empty Search Result") );
        return 0;
    }

    return new OverviewPage( Breadcrumb( i18n("EXIF Search")), info, browser() );
#else
    return 0;
#endif // HAVE_EXIV2
}

Browser::BrowserPage* Browser::OverviewPage::activateSearchAction()
{
    if ( !_config )
        _config = new AnnotationDialog::Dialog( browser() );

    Utilities::ShowBusyCursor undoTheBusyWhileShowingTheDialog( Qt::ArrowCursor );
    DB::ImageSearchInfo tmpInfo = BrowserPage::searchInfo();
    DB::ImageSearchInfo info = _config->search( &tmpInfo ); // PENDING(blackie) why take the address?

    if ( info.isNull() )
        return 0;

    if ( DB::ImageDB::instance()->count( info ).total() == 0 ) {
        KMessageBox::information( browser(), i18n( "Search did not match any images or videos." ), i18n("Empty Search Result") );
        return 0;
    }

    return new OverviewPage( Breadcrumb( i18n("search") ), info, browser() );

}

Browser::Breadcrumb Browser::OverviewPage::breadcrumb() const
{
    return _breadcrumb;
}

bool Browser::OverviewPage::showDuringMovement() const
{
    return true;
}

Browser::BrowserPage* Browser::OverviewPage::activateUntaggedImagesAction()
{
    if ( Settings::SettingsData::instance()->hasUntaggedCategoryFeatureConfigured() ) {
        DB::ImageSearchInfo info = BrowserPage::searchInfo();
        info.addAnd( Settings::SettingsData::instance()->untaggedCategory(),
                                   Settings::SettingsData::instance()->untaggedTag() );
        return new ImageViewPage( info, browser()  );
    }
    else {
        KMessageBox::information( browser(),
                                  i18n("<p>You have not yet configured which tag to use for indicating untagged images.</p>"
                                       "<p>Please follow these steps to do so:"
                                       "<ul><li>In the menu bar choose <b>Settings</b></li>"
                                       "<li>From there choose <b>Configure KPhotoAlbum</b></li>"
                                       "<li>Now choose the <b>Categories</b> icon</li>"
                                       "<li>Now configure section <b>Untagged Images</b></li></ul></p>"),
                                  i18n("Features has not been configured") );
        return 0;
    }
}



