/*
 *  Copyright (c) 2003 Jesper K. Pedersen <blackie@kde.org>
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

#include "rectdraw.h"
#include <qpainter.h>
RectDraw::RectDraw(QWidget* widget ) :Draw( widget)
{

}

void RectDraw::draw( QPainter& painter, QMouseEvent* event )
{
    Draw::draw( painter, event );
    painter.drawRect( g2w(_startPos).x(), g2w(_startPos).y(), g2w(_lastPos).x()-g2w(_startPos).x(), g2w(_lastPos).y()-g2w(_startPos).y() );
}

PointList RectDraw::anchorPoints()
{
    PointList res;
    res << g2w(_startPos) << g2w(_lastPos) << g2w(QPoint( _startPos.x(), _lastPos.y() ))
        << g2w(QPoint( _lastPos.x(), _startPos.y() ));
    return res;
}

Draw* RectDraw::clone()
{
    RectDraw* res = new RectDraw();
    *res = *this;
    return res;

}

QDomElement RectDraw::save( QDomDocument doc )
{
    QDomElement res = doc.createElement( QString::fromLatin1( "Rectangle" ) );
    saveDrawAttr( &res );
    return res;
}

RectDraw::RectDraw( QDomElement elm )
    : Draw()
{
    readDrawAttr( elm );
}


