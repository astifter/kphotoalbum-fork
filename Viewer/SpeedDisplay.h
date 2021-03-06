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

#ifndef SPEEDDISPLAY_H
#define SPEEDDISPLAY_H
#include <qdialog.h>
#include <QLabel>
class QTimeLine;
class QTimer;
class QLabel;
class QHBoxLayout;

namespace Viewer
{

class SpeedDisplay :public QLabel {
    Q_OBJECT

public:
    explicit SpeedDisplay( QWidget* parent );
    void display( int );
    void start();
    void end();
    void go();

private slots:
    void setAlphaChannel(int alpha);
    void setAlphaChannel(int background, int label);

private:
    QTimer* m_timer;
    QTimeLine* m_timeLine;
};

}

#endif /* SPEEDDISPLAY_H */

// vi:expandtab:tabstop=4 shiftwidth=4:
