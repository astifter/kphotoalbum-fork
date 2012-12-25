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
#ifndef CATEGORYLISTVIEW_DRAGGABLELISTVIEW_H
#define CATEGORYLISTVIEW_DRAGGABLELISTVIEW_H
#define QT3_SUPPORT
#include <q3listview.h>
#include "DB/CategoryPtr.h"

namespace CategoryListView
{
class DraggableListView :public Q3ListView
{
    Q_OBJECT

public:
    DraggableListView( const DB::CategoryPtr& category, QWidget* parent, const char* name = 0 );
    DB::CategoryPtr category() const;
    void emitItemsChanged();

signals:
    void itemsChanged();

protected:
    virtual Q3DragObject* dragObject();

private:
    const DB::CategoryPtr _category;
};

}

#endif /* CATEGORYLISTVIEW_DRAGGABLELISTVIEW_H */
