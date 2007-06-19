/* Copyright (C) 2003-2006 Jesper K. Pedersen <blackie@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef EXIF_H
#define EXIF_H
#include <qmap.h>
#include <qstringlist.h>
#include "Utilities/Set.h"
#include <exiv2/exif.hpp>
#include <exiv2/iptc.hpp>
#include "Utilities/Util.h"

namespace Exif {

class Info {
public:
    Info();
    static Info* instance();
    QMap<QString, QString> info( const QString& fileName, Set<QString> wantedKeys, bool returnFullExifName, Utilities::IptcCharset charset );
    QMap<QString, QString> infoForViewer( const QString& fileName, bool returnFullExifName = false );
    QMap<QString, QString> infoForDialog( const QString& fileName, Utilities::IptcCharset charset );
    Set<QString> availableKeys();
    Set<QString> standardKeys();
    void writeInfoToFile( const QString& srcName, const QString& destName );
    Exiv2::ExifData exifData( const QString& fileName );
    Exiv2::IptcData iptcData( const QString& fileName );

protected:
    QString exifInfoFile( const QString& fileName );

private:
    static Info* _instance;
    Set<QString> _keys;
};

}

#endif /* EXIF_H */

