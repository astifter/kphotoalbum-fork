#ifndef DB_CATEGORYITEMS_H
#define DB_CATEGORYITEMS_H

#include <ksharedptr.h>

namespace DB
{
class CategoryItem :public KShared
{
public:
    CategoryItem( const QString& name ) : _name( name ) {}
    ~CategoryItem()
    {
        for( QValueList<CategoryItem*>::ConstIterator it = _subcategories.begin(); it != _subcategories.end(); ++it ) {
            delete *it;
        }
    }

    void print( int offset )
    {
        QString spaces;
        spaces.fill( ' ', offset );
        qDebug( "%s%s", spaces.latin1(), _name.latin1() );
        for( QValueList< CategoryItem* >::Iterator it = _subcategories.begin(); it != _subcategories.end(); ++it ) {
            (*it)->print( offset + 2 );
        }
    }

    QString _name;
    QValueList< CategoryItem* > _subcategories;
};

}


#endif /* DB_CATEGORYITEMS_H */

