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

#include "viewhandler.h"
#include <qevent.h>
#include <qpainter.h>
#include "displayarea.h"
#include <qapplication.h>
#include <qcursor.h>

ViewHandler::ViewHandler( DisplayArea* display )
    :DisplayAreaHandler( display )
{

}

bool ViewHandler::mousePressEvent( QMouseEvent*e,  const QPoint& unTranslatedPos, double /*scaleFactor*/ )
{
    _pan = false;
    _scale = false;

    if ( (e->button() & Qt::LeftButton ) ) {
        // scaling
        _scale = true;
        _start = e->pos();
        return true;
    }

    else if ( e->button() & Qt::MidButton ) {
        // panning
        _pan = true;
        _last = unTranslatedPos;
        qApp->setOverrideCursor( SizeAllCursor  );
        _errorX = 0;
        _errorY = 0;
        return true;
    }
    else
        return true;
}

bool ViewHandler::mouseMoveEvent( QMouseEvent* e,  const QPoint& unTranslatedPos, double scaleFactor )
{
    if ( _scale ) {
        QPainter* p = _display->painter();
        p->setPen( QPen(Qt::black, 3, Qt::DashDotLine) );
        p->drawRect( QRect(_start, e->pos()) );
        delete p;
        return true;
    }
    else if ( _pan ) {
        // This code need to be taking the error into account, consider this situation:
        // The user moves the mouse very slowly, only 1 pixel at a time, scale factor is 3
        // Then translated delta would be 1/3 which every time would be
        // rounded down to 0, and the panning would never move any pixels.
        double deltaX = _errorX + (_last.x() - unTranslatedPos.x())/scaleFactor;
        double deltaY = _errorY + (_last.y() - unTranslatedPos.y())/scaleFactor;
        QPoint deltaPoint = QPoint( (int) deltaX, (int) deltaY );
        _errorX = deltaX - ((double) ((int) deltaX ) );
        _errorY = deltaY - ((double) ((int) deltaY) );

        _display->pan( deltaPoint );
        _last = unTranslatedPos;
        return true;
    }
    else
        return false;
}

bool ViewHandler::mouseReleaseEvent( QMouseEvent* e,  const QPoint& /*unTranslatedPos*/, double /*scaleFactor*/ )
{
    if ( _scale && (e->pos()-_start).manhattanLength() > 1 ) {
        _display->zoom( _start, e->pos() );
        return true;
    }
    else if ( _pan ) {
        qApp->restoreOverrideCursor();
        return true;
    }
    else
        return false;
}
