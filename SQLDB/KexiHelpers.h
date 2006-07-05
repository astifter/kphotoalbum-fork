/*
  Copyright (C) 2006 Tuomas Suutari <thsuut@utu.fi>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program (see the file COPYING); if not, write to the
  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
  MA 02110-1301 USA.
*/

#ifndef KEXIHELPERS_H
#define KEXIHELPERS_H

#include <qstringlist.h>
#include <qvariant.h>
#include <kexidb/cursor.h>
#include <kexidb/field.h>

namespace SQLDB
{

QStringList readStringsFromCursor(KexiDB::Cursor& cursor, int col=0);
QValueList<QString[2]> readString2sFromCursor(KexiDB::Cursor& cursor);
QValueList<QString[3]> readString3sFromCursor(KexiDB::Cursor& cursor);
QValueList<int> readIntsFromCursor(KexiDB::Cursor& cursor, int col=0);
KexiDB::Field::Type fieldTypeFor(QVariant qv);

}

#endif /* KEXIHELPERS_H */
