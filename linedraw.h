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

#ifndef LINEDRAW_H
#define LINEDRAW_H
#include "draw.h"

class LineDraw :public Draw
{
public:
    LineDraw( QWidget* widget = 0 );
    LineDraw( QDomElement elm );
    void draw( QPainter&, QMouseEvent* );
    virtual PointList anchorPoints();
    virtual Draw* clone();
    virtual QDomElement save( QDomDocument doc );
};

#endif /* LINEDRAW_H */

