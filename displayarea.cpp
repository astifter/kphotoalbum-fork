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
#include "viewhandler.h"
#include "drawhandler.h"
#include <qwmatrix.h>
#include <qlabel.h>

DisplayArea::DisplayArea( QWidget* parent, const char* name )
    :QWidget( parent, name )
{
    setBackgroundMode( NoBackground );

    _viewHandler = new ViewHandler( this );
    _drawHanler = new DrawHandler( this );
    _currentHandler = _viewHandler;

    connect( _drawHanler, SIGNAL( redraw() ), this, SLOT( drawAll() ) );
}

void DisplayArea::mousePressEvent( QMouseEvent* event )
{
    QMouseEvent e( event->type(), mapPos( event->pos() ), event->button(), event->state() );
    bool block = _currentHandler->mousePressEvent( &e  );
    if ( !block )
        QWidget::mousePressEvent( event );
    update();
}

void DisplayArea::mouseMoveEvent( QMouseEvent* event )
{
    QMouseEvent e( event->type(), mapPos( event->pos() ), event->button(), event->state() );
    bool block = _currentHandler->mouseMoveEvent( &e );
    if ( !block )
        QWidget::mousePressEvent( event );
    update();
}

void DisplayArea::mouseReleaseEvent( QMouseEvent* event )
{
    QMouseEvent e( event->type(), mapPos( event->pos() ), event->button(), event->state() );
    bool block = _currentHandler->mouseReleaseEvent( &e );
    if ( !block )
        QWidget::mousePressEvent( event );
    drawAll();
    update();
}

void DisplayArea::drawAll()
{
    if ( _loadedPixmap.isNull() )
        return;

    _drawingPixmap = _loadedPixmap;
    if ( Options::instance()->showDrawings() )
         _drawHanler->drawAll( _drawingPixmap );
    _viewPixmap = _drawingPixmap;
    update();
}

void DisplayArea::startDrawing()
{
    _currentHandler = _drawHanler;
}

void DisplayArea::stopDrawing()
{
    _drawHanler->stopDrawing();
    _currentHandler = _viewHandler;
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
    _loadedPixmap = image;
    _zStart = QPoint(0,0);
    _zEnd = QPoint( image.width(), image.height() );
    drawAll();
    update();
}

void DisplayArea::resizeEvent( QResizeEvent* )
{
    drawAll();
}

DrawHandler* DisplayArea::drawHandler()
{
    return _drawHanler;
}

QPainter* DisplayArea::painter()
{
    _viewPixmap = _drawingPixmap;
    QPainter* p = new QPainter( &_viewPixmap );
    return p;
}

void DisplayArea::paintEvent( QPaintEvent* )
{
    QPixmap tmp( QABS( _zEnd.x()-_zStart.x() ), QABS( _zEnd.y()-_zStart.y() ) );
    bitBlt( &tmp, QPoint(0,0), &_viewPixmap, QRect( _zStart, _zEnd ) );

    QPainter p(this);
    p.setWindow( QRect( QPoint(0,0), _zEnd-_zStart) );
    p.drawPixmap( 0,0, tmp );
}

QPixmap DisplayArea::scalePixmap( QPixmap pix, int width, int height )
{
    double pixWidth = pix.width();
    double pixHeight = pix.height();
    double ratio = width/pixWidth;

    if ( ratio * pixHeight > height ) {
        ratio = height/pixHeight;
        Q_ASSERT( ratio*pixWidth <= width );
    }

    QWMatrix matrix;
    matrix.scale( ratio, ratio );

    int ox = (int) ((width - pixWidth*ratio)/ratio)/2;
    int oy = (int) ((height - pixHeight*ratio)/ratio)/2;

    QPixmap res( width, height );
    res.fill( black );
    QPainter p(&res );
    p.setWorldMatrix( matrix );
    p.drawPixmap( ox, oy, pix );
    return res;
}


void DisplayArea::zoom( const QPoint& p1, const QPoint& p2 )
{
    _zStart = p1;
    _zEnd = p2;
}

QPoint DisplayArea::mapPos( const QPoint& p )
{
    int x = (int) (_zStart.x() + (_zEnd.x()-_zStart.x())*((double)p.x()/ width()));
    int y = (int) (_zStart.y() + (_zEnd.y()-_zStart.y())*((double)p.y()/ height()));
    return QPoint( x, y );
}

#include "displayarea.moc"
