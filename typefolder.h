/* Copyright (C) 2003-2004 Jesper K. Pedersen <blackie@kde.org>

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

#ifndef TYPEFOLDER_H
#define TYPEFOLDER_H
#include "folder.h"

class TypeFolder :public Folder {
public:
    TypeFolder( const QString& optionGroup, const ImageSearchInfo& info, Browser* parent );
    virtual FolderAction* action( bool ctrlDown = false );
    virtual QPixmap pixmap();
    virtual QString text() const;
    virtual QString countLabel() const;
private:
    QString _optionGroup;
};

class TypeFolderAction :public FolderAction {

public:
    TypeFolderAction( const QString& optionGroup, const ImageSearchInfo& info, Browser* parent  );
    virtual void action( BrowserItemFactory* factory );
    virtual bool showsImages() const { return false; }
    virtual bool contentView() const { return true; }
    virtual QString title() const;
    virtual QString optionGroup() const;

private:
    QString _optionGroup;
};



#endif /* TYPEFOLDER_H */

