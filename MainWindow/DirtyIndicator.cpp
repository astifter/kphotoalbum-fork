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
#include "DirtyIndicator.h"
#include <kiconloader.h>
#include <QPixmap>
#include <QLabel>
#include <QDebug>

static MainWindow::DirtyIndicator* _instance = nullptr;
bool MainWindow::DirtyIndicator::_autoSaveDirty = false;
bool MainWindow::DirtyIndicator::_saveDirty = false;
bool MainWindow::DirtyIndicator::_suppressMarkDirty = false;

MainWindow::DirtyIndicator::DirtyIndicator( QWidget* parent )
    :QLabel( parent )
{
    _dirtyPix = QPixmap( SmallIcon( QString::fromLatin1( "media-floppy" ) ) );
    setFixedWidth( _dirtyPix.width() + 10);
    _instance = this;

    // Might have been marked dirty even before the indicator had been created, by the database searching during loading.
    if ( _saveDirty )
        markDirty();
}

void MainWindow::DirtyIndicator::suppressMarkDirty(bool state)
{
    MainWindow::DirtyIndicator::_suppressMarkDirty = state;
}

void MainWindow::DirtyIndicator::markDirty()
{
    if (MainWindow::DirtyIndicator::_suppressMarkDirty) {
        return;
    }

    if ( _instance ) {
        _instance->markDirtySlot();
    } else {
        _saveDirty = true;
        _autoSaveDirty = true;
    }
}

void MainWindow::DirtyIndicator::markDirtySlot() {
    if (MainWindow::DirtyIndicator::_suppressMarkDirty) {
        return;
    }

    _saveDirty = true;
    _autoSaveDirty = true;
    setPixmap( _dirtyPix );
    emit dirty();
}

void MainWindow::DirtyIndicator::autoSaved()
{
    _autoSaveDirty= false;
}

void MainWindow::DirtyIndicator::saved()
{
    _autoSaveDirty = false;
    _saveDirty = false;
    setPixmap( QPixmap() );
}

bool MainWindow::DirtyIndicator::isSaveDirty() const
{
    return _saveDirty;
}

bool MainWindow::DirtyIndicator::isAutoSaveDirty() const
{
    return _autoSaveDirty;
}


#include "DirtyIndicator.moc"
// vi:expandtab:tabstop=4 shiftwidth=4:
