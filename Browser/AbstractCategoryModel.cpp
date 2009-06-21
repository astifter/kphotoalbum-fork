/* Copyright (C) 2003-2009 Jesper K. Pedersen <blackie@kde.org>

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

#include "AbstractCategoryModel.h"
#include <klocale.h>
#include <DB/ImageDB.h>
#include <DB/MemberMap.h>
#include <KIcon>
#include "enums.h"

Browser::AbstractCategoryModel::AbstractCategoryModel( const DB::CategoryPtr& category, const DB::ImageSearchInfo& info )
    : _category( category ), _info( info )
{
    _images = DB::ImageDB::instance()->classify( info, _category->name(), DB::Image );
    _videos = DB::ImageDB::instance()->classify( info, _category->name(), DB::Video );
}

bool Browser::AbstractCategoryModel::hasNoneEntry() const
{
    int imageCount = _images[DB::ImageDB::NONE()];
    int videoCount = _videos[DB::ImageDB::NONE()];
    return (imageCount + videoCount != 0);
}

QString Browser::AbstractCategoryModel::text( const QString& name ) const
{
    if ( name == DB::ImageDB::NONE() ) {
        if ( _info.categoryMatchText(_category->name()).length() == 0 )
            return i18n( "None" );
        else
            return i18n( "No other" );
    }
    else if ( name == QString::fromLatin1( "Video" ) )
        return i18n("Video");
    else if ( name == QString::fromLatin1( "Image" ) )
        return i18n("Image");

    else {
        if ( _category->name() == QString::fromLatin1( "Folder" ) ) {
            QRegExp rx( QString::fromLatin1( "(.*/)(.*)$") );
            QString value = name;
            value.replace( rx, QString::fromLatin1("\\2") );
            return value;
        } else {
            return name;
        }
    }
}

QPixmap Browser::AbstractCategoryModel::icon( const QString& name ) const
{
    if ( _category->viewType() == DB::Category::TreeView || _category->viewType() == DB::Category::IconView ) {
        if ( DB::ImageDB::instance()->memberMap().isGroup( _category->name(), name ) )
            return KIcon( QString::fromLatin1( "folder_image" ) ).pixmap(22);
        else {
            return _category->icon();
        }
    }
    else {
        // The category images are screenshot from the size of the viewer (Which might very well be considered a bug)
        // This is the reason for asking for the thumbnail height being 3/4 of its width.
        return _category->categoryImage( _category->name(), name, _category->thumbnailSize(), _category->thumbnailSize() * 3.0 / 4.0 );
    }
}

QVariant Browser::AbstractCategoryModel::data( const QModelIndex & index, int role) const
{
    if ( !index.isValid() )
        return QVariant();
    const QString name = indexToName( index );
    const int column = index.column();

    if ( role == Qt::DisplayRole ) {
        switch( column ) {
        case 0: return text(name);
        case 1: return i18np("1 images", "%1 images", _images[name]);
        case 2: return i18np("1 video", "%1 videos", _videos[name]);
        }
    }

    else if ( role == Qt::DecorationRole && column == 0) {
        return icon( name );
    }

    else if ( role == Qt::ToolTipRole )
        return text(name);

    else if ( role == ItemNameRole )
        return name;

    else if ( role == ValueRole ) {
        switch ( column ) {
        case 0: return name; // Notice we sort by **None** rather than None, which makes it show up at the top for less than searches.
        case 1: return _images[name];
        case 2: return _videos[name];
        }
    }

    return QVariant();
}

Qt::ItemFlags Browser::AbstractCategoryModel::flags( const QModelIndex& ) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant Browser::AbstractCategoryModel::headerData( int section, Qt::Orientation orientation, int role) const
{
    if ( orientation == Qt::Vertical || role != Qt::DisplayRole )
        return QVariant();

    switch ( section ) {
    case 0: return _category->text();
    case 1: return i18n("Images");
    case 2: return i18n("Videos");
    }

    return QVariant();
}

