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

#include "speeddisplay.h"
#include <qlayout.h>
#include <qlabel.h>
#include <qtimer.h>

#include <klocale.h>

SpeedDisplay::SpeedDisplay( QWidget* parent, const char* name )
    :QDialog( parent, name, false, WStyle_Customize | WStyle_NoBorder | WX11BypassWM| WStyle_StaysOnTop )
{
    _label = new QLabel( this );
    _layout = new QHBoxLayout( this );
    _layout->addWidget( _label );
    _timer = new QTimer( this );
    connect( _timer, SIGNAL( timeout() ), this, SLOT( hide() ) );

    _label->setPaletteBackgroundColor( yellow );
    _label->setFrameStyle( QFrame::Box | QFrame::Plain );
}

void SpeedDisplay::display( int i )
{
    _label->setText( i18n("<qt><center><font size=\"+4\">%1&nbsp;s</font></center></qt>").arg( QString::number( i/1000.0, 'f', 1 ) ) );
    go();
}

void SpeedDisplay::start( )
{
    _label->setText( i18n("<qt><center><font size=\"+4\">Starting Slideshow</font></center></qt>"));
    go();
}

void SpeedDisplay::go()
{
    _layout->invalidate();
    resize( sizeHint() );
    QWidget* p = static_cast<QWidget*>( parent() );
    move( ( p->width() - width() )/2 + p->x(), ( p->height() - height() )/2 + p->y() );
    show();
    _timer->start( 1000 );
}

void SpeedDisplay::end()
{
    _label->setText( i18n("<qt><center><font size=\"+4\">Ending Slideshow</font></center></qt>") );
    go();
}

#include "speeddisplay.moc"
