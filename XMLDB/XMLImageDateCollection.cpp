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

#include "XMLImageDateCollection.h"
#include "DB/ImageDB.h"
#include "DB/FileNameList.h"

void XMLDB::XMLImageDateCollection::add( const DB::ImageDate& date )
{
    m_startIndex.insertMulti(date.start(), date);
}

void XMLDB::XMLImageDateCollection::buildIndex() {
    StartIndexMap::ConstIterator startSearch = m_startIndex.constBegin();
    QDateTime biggestEnd = QDateTime( QDate( 1900, 1, 1 ) );
    for (StartIndexMap::ConstIterator it = m_startIndex.constBegin();
         it != m_startIndex.constEnd();
         ++it) {
        // We want a monotonic mapping end-date -> smallest-in-start-index.
        // Since we go through the start index sorted, lowest first, we just
        // have to keep the last pointer as long as we find smaller end-dates.
        // This should be rare as it only occurs if there are images that
        // actually represent a range not just a point in time.
        if (it.value().end() >= biggestEnd) {
            biggestEnd = it.value().end();
            startSearch = it;
        }
        m_endIndex.insert(it.value().end(), startSearch);
    }
}

/**
   Previously, counting the elements was done by going through all elements
   and count the matches for a particular range, this unfortunately had
   O(n) complexity multiplied by m ranges we would get O(mn).

   Henner Zeller rewrote it to its current state. The main idea now is to
   have all dates sorted so that it is possible to only look at the
   requested range. Since it is not points in time, we can't have just a
   simple sorted list. So we have two sorted maps, the m_startIndex and
   m_endIndex. m_startIndex is sorted by the start time of all ImageDates
   (which are in fact ranges)

   If we would just look for Images that start _after_ the query-range, we
   would miscount, because there might be Image ranges starting before the
   query time but whose end time reaches into the query range this is what
   the m_endIndex is for: it is sortd by end-date; here we look for
   everything that is >= our query start. its value() part is basically a
   pointer to the position in the m_startIndex where we actually have to
   start looking.

   The rest is simple: we determine the interesting start in
   m_startIndex using the m_endIndex and iterate through it until the
   elements in that sorted list have a start time that is larger than the
   query-end-range .. there will no more elements coming.

   The above uses the fact that a QMap::constIterator iterates the map in
   sorted order.
**/
DB::ImageCount XMLDB::XMLImageDateCollection::count( const DB::ImageDate& range )
{
    if ( m_cache.contains( range ) )
        return m_cache[range];

    int exact = 0, rangeMatch = 0;

    // We start searching in ranges that overlap our start search range, i.e.
    // where the end-date is higher than our search start.
    EndIndexMap::Iterator endSearch = m_endIndex.lowerBound(range.start());

    if (endSearch != m_endIndex.end()) {
        for ( StartIndexMap::ConstIterator it = endSearch.value();
              it != m_startIndex.constEnd() && it.key() < range.end();
              ++it) {
            DB::ImageDate::MatchType tp = it.value().isIncludedIn( range );
            switch (tp) {
            case DB::ImageDate::ExactMatch: exact++;break;
            case DB::ImageDate::RangeMatch: rangeMatch++; break;
            case DB::ImageDate::DontMatch: break;
            }
        }
    }

    DB::ImageCount res( exact, rangeMatch );
    m_cache.insert( range, res );  // TODO(hzeller) this might go now
    return res;
}

QDateTime XMLDB::XMLImageDateCollection::lowerLimit() const
{
    if (!m_startIndex.empty()) {
        // skip null dates:
        for ( StartIndexMap::ConstIterator it = m_startIndex.constBegin()
              ; it != m_startIndex.constEnd()
              ; ++it )
        {
            if (it.key().isValid())
                return it.key();
        }
    }
    return QDateTime( QDate( 1900, 1, 1 ) );
}

QDateTime XMLDB::XMLImageDateCollection::upperLimit() const
{
    if (!m_endIndex.empty()) {
        EndIndexMap::ConstIterator highest = m_endIndex.constEnd();
        --highest;
        return highest.key();
    }
    return QDateTime( QDate( 2100, 1, 1 ) );
}

XMLDB::XMLImageDateCollection::XMLImageDateCollection(const DB::FileNameList& list)
{
    Q_FOREACH(const DB::FileName& fileName, list) {
        add(fileName.info()->date());
    }
    buildIndex();
}

// vi:expandtab:tabstop=4 shiftwidth=4:
