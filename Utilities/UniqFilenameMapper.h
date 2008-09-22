/* Copyright (C) 2008 Henner Zeller <h.zeller@acm.org>

   based on Utilities::createUniqNameMap() by <blackie@kde.org>

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
#ifndef UTILITIES_UNIQ_FILENAME_MAPPER_H
#define UTILITIES_UNIQ_FILENAME_MAPPER_H

#include <QMap>
#include <QSet>
#include <QString>

namespace Utilities {

/**
 * The UniqFilenameMapper creates flat filenames from arbitrary input filenames
 * so that there are no conflicts if they're written in one directory together.
 * The resulting names do not contain any path unless a targetDirectory is
 * given in the constructor.
 *
 * Example:
 * uniqNameFor("cd1/def.jpg")      -> def.jpg
 * uniqNameFor("cd1/abc/file.jpg") -> file.jpg
 * uniqNameFor("cd3/file.jpg")     -> file-1.jpg
 * uniqNameFor("cd1/abc/file.jpg") -> file.jpg    // file from above.
 */
class UniqFilenameMapper {
public:
    UniqFilenameMapper();

    // Create a UniqFilenameMapper that returns filenames with the
    // targetDirectory prepended.
    // The UniqFilenameMapper makes sure, that generated filenames do not
    // previously exist in the targetDirectory.
    explicit UniqFilenameMapper(const QString &targetDirectory);

    // Create a unique, flat filename for the target directory. If this method
    // has been called before with the same argument, the uniq name that has
    // been created before is returned (see example above).
    QString uniqNameFor(const QString& filename);

    // Reset all mappings.
    void reset();

private:
    UniqFilenameMapper(const UniqFilenameMapper&);  // don't copy.

    bool fileClashes(const QString& file);

    const QString _targetDirectory;
    typedef QMap<QString, QString> FileNameMap;
    FileNameMap _origToUniq;
    QSet<QString> _uniqFiles;
};
}
#endif /* UTILITIES_UNIQ_FILENAME_MAPPER_H */