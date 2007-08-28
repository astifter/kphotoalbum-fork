/*
  Copyright (C) 2006-2007 Tuomas Suutari <thsuut@utu.fi>

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

#include "QueryHelper.h"
#include "QueryErrors.h"
#include "Viewer/CircleDraw.h"
#include "Viewer/LineDraw.h"
#include "Viewer/RectDraw.h"
#include "DB/ImageSearchInfo.h"
#include "DB/CategoryMatcher.h"
#include "Utilities/List.h"
#include "TransactionGuard.h"
#include <klocale.h>
#include <qsize.h>
#include <QList>
#include <QSqlField>
#include <QSqlDriver>
#include <QSqlError>
#include <memory>

using namespace SQLDB;
using Utilities::mergeListsUniqly;
using Utilities::listSubtract;

QueryHelper::QueryHelper(DatabaseConnection& connection):
    _database(connection),
    _driver(_database.driver())
{
    if (!_driver)
        throw InitializationError();
}

QString QueryHelper::variantListAsSql(const QList<QVariant>& l) const
{
    QStringList repr;
    QSqlField f;
    for (QList<QVariant>::const_iterator i = l.begin();
         i != l.end(); ++i) {
        f.setType(i->type());
        f.setValue(*i);
        repr << _driver->formatValue(f);
    }
    if (repr.isEmpty())
        return QLatin1String("NULL");
    else
        return repr.join(QLatin1String(","));
}

void QueryHelper::processListParameters(QString& query,
                                        Bindings& bindings) const
{
    int mark = 0;
    Bindings::iterator i = bindings.begin();
    while (i != bindings.end()) {
        mark = query.indexOf('?', mark);
        if (mark == -1)
            break;

        if (i->type() == QVariant::List) {
            QString sql = variantListAsSql(i->toList());
            query.replace(mark, 1, sql);
            i = bindings.erase(i);
            mark += sql.length();
        }
        else {
            ++i;
            ++mark;
        }
    }
}

void QueryHelper::bindValues(QSqlQuery& query, const Bindings& b) const
{
    int n = 0;
    for (Bindings::const_iterator i = b.begin(); i != b.end(); ++i) {
        query.bindValue(n, *i);
        ++n;
    }
}

std::auto_ptr<QSqlQuery>
QueryHelper::initializeQuery(const QString& statement,
                             const Bindings& bindings) const
{
    QString queryStr(statement);
    Bindings b(bindings);

    processListParameters(queryStr, b);

    std::auto_ptr<QSqlQuery> query(new QSqlQuery(queryStr, _database));
    bindValues(*query, b);

    return query;
}


void QueryHelper::executeStatement(const QString& statement,
                                   const Bindings& bindings)
{
    std::auto_ptr<QSqlQuery> s(initializeQuery(statement, bindings));

    if (!s->exec())
        throw QtSQLError(s->lastError());

    //TODO: remove debug
    qDebug("Executed statement: %s", s->executedQuery().toLocal8Bit().data());
}

QueryResult QueryHelper::executeQuery(const QString& query,
                                      const Bindings& bindings) const
{
    std::auto_ptr<QSqlQuery> q(initializeQuery(query, bindings));

#ifdef DEBUG_QUERY_TIMES
    QTime t;
    t.start();
#endif

    if (!q->exec())
        throw QtSQLError(q->lastError());

#ifdef DEBUG_QUERY_TIMES
    int te = t.elapsed();
    if (te > 100) {
        {
            queryTimes.push_back(QPair<QString, uint>(q->executedQuery(), te));
        }
    }
    qDebug("Time elapsed: %d ms", te);
#endif

    //TODO: remove debug
    qDebug("Executing query: %s", q->executedQuery().toLocal8Bit().data());

    return QueryResult(q);
}

qulonglong QueryHelper::insert(const QString& tableName,
                               const QString& aiFieldName,
                               const QStringList& fields,
                               const Bindings& values)
{
    Q_ASSERT(fields.count() == values.count());

    QString q = "INSERT INTO %1(%2) VALUES (%3)";
    q = q.arg(tableName);
    q = q.arg(fields.join(", "));
    QStringList l;
    for (Bindings::size_type i = values.count(); i > 0; --i)
        l.append("?");
    q = q.arg(l.join(", "));


    std::auto_ptr<QSqlQuery> s(initializeQuery(q, values));

    if (!s->exec())
        throw QtSQLError(s->lastError());

    //TODO: remove debug
    qDebug("Executed statement: %s", s->executedQuery().toLocal8Bit().data());

    Q_UNUSED(aiFieldName);

    return s->lastInsertId().toULongLong();
}

namespace
{
void splitPath(const QString& filename, QString& path, QString& basename)
{
    int i = filename.lastIndexOf("/");
    if (i == -1) {
        path = ".";
        basename = filename;
    }
    else {
        // FIXME: anything special needed if filename is form "/foo.jpg"?
        path = filename.left(i);
        basename = filename.mid(i + 1);
    }
}

QString makeFullName(const QString& path, const QString& basename)
{
    if (path == ".")
        return basename;
    else
        return path + "/" + basename;
}

QString typeCondition(const QString& fieldName, DB::MediaType typemask)
{
    if (typemask == DB::Image ||
        typemask == DB::Video)
        return fieldName + "=" + QString::number(typemask);
    else if (typemask == DB::anyMediaType)
        return "1=1";
    else {
        QStringList typeList;
        for (uint t = 1; t < DB::anyMediaType; t <<= 1)
            if (typemask & t)
                typeList.append(fieldName + "=" + QString::number(t));
        return "(" + typeList.join(" OR ") + ")";
    }
}
}

QStringList QueryHelper::filenames() const
{
    StringStringList dirFilePairs =
        executeQuery("SELECT dir.path, media.filename FROM dir, media "
                     "WHERE media.dirId=dir.id ORDER BY media.place"
                     ).asStringStringList();
    QStringList r;
    for (StringStringList::const_iterator i = dirFilePairs.begin();
         i != dirFilePairs.end(); ++i) {
        r << makeFullName((*i).first, (*i).second);
    }
    return r;
}

QString QueryHelper::mediaItemFilename(int id) const
{
    StringStringList dirFilePairs =
        executeQuery("SELECT dir.path, media.filename FROM dir, media "
                     "WHERE dir.id=media.dirId AND media.id=?",
                     Bindings() << id).asStringStringList();
    if (dirFilePairs.isEmpty())
        throw EntryNotFoundError();

    return makeFullName(dirFilePairs[0].first, dirFilePairs[0].second);
}

int QueryHelper::mediaItemId(const QString& filename) const
{
    QString path;
    QString basename;
    splitPath(filename, path, basename);
    QVariant id =
        executeQuery("SELECT media.id FROM media, dir "
                     "WHERE media.dirId=dir.id AND "
                     "dir.path=? AND media.filename=?",
                     Bindings() << path << basename).firstItem();
    if (id.isNull())
        throw EntryNotFoundError();
    return id.toInt();
}

QList< QPair<int, QString> > QueryHelper::mediaItemIdFileMap() const
{
    Cursor c = executeQuery("SELECT media.id, dir.path, media.filename "
                            "FROM media, dir "
                            "WHERE media.dirId=dir.id").cursor();
    QList< QPair<int, QString> > r;

    if (c.isNull())
        return r;

    for (c.selectFirstRow(); c.rowExists(); c.selectNextRow()) {
        QPair<int, QString> x;
        x.first = c.value(0).toInt();
        x.second = makeFullName(c.value(1).toString(), c.value(2).toString());
        r << x;
    }
    return r;
}

QList<int>
QueryHelper::mediaItemIdsForFilenames(const QStringList& filenames) const
{
#if 1
    QStringList paths;
    QStringList basenames;
    QMap<QString, int> pathIdMap;
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i) {
        QString path;
        QString basename;
        splitPath(*i, path, basename);
        paths << path;
        basenames << basename;
        if (!pathIdMap.contains(path)) {
            pathIdMap.insert(path, executeQuery("SELECT id FROM dir "
                                                "WHERE path=?",
                                                Bindings() << path
                                                ).firstItem().toInt());
        }
    }
    QList<int> idList;
    QStringList::const_iterator pathIt = paths.begin();
    QStringList::const_iterator basenameIt = basenames.begin();
    while (pathIt != paths.end()) {
        QVariant id =
            executeQuery("SELECT id FROM media WHERE dirId=? AND filename=?",
                         Bindings() << pathIdMap[*pathIt] << *basenameIt
                         ).firstItem();
        /*
        if (id.isNull())
            throw EntryNotFoundError();
        */
        if (!id.isNull())
            idList << id.toInt();
        ++pathIt;
        ++basenameIt;
    }
    return idList;
#else
    // Uses CONCAT function, which is MySQL specific.

    QStringList adjustedPaths = filenames;
    for (QStringList::iterator i = adjustedPaths.begin(); i != adjustedPaths.end(); ++i) {
        if ((*i).find('/') == -1) {
            *i = QString::fromLatin1("./") + *i;
        }
    }
    executeQuery("SELECT id FROM media, dir WHERE CONCAT(dir.path, '/', media.filename) IN (?)");
#endif
}

QStringList QueryHelper::categoryNames() const
{
    return executeQuery("SELECT name FROM category").asStringList();
}

int QueryHelper::categoryId(const QString& category) const
{
    QVariant r = executeQuery("SELECT id FROM category WHERE name=?",
                              Bindings() << category).firstItem();
    if (r.isNull())
        throw EntryNotFoundError();
    else
        return r.toInt();
}

QList<int> QueryHelper::tagIdsOfCategory(const QString& category) const
{
    return executeQuery("SELECT tag.id FROM tag, category "
                        "WHERE tag.categoryId=category.id AND "
                        "category.name=?",
                        Bindings() << category).asIntegerList();
}

QStringList QueryHelper::tagNamesOfCategory(int categoryId) const
{
    // Tags with larger place come first. (NULLs last)
    return executeQuery("SELECT name FROM tag "
                        "WHERE categoryId=? ORDER BY place DESC",
                        Bindings() << categoryId).asStringList();
}

QStringList QueryHelper::folders() const
{
    return executeQuery("SELECT path FROM dir").asStringList();
}

uint QueryHelper::mediaItemCount(DB::MediaType typemask,
                                 QList<int>* scope) const
{
    if (!scope) {
        if (typemask == DB::anyMediaType)
            return executeQuery("SELECT COUNT(*) FROM media"
                                ).firstItem().toUInt();
        else
            return executeQuery("SELECT COUNT(*) FROM media WHERE " +
                                typeCondition("type", typemask)
                                ).firstItem().toUInt();
    }
    else {
        if (scope->isEmpty())
            return 0; // Empty scope contains no media items
        else {
            if (typemask == DB::anyMediaType) {
                return executeQuery("SELECT COUNT(*) FROM media "
                                    "WHERE id IN (?)",
                                    Bindings() << toVariantList(*scope)
                                    ).firstItem().toUInt();
            }
            else {
                return executeQuery("SELECT COUNT(*) FROM media WHERE " +
                                    typeCondition("type", typemask) +
                                    " AND id IN (?)",
                                    Bindings() << toVariantList(*scope)
                                    ).firstItem().toUInt();
            }
        }
    }
}

QList<int> QueryHelper::mediaItemIds(DB::MediaType typemask) const
{
    if (typemask == DB::anyMediaType)
        return executeQuery("SELECT id FROM media ORDER BY place"
                            ).asIntegerList();
    else
        return executeQuery("SELECT id FROM media "
                            "WHERE " + typeCondition("type", typemask) +
                            " ORDER BY place").asIntegerList();
}

void QueryHelper::getMediaItem(int id, DB::ImageInfo& info) const
{
    RowData row;
    try {
        row =
            executeQuery("SELECT dir.path, m.filename, m.md5sum, m.type, "
                         "m.label, m.description, "
                         "m.startTime, m.endTime, m.width, m.height, m.angle "
                         "FROM media m, dir "
                         "WHERE m.dirId=dir.id AND "
                         "m.id=?", Bindings() << id).getRow();
    }
    catch (RowNotFoundError&) {
        throw EntryNotFoundError();
    }

    Q_ASSERT(row.count() == 11);

    info.delaySavingChanges(true);

    info.setFileName(makeFullName(row[0].toString(), row[1].toString()));
    info.setMD5Sum(DB::MD5(row[2].toString()));
    info.setMediaType(static_cast<DB::MediaType>(row[3].toInt()));
    info.setLabel(row[4].toString());
    info.setDescription(row[5].toString());
    QDateTime startDate = row[6].toDateTime();
    QDateTime endDate = row[7].toDateTime();
    info.setDate(DB::ImageDate(startDate, endDate));
    int width = row[8].isNull() ? -1 : row[8].toInt();
    int height = row[9].isNull() ? -1 :row[9].toInt();
    info.setSize(QSize(width, height));
    info.setAngle(row[10].toInt());

    StringStringList categoryTagPairs =
        executeQuery("SELECT category.name, tag.name "
                     "FROM media_tag, category, tag "
                     "WHERE media_tag.tagId=tag.id AND "
                     "category.id=tag.categoryId AND "
                     "media_tag.mediaId=?", Bindings() << id).asStringStringList();

    for (StringStringList::const_iterator i = categoryTagPairs.begin();
         i != categoryTagPairs.end(); ++i)
        info.addCategoryInfo((*i).first, (*i).second);

    Viewer::DrawList drawList;
    Cursor c = executeQuery("SELECT shape, x0, y0, x1, y1 "
                            "FROM drawing WHERE mediaId=?",
                            Bindings() << id).cursor();
    for (c.selectFirstRow(); c.rowExists(); c.selectNextRow()) {
        Viewer::Draw* drawing;
        switch (c.value(0).toInt()) {
        case 0:
            drawing = new Viewer::CircleDraw();
            break;
        case 1:
            drawing = new Viewer::LineDraw();
            break;
        case 2:
            drawing = new Viewer::RectDraw();
            break;
        default:
            continue;
        }
        drawing->setCoordinates(QPoint(c.value(1).toInt(),
                                       c.value(2).toInt()),
                                QPoint(c.value(3).toInt(),
                                       c.value(4).toInt()));
        drawList << drawing;
    }
    info.setDrawList(drawList);

    // TODO: remove debug
    qDebug("Read info of file %s (id %d)", info.fileName().toLocal8Bit().data(), id);
}

int QueryHelper::insertTag(int categoryId, const QString& name)
{
    QVariant i = executeQuery("SELECT id FROM tag "
                              "WHERE categoryId=? AND name=?",
                              Bindings() << categoryId << name).firstItem();
    if (!i.isNull())
        return i.toInt();

    return insert("tag", "id", QStringList() << "categoryId" << "name",
                  Bindings() << categoryId << name);
}

void QueryHelper::insertTagFirst(int categoryId, const QString& name)
{
    // See membersOfCategory() for tag ordering usage.

    int id = insertTag(categoryId, name);
    QVariant oldPlace = executeQuery("SELECT place FROM tag WHERE id=?",
                                     Bindings() << id).firstItem();

    // Move tags from this to previous first tag one place towards end
    if (!oldPlace.isNull()) {
        executeStatement("UPDATE tag SET place=place-1 "
                         "WHERE categoryId=? AND place>?",
                         Bindings() << categoryId << oldPlace);
    }

    // MAX(place) could be NULL, but it'll be returned as 0 with
    // toInt(), which is ok.
    int newPlace = executeQuery("SELECT MAX(place) FROM tag "
                                "WHERE categoryId=?",
                                Bindings() << categoryId
                                ).firstItem().toInt() + 1;

    // Put this tag first
    executeStatement("UPDATE tag SET place=? WHERE id=?",
                     Bindings() << newPlace << id);
}

void QueryHelper::removeTag(int categoryId, const QString& name)
{
#ifndef USEFOREIGNCONSTRAINT
    int tagId =
        executeQuery("SELECT id FROM tag WHERE categoryId=? AND name=?",
                     Bindings() << categoryId << name).firstItem().toInt();
    executeStatement("DELETE FROM media_tag WHERE tagId=?",
                     Bindings() << tagId);
    executeStatement("DELETE FROM membergroup "
                     "WHERE groupTag=? OR memberTag=?",
                     Bindings() << tagId << tagId);
#endif
    executeStatement("DELETE FROM tag WHERE categoryId=? AND name=?",
                     Bindings() << categoryId << name);
}

void QueryHelper::insertMediaTag(int mediaId, int tagId)
{
    if (executeQuery("SELECT COUNT(*) FROM media_tag "
                     "WHERE mediaId=? AND tagId=?",
                     Bindings() << mediaId << tagId).firstItem().toUInt() > 0)
        return;
    executeStatement("INSERT INTO media_tag (mediaId, tagId) "
                     "VALUES (?, ?)", Bindings() << mediaId << tagId);
}

void QueryHelper::insertMediaItemTags(int mediaId, const DB::ImageInfo& info)
{
    QStringList categories = info.availableCategories();
    for (QStringList::const_iterator categoryIt = categories.begin();
         categoryIt != categories.end(); ++categoryIt) {
        if (*categoryIt == "Folder")
            continue;
        int cid = categoryId(*categoryIt);
        StringSet items = info.itemsOfCategory(*categoryIt);
        for (StringSet::const_iterator itemIt = items.begin();
             itemIt != items.end(); ++itemIt) {
            qulonglong tagId = insertTag(cid, *itemIt);
            insertMediaTag(mediaId, tagId);
        }
    }
}

void QueryHelper::insertMediaItemDrawings(int mediaId,
                                          const DB::ImageInfo& info)
{
    Viewer::DrawList drawings = info.drawList();
    for (Viewer::DrawList::const_iterator i = drawings.begin();
        i != drawings.end(); ++i) {
        int shape;
        if (dynamic_cast<Viewer::CircleDraw*>(*i)) {
            shape = 0;
        }
        else if (dynamic_cast<Viewer::LineDraw*>(*i)) {
            shape = 1;
        }
        else if (dynamic_cast<Viewer::RectDraw*>(*i)) {
            shape = 2;
        }
        else
            continue;

        QPoint p0 = (*i)->getStartPoint();
        QPoint p1 = (*i)->getEndPoint();

        executeStatement("INSERT INTO drawing "
                         "(mediaId, shape, x0, y0, x1, y1) "
                         "VALUES (?, ?, ?, ?, ?, ?)",
                         Bindings() << mediaId << shape <<
                         p0.x() << p0.y() << p1.x() << p1.y());
    }
}

int QueryHelper::insertDir(const QString& dirname)
{
    QVariant i = executeQuery("SELECT id FROM dir "
                              "WHERE path=?",
                              Bindings() << dirname).firstItem();
    if (!i.isNull())
        return i.toInt();

    return insert("dir", "id", QStringList() << "path",
                  Bindings() << dirname);
}

/** Get data from ImageInfo to Bindings list.
 *
 * Adds also image directory to database, if it's not already there.
 *
 * @return Bingings in following order:
 *  dirId, filename, md5sum, type, label, description,
 *  startTime, endTime, width, height, angle
 */
QueryHelper::Bindings
QueryHelper::imageInfoToBindings(const DB::ImageInfo& info)
{
    //Q_ASSERT(bindings.isEmpty());

    QVariant md5;
    if (!info.MD5Sum().isNull())
        md5 = info.MD5Sum().toHexString();
    QVariant w = info.size().width();
    if (w.toInt() == -1)
        w = QVariant();
    QVariant h =  info.size().height();
    if (h.toInt() == -1)
        h = QVariant();
    QString path;
    QString filename;
    splitPath(info.fileName(true), path, filename);

    int dirId = insertDir(path);

    return Bindings() <<
        dirId << filename << md5 <<
        info.mediaType() << info.label() <<
        info.description() <<
        info.date().start() << info.date().end() <<
        w << h << info.angle();
}

void QueryHelper::insertMediaItem(const DB::ImageInfo& info, int place)
{
    Bindings infoValues = imageInfoToBindings(info);

    if (executeQuery("SELECT COUNT(*) FROM media "
                     "WHERE dirId=? AND filename=?",
                     Bindings() << infoValues[0] << infoValues[1]
                     ).firstItem().toUInt() != 0)
        return;

    // TODO: remove debug
    qDebug("Inserting info of file %s", info.fileName().toLocal8Bit().data());

    QStringList fields;
    Bindings bindings;
    if (place != 0) {
        fields << "place";
        bindings << place;
    }
    fields <<
        "dirId" << "filename" << "md5sum" <<
        "type" << "label" <<
        "description" <<
        "startTime" << "endTime" <<
        "width" << "height" << "angle";
    bindings += infoValues;

    qulonglong mediaId = insert("media", "id", fields, bindings);

    insertMediaItemTags(mediaId, info);
    insertMediaItemDrawings(mediaId, info);
}

void
QueryHelper::insertMediaItemsLast(const QList<DB::ImageInfoPtr>& items)
{
    TransactionGuard transaction(_database);

    int place =
        executeQuery("SELECT MAX(place) FROM media").firstItem().toInt() + 1;

    for (QList<DB::ImageInfoPtr>::const_iterator i = items.constBegin();
         i != items.constEnd(); ++i) {
        insertMediaItem(*(*i), place++);
    }

    transaction.commit();
}

void QueryHelper::updateMediaItem(int id, const DB::ImageInfo& info)
{
    // TODO: remove debug
    qDebug("Updating info of file %s", info.fileName().toLocal8Bit().data());

    executeStatement("UPDATE media SET dirId=?, filename=?, md5sum=?, "
                     "type=?, label=?, description=?, "
                     "startTime=?, endTime=?, "
                     "width=?, height=?, angle=? WHERE id=?",
                     imageInfoToBindings(info) << id);

    executeStatement("DELETE FROM media_tag WHERE mediaId=?",
                     Bindings() << id);
    insertMediaItemTags(id, info);

    executeStatement("DELETE FROM drawing WHERE mediaId=?",
                     Bindings() << id);
    insertMediaItemDrawings(id, info);
}

QList<int> QueryHelper::directMembers(int tagId) const
{
    return executeQuery("SELECT memberTag FROM membergroup WHERE groupTag=?",
                        Bindings() << tagId).asIntegerList();
}

int QueryHelper::tagId(const QString& category, const QString& item) const
{
    QVariant id =
        executeQuery("SELECT tag.id FROM tag,category "
                     "WHERE tag.categoryId=category.id AND "
                     "category.name=? AND tag.name=?",
                     Bindings() << category << item).firstItem();
    if (id.isNull())
        throw EntryNotFoundError();
    else
        return id.toInt();
}

QList<int> QueryHelper::tagIdList(const QString& category,
                                       const QString& item) const
{
    int rootTagId = tagId(category, item);
    QList<int> visited;
    QList<int> queue;
    visited << rootTagId;
    queue << rootTagId;
    while (!queue.empty()) {
        QList<int> adj = directMembers(queue.first());
        queue.pop_front();
        QList<int>::const_iterator adjEnd(adj.constEnd());
        for (QList<int>::const_iterator a = adj.constBegin();
             a != adjEnd; ++a) {
            if (!visited.contains(*a)) { // FIXME: Slow to find
                queue << *a;
                visited << *a;
            }
        }
    }
    return visited;
}

StringStringList
QueryHelper::memberGroupConfiguration(const QString& category) const
{
    return executeQuery("SELECT g.name, m.name "
                        "FROM membergroup mg, tag g, tag m, category c "
                        "WHERE mg.groupTag=g.id AND "
                        "mg.memberTag=m.id AND "
                        "g.categoryId=c.id AND c.name=?",
                        Bindings() << category).asStringStringList();
}

void QueryHelper::addBlockItem(const QString& filename)
{
    QString path;
    QString basename;
    splitPath(filename, path, basename);
    int dirId = insertDir(path);
    if (executeQuery("SELECT COUNT(*) FROM blockitem "
                     "WHERE dirId=? AND filename=?", Bindings() <<
                     dirId << basename).firstItem().toUInt() == 0)
        executeStatement("INSERT INTO blockitem (dirId, filename) "
                         "VALUES (?, ?)", Bindings() << dirId << basename);
}

void QueryHelper::addBlockItems(const QStringList& filenames)
{
    for (QStringList::const_iterator i = filenames.begin();
         i != filenames.end(); ++i)
        addBlockItem(*i);
}

bool QueryHelper::isBlocked(const QString& filename) const
{
    QString path;
    QString basename;
    splitPath(filename, path, basename);
    return executeQuery("SELECT COUNT(*) FROM blockitem, dir "
                        "WHERE blockitem.dirId=dir.id AND "
                        "dir.path=? AND blockitem.filename=?",
                        Bindings() << path << basename
                        ).firstItem().toUInt() > 0;
}

void QueryHelper::removeMediaItem(const QString& filename)
{
    int id = mediaItemId(filename);
    executeStatement("DELETE FROM media_tag WHERE mediaId=?",
                     Bindings() << id);
    executeStatement("DELETE FROM media WHERE id=?", Bindings() << id);
}

void QueryHelper::insertCategory(const QString& name,
                                 const QString& icon, bool visible,
                                 DB::Category::ViewType type,
                                 int thumbnailSize)
{
    executeStatement("INSERT INTO category (name, icon, visible, "
                     "viewtype, thumbsize) VALUES (?, ?, ?, ?, ?)",
                     Bindings() << name << icon <<
                     QVariant(visible) <<
                     type << thumbnailSize);
}

void QueryHelper::removeCategory(const QString& name)
{
    int id = categoryId(name);
    Cursor c = executeQuery("SELECT id FROM tag WHERE categoryId=?").cursor();
    for (c.selectFirstRow(); c.rowExists(); c.selectNextRow()) {
        QVariant tagId = c.value(0);
        executeStatement("DELETE FROM media_tag WHERE tagId=?",
                         Bindings() << tagId);
        executeStatement("DELETE FROM membergroup "
                         "WHERE groupTag=? OR memberTag=?",
                         Bindings() << tagId << tagId);
    }
    executeStatement("DELETE FROM tag WHERE categoryId=?", Bindings() << id);
    executeStatement("DELETE FROM category WHERE id=?", Bindings() << id);
}

QString QueryHelper::categoryName(int id) const
{
    QVariant r = executeQuery("SELECT name FROM category WHERE id=?",
                              Bindings() << id).firstItem();

    if (r.isNull())
        throw EntryNotFoundError();
    else
        return r.toString();
}

QString QueryHelper::categoryIcon(int id) const
{
    return executeQuery("SELECT icon FROM category WHERE id=?",
                        Bindings() << id).firstItem().toString();
}

bool QueryHelper::categoryVisible(int id) const
{
    return executeQuery("SELECT visible FROM category WHERE id=?",
                        Bindings() << id).firstItem().toBool();
}

DB::Category::ViewType QueryHelper::categoryViewType(int id) const
{
    return static_cast<DB::Category::ViewType>
        (executeQuery("SELECT viewtype FROM category WHERE id=?",
                      Bindings() << id).firstItem().toUInt());
}

void QueryHelper::changeCategoryName(int id, const QString& newName)
{
    executeStatement("UPDATE category SET name=? WHERE id=?",
                     Bindings() << newName << id);
}

void QueryHelper::changeCategoryIcon(int id, const QString& icon)
{
    executeStatement("UPDATE category SET icon=? WHERE id=?",
                     Bindings() << icon << id);
}

void QueryHelper::changeCategoryVisible(int id, bool visible)
{
    executeStatement("UPDATE category SET visible=? WHERE id=?",
                     Bindings() << QVariant(visible) << id);
}

void QueryHelper::changeCategoryViewType(int id, DB::Category::ViewType type)
{
    executeStatement("UPDATE category SET viewtype=? WHERE id=?",
                     Bindings() << type << id);
}

bool QueryHelper::containsMD5Sum(const DB::MD5& md5sum) const
{
    return executeQuery("SELECT COUNT(*) FROM media WHERE md5sum=?",
                        Bindings() << md5sum.toHexString()
                        ).firstItem().toUInt() > 0;
}

QString QueryHelper::filenameForMD5Sum(const DB::MD5& md5sum) const
{
    StringStringList rows =
        executeQuery("SELECT dir.path, media.filename FROM dir, media "
                     "WHERE dir.id=media.dirId AND media.md5sum=?",
                     Bindings() << md5sum.toHexString()
                     ).asStringStringList();
    if (rows.isEmpty())
        return QString::null;
    else {
        return makeFullName(rows[0].first, rows[0].second);
    }
}

QMap<int, Set<QString> >
QueryHelper::mediaIdTagsMap(const QString& category,
                            DB::MediaType typemask) const
{
    QString q;
    Bindings b;

    if (category == "Folder") {
        q =
            "SELECT media.id, dir.path FROM media, dir "
            "WHERE media.dirId=dir.id AND " +
            typeCondition("media.type", typemask);
    }
    else {
        q =
            "SELECT media.id, tag.name "
            "FROM media, media_tag, tag, category "
            "WHERE media.id=media_tag.mediaId AND "
            "media_tag.tagId=tag.id AND "
            "tag.categoryId=category.id AND " +
            typeCondition("media.type", typemask) +
            " AND category.name=?";
        b.append(category);
    }

    Cursor c = executeQuery(q, b).cursor();

    QMap<int, Set<QString> > r;
    if (!c.isNull()) {
        for (c.selectFirstRow(); c.rowExists(); c.selectNextRow())
            r[c.value(0).toInt()].insert(c.value(1).toString());
    }
    return r;
}

int QueryHelper::mediaPlaceByFilename(const QString& filename) const
{
    QString path;
    QString basename;
    splitPath(filename, path, basename);
    QVariant place =
        executeQuery("SELECT media.place FROM media, dir "
                     "WHERE media.dirId=dir.id AND "
                     "dir.path=? AND media.filename=?",
                     Bindings() << path << basename).firstItem();
    if (place.isNull())
        throw EntryNotFoundError();
    return place.toInt();
}

/** Move media items after or before destination item.
 */
void QueryHelper::moveMediaItems(const QStringList& filenames,
                                 const QString& destinationFilename, bool after)
{

    // BROKEN!
    // TODO: make this function work!


    if (filenames.isEmpty())
        return;

    QList<int> srcIds;
    for (QStringList::const_iterator i = filenames.constBegin();
         i != filenames.constEnd(); ++i) {
        srcIds << mediaItemId(*i);
    }

    RowData minmax =
        executeQuery("SELECT MIN(place), MAX(place) "
                     "FROM media WHERE id IN (?)",
                     Bindings() << toVariantList(srcIds)).getRow();
    int srcMin = minmax[0].toInt();
    int srcMax = minmax[1].toInt();

    // Move media items which are between srcIds just before first
    // item in srcIds list.
    QList<int> betweenList =
        executeQuery("SELECT id FROM media "
                     "WHERE ? <= place AND place <= ? AND id NOT IN (?)",
                     Bindings() << srcMin << srcMax << toVariantList(srcIds)
                     ).asIntegerList();
    int n = srcMin;
    for (QList<int>::const_iterator i = betweenList.begin();
         i != betweenList.end(); ++i) {
        executeStatement("UPDATE media SET place=? WHERE id=?",
                         Bindings() << n << *i);
        ++n;
    }

    // Make items in srcIds continuous, so places of items are
    // N, N+1, N+2, ..., N+n.
    for (QList<int>::const_iterator i = srcIds.begin();
         i != srcIds.end(); ++i) {
        executeStatement("UPDATE media SET place=? WHERE id=?",
                         Bindings() << n << *i);
        ++n;
    }

    int destPlace =
        mediaPlaceByFilename(destinationFilename) + (after ? 1 : 0);

    if (srcMin <= destPlace && destPlace <= srcMax)
        return;
    int low = destPlace <= srcMin ? destPlace : srcMin;
    int high = destPlace >= srcMax ? destPlace : srcMax;

    Q_ASSERT(low == destPlace || high == destPlace);

    uint N = srcIds.count();

    if (low < destPlace)
        executeStatement("UPDATE media SET place=place-? "
                         "WHERE ? <= place AND place <= ?",
                         Bindings() << N << low << destPlace - 1);
    else if (destPlace < high)
        executeStatement("UPDATE media SET place=place+? "
                         "WHERE ? <= place AND place <= ?",
                         Bindings() << N << destPlace << high - 1);

    executeStatement("UPDATE media SET place=place+(?) WHERE id IN (?)",
                     Bindings() << destPlace - srcMin << toVariantList(srcIds));
}

void QueryHelper::makeMediaPlacesContinuous()
{
    executeStatement("UPDATE media, "
                     "(SELECT a.id AS id, COUNT(*) AS n "
                     "FROM media a, media b "
                     "WHERE b.place<a.place OR (b.place=a.place AND b.id<a.id)"
                     "GROUP BY a.id) x "
                     "SET media.place=x.n WHERE media.id=x.id");
}

void QueryHelper::sortMediaItems(const QStringList& filenames)
{
    TransactionGuard transaction(_database);

    QList<int> idList = mediaItemIdsForFilenames(filenames);

#if 0
    QList<QVariant> x = toVariantList(idList);

    executeStatement("UPDATE media, "
                     "(SELECT a.place AS place, COUNT(*) AS n "
                     " FROM media a, media b "
                     " WHERE a.id IN (?) AND b.id IN (?) AND "
                     " b.place <= a.place GROUP BY a.id) byplace, "
                     "(SELECT a.id AS id, COUNT(*) AS n "
                     " FROM media a, media b "
                     " WHERE a.id IN (?) AND b.id IN (?) AND "
                     " b.startTime <= a.startTime GROUP BY a.id) bytime "
                     "SET media.place=byplace.place "
                     "WHERE media.id=bytime.id AND byplace.n=bytime.n",
                     Bindings() << x << x << x << x);
#else
    executeStatement("CREATE TEMPORARY TABLE sorttmp "
                     "SELECT id, place, startTime "
                     "FROM media WHERE id IN (?)",
                     Bindings() << toVariantList(idList));

    idList =
        executeQuery("SELECT id FROM sorttmp "
                     "ORDER BY startTime").asIntegerList();
    QList<int> placeList =
        executeQuery("SELECT place FROM sorttmp "
                     "ORDER BY place").asIntegerList();

    QList<int>::const_iterator idIt = idList.begin();
    QList<int>::const_iterator placeIt = placeList.begin();
    for (; idIt != idList.end(); ++idIt, ++placeIt) {
        executeStatement("UPDATE sorttmp SET place=? WHERE id=?",
                         Bindings() << *placeIt << *idIt);

    }

    executeStatement("UPDATE media, sorttmp t "
                     "SET media.place=t.place WHERE media.id=t.id");

    executeStatement("DROP TABLE sorttmp");
#endif

    transaction.commit();
}

QString QueryHelper::findFirstFileInTimeRange(const DB::ImageDate& range,
                                              bool includeRanges) const
{
    return findFirstFileInTimeRange(range, includeRanges, 0);
}

QString
QueryHelper::findFirstFileInTimeRange(const DB::ImageDate& range,
                                      bool includeRanges,
                                      const QList<int>& idList) const
{
    return findFirstFileInTimeRange(range, includeRanges, &idList);
}

QString
QueryHelper::findFirstFileInTimeRange(const DB::ImageDate& range,
                                      bool includeRanges,
                                      const QList<int>* idList) const
{
    QString query =
        "SELECT dir.path, media.filename FROM media, dir "
        "WHERE dir.id=media.dirId AND ";
    Bindings bindings;

    if (idList) {
        query += "media.id IN (?) AND ";
        bindings << toVariantList(*idList);
    }

    if (!includeRanges) {
        query += "? <= media.startTime AND media.endTime <= ?";
    }
    else {
        query += "? <= media.endTime AND media.startTime <= ?";
    }
    bindings << range.start() << range.end();

    query += " ORDER BY media.startTime LIMIT 1";

    StringStringList dirFilenamePairs =
        executeQuery(query, bindings).asStringStringList();

    if (dirFilenamePairs.isEmpty())
        return QString::null;
    else {
        return makeFullName(dirFilenamePairs[0].first,
                            dirFilenamePairs[0].second);
    }
}

namespace
{
    void split(const MatcherList& input,
               MatcherList& positiveList, MatcherList& negativeList)
    {
        for (MatcherList::const_iterator it = input.constBegin();
             it != input.constEnd(); ++it) {
            DB::OptionValueMatcher* valueMatcher;
            valueMatcher = dynamic_cast<DB::OptionValueMatcher*>(*it);
            if (valueMatcher) {
                if (valueMatcher->_sign)
                    positiveList.append(valueMatcher);
                else
                    negativeList.append(valueMatcher);
            }
            else
                negativeList.append(*it);
        }
    }
}

QList<int>
QueryHelper::searchMediaItems(const DB::ImageSearchInfo& search,
                              DB::MediaType typemask) const
{
    Q_UNUSED(search);
    MatcherListList dnf /*
                          --------------------------------------
                          TEMPORARILY DISABLED
                           = search.query()
                        */;
    // dnf is in Disjunctive Normal Form ( OR(AND(a,b),AND(c,d)) )

    if (dnf.isEmpty())
        return mediaItemIds(typemask);

    QList<int> r;
    for (MatcherListList::const_iterator i = dnf.begin();
         i != dnf.end(); ++i) {
         r = mergeListsUniqly(r, getMatchingFiles(*i, typemask));
    }

    return r;
}

QList<int>
QueryHelper::getMatchingFiles(MatcherList matches,
                              DB::MediaType typemask) const
{
    MatcherList positiveList;
    MatcherList negativeList;
    split(matches, positiveList, negativeList);

    /*
    SELECT id FROM media
    WHERE
    id IN (SELECT mediaId FROM media_tag WHERE tagId IN (memberItem1TagIds))
    AND
    id IN (SELECT mediaId FROM media_tag WHERE tagId IN (memberItem2TagIds))
    AND ...
    */

    // Positive part of the query
    QStringList positiveQuery;
    QMap<QString, QList<int> > matchedTags;
    QStringList matchedFolders;
    Bindings binds;
    for (MatcherList::const_iterator i = positiveList.begin();
         i != positiveList.end(); ++i) {
        DB::OptionValueMatcher* m = static_cast<DB::OptionValueMatcher*>(*i);
        if (m->_category == "Folder") {
            positiveQuery <<
                "id IN (SELECT media.id FROM media, dir "
                "WHERE media.dirId=dir.id AND dir.path=?)";
            binds << m->_option;
            matchedFolders += m->_option;
        }
        else {
            positiveQuery <<
                "id IN (SELECT mediaId FROM media_tag WHERE tagId IN (?))";
            QList<int> tagIds = tagIdList(m->_category, m->_option);
            binds << toVariantList(tagIds);
            matchedTags[m->_category] += tagIds;
        }
    }

    // Negative query
    QStringList negativeQuery;
    QStringList excludeQuery;
    Bindings excBinds;
    for (MatcherList::const_iterator i = negativeList.begin();
         i != negativeList.end(); ++i) {
        DB::OptionValueMatcher* m = dynamic_cast<DB::OptionValueMatcher*>(*i);
        if (m) {
            if (m->_category == "Folder") {
                negativeQuery <<
                    "id NOT IN (SELECT media.id FROM media, dir "
                    "WHERE media.dirId=dir.id AND dir.path=?)";
                binds << m->_option;
            }
            else {
                negativeQuery <<
                    "id NOT IN (SELECT mediaId "
                    "FROM media_tag WHERE tagId IN (?))";
                binds << toVariantList(tagIdList(m->_category, m->_option));
            }
        }
        else {
            if ((*i)->_category == "Folder") {
                QStringList excludedFolders;
                if (!matchedFolders.isEmpty()) {
                    excludedFolders = matchedFolders;
                }
                else {
                    excludedFolders = executeQuery("SELECT path FROM dir").
                        asStringList();
                }
                if (!excludedFolders.isEmpty()) {
                    excludeQuery <<
                        "id IN (SELECT media.id FROM media, dir "
                        "WHERE media.dirId=dir.id AND dir.path IN (?))";
                    excBinds << toVariantList(excludedFolders);
                }
            }
            else {
                QList<int> excludedTags;
                if (!matchedTags[(*i)->_category].isEmpty()) {
                    excludedTags = matchedTags[(*i)->_category];
                } else {
                    excludedTags = tagIdsOfCategory((*i)->_category);
                }
                if (!excludedTags.isEmpty()) {
                    excludeQuery <<
                        "id IN (SELECT mediaId "
                        "FROM media_tag WHERE tagId IN (?))";
                    excBinds << toVariantList(excludedTags);
                }
            }
        }
    }

    QString select = "SELECT id FROM media";
    QStringList condList = positiveQuery + negativeQuery;

    if (typemask != DB::anyMediaType)
        condList.prepend(typeCondition("type", typemask));

    QString cond = condList.join(" AND ");

    QString query = select;
    if (!cond.isEmpty())
        query += " WHERE " + cond;

    query += " ORDER BY place";

    QList<int> positive = executeQuery(query, binds).asIntegerList();

    if (excludeQuery.isEmpty())
        return positive;

    QList<int> negative =
        executeQuery(select + " WHERE " + excludeQuery.join(" OR "),
                     excBinds).asIntegerList();

    return listSubtract(positive, negative);
}

// TODO: remove dependencies to these two
#include "DB/ImageDB.h"
#include "DB/GroupCounter.h"


QMap<QString, uint>
QueryHelper::classify(const QString& category,
                      DB::MediaType typemask,
                      QList<int>* scope) const
{
    QMap<QString, uint> result;
    DB::GroupCounter counter( category );

    const QMap<int, Set<QString> > wholeItemMap =
        mediaIdTagsMap(category, typemask);

    QMap<int,QStringList> itemMap;

    for (QMap<int, Set<QString> >::const_iterator
             i = wholeItemMap.begin(); i != wholeItemMap.end(); ++i) {
        int fileId = i.key();
        if (!scope || scope->contains(fileId))
            itemMap[fileId] = i.value().toList();
    }

    // Count images that doesn't contain an item
    if (!scope)
        result[DB::ImageDB::NONE()] = mediaItemCount(typemask) - itemMap.count();
    else
        result[DB::ImageDB::NONE()] = scope->count() - itemMap.count();

    for( QMap<int,QStringList>::Iterator mapIt = itemMap.begin(); mapIt != itemMap.end(); ++mapIt ) {
        QStringList list = mapIt.value();
        for( QStringList::Iterator listIt = list.begin(); listIt != list.end(); ++listIt ) {
            //if ( !alreadyMatched[ *listIt ] ) // We do not want to match "Jesper & Jesper"
                result[ *listIt ]++;
        }
        counter.count( list );
    }

    const QMap<QString, uint> groups = counter.result();
    for (QMap<QString, uint>::const_iterator it = groups.begin();
         it != groups.end(); ++it) {
        result[it.key()] = it.value();
    }
    return result;
}
