/* Copyright (C) 2012 Jesper K. Pedersen <blackie@kde.org>

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

#ifndef VIDEOTHUMBNAILCYCLER_H
#define VIDEOTHUMBNAILCYCLER_H

#include <QObject>
#include <DB/Id.h>

class QTimer;

namespace ImageManager {
    class VideoThumbnails;
}
namespace ThumbnailView {
    class ThumbnailModel;

class VideoThumbnailCycler : public QObject
{
    Q_OBJECT
public:
    explicit VideoThumbnailCycler(ThumbnailModel* model, QObject *parent = 0);
    void setActiveId( const DB::Id& id );

private slots:
    void updateThumbnail();
    void gotFrame(const QImage& image );

private:
    void resetPreviousThumbail();
    bool isVideo( const DB::Id& id ) const;
    QString fileNameForId( const DB::Id& ) const;
    void startCycle();
    void stopCycle();

    DB::Id m_id;
    int m_index;
    QTimer* m_timer;
    ImageManager::VideoThumbnails *m_thumbnails;
    ThumbnailModel* m_model;
};

}
#endif // VIDEOTHUMBNAILCYCLER_H