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

#include "displayarea.h"
#include "draw.h"
#include "linedraw.h"
#include "rectdraw.h"
#include "circledraw.h"
#include <qpainter.h>
#include <qpicture.h>
#include "options.h"
#include "imageinfo.h"
#include "imagemanager.h"
#include "imageloader.h"

DisplayArea::DisplayArea( QWidget* parent, const char* name )
    :QLabel( parent, name ), _tool( None ), _activeTool( 0 )
{
    setAlignment( AlignCenter );
//    setBackgroundMode( NoBackground );
    setPaletteBackgroundColor( black );
}

void DisplayArea::slotLine()
{
    _tool = Line;
    drawAll();
}

void DisplayArea::slotRectangle()
{
    _tool = Rectangle;
    drawAll();
}

void DisplayArea::slotCircle()
{
    _tool = Circle;
    drawAll();
}

void DisplayArea::mousePressEvent( QMouseEvent* event )
{
    if ( _tool == None )
        QLabel::mousePressEvent( event );
    else if ( _tool == Select )  {
        _activeTool = findShape( event->pos() );
        drawAll();
        if ( !_activeTool )
            QLabel::mousePressEvent( event );
    }
    else {
        _activeTool = createTool();
        _activeTool->startDraw( event );
    }
}

void DisplayArea::mouseMoveEvent( QMouseEvent* event )
{
    if ( _activeTool && _tool != Select && _tool != None) {
        QPixmap pix = _curPixmap;
        QPainter painter( &pix );
        setupPainter( painter );
        _activeTool->draw( painter, event );
        QLabel::setPixmap( pix );
    }
    else
        QLabel::mouseMoveEvent( event );
}

void DisplayArea::mouseReleaseEvent( QMouseEvent* event )
{
    if ( _tool == Select || _tool == None ) {
        QLabel::mouseReleaseEvent( event );
    }
    else if ( _activeTool ) {
        QPixmap pix = _curPixmap;
        QPainter painter( &pix );
        setupPainter( painter );
        _activeTool->draw( painter, event );
        _curPixmap = pix;
        QLabel::setPixmap( pix );
        _drawings.append( _activeTool );
    }
    else
        QLabel::mouseReleaseEvent( event );
}

Draw* DisplayArea::createTool()
{
    switch ( _tool ) {
    case Line: return new LineDraw( this );
    case Rectangle: return new RectDraw( this );
    case Circle: return new CircleDraw( this );
    default:
    {
        Q_ASSERT( false );
        return 0;
    }
    }
}

void DisplayArea::setPixmap( const QPixmap& pixmap )
{
    _origPixmap = pixmap;
    drawAll();
    QLabel::setPixmap( _curPixmap );
}

void DisplayArea::drawAll()
{
    _curPixmap = _origPixmap;
    if ( _curPixmap.isNull() )
        return;

    QPainter painter( &_curPixmap );
    if ( Options::instance()->showDrawings() || _tool != None ) {
        for( QValueList<Draw*>::Iterator it = _drawings.begin(); it != _drawings.end(); ++it ) {
            painter.save();
            setupPainter( painter );
            (*it)->draw( painter, 0 );
            painter.restore();
            if ( _tool == Select ) {
                PointList list = (*it)->anchorPoints();
                for( PointListIterator it2 = list.begin(); it2 != list.end(); ++it2 ) {
                    QPoint point = *it2;
                    painter.save();
                    if ( *it == _activeTool )
                        painter.setBrush( Qt::red );
                    else
                        painter.setBrush( Qt::blue );
                    painter.drawRect( point.x()-4, point.y()-4, 8, 8 );
                    painter.restore();
                }
            }
        }
    }
    QLabel::setPixmap( _curPixmap );
}

void DisplayArea::slotSelect()
{
    _tool = Select;
    _activeTool = 0;
    drawAll();
}

Draw* DisplayArea::findShape( const QPoint& pos)
{
    for( QValueList<Draw*>::Iterator it = _drawings.begin(); it != _drawings.end(); ++it ) {
        PointList list = (*it)->anchorPoints();
        for( PointListIterator it2 = list.begin(); it2 != list.end(); ++it2 ) {
            QPoint point = *it2;
            QRect rect( point.x()-4, point.y()-4, 8, 8 );
            if ( rect.contains( pos ) )
                return *it;
        }
    }
    return 0;
}

void DisplayArea::cut()
{
    if ( _activeTool )  {
        _drawings.remove( _activeTool );
        delete _activeTool;
        _activeTool = 0;
        drawAll();
    }
}

void DisplayArea::setupPainter( QPainter& painter )
{
    painter.setPen( QPen( Qt::black, 3 ) );
}

DrawList DisplayArea::drawList() const
{
    return _drawings;
}

void DisplayArea::setDrawList( const DrawList& list )
{
    _drawings.setWidget( 0 );
    _drawings = list;
    _drawings.setWidget( this );
    drawAll();
}

void DisplayArea::stopDrawings()
{
    _activeTool = 0;
    _tool = None;
    drawAll();
}

void DisplayArea::toggleShowDrawings( bool b )
{
    Options::instance()->setShowDrawings( b );
    drawAll();
}

void DisplayArea::setImage( ImageInfo* info )
{
    _info = info;
    ImageManager::instance()->load( info->fileName( false ), this, 0, -1,  -1, false, true );
}

void DisplayArea::pixmapLoaded( const QString&, int, int, int, const QImage& image )
{
    _currentImage= image;
    QImage img = ImageLoader::rotateAndScale( _currentImage, width(), height(), _info->angle() );

    QPixmap pixmap;
    pixmap.convertFromImage( img );
    setPixmap( pixmap );
}

void DisplayArea::resizeEvent( QResizeEvent* )
{
    if ( !_currentImage.isNull() ) {
        QImage img = ImageLoader::rotateAndScale( _currentImage, width(), height(), _info->angle() );

        QPixmap pixmap;
        pixmap.convertFromImage( img );
        setPixmap( pixmap );
    }
}

#include "displayarea.moc"
