#include "sqlcategorycollection.h"
#include "sqlcategory.h"
#include "query.h"
#include <qsqlquery.h>
CategoryPtr SQLDB::SQLCategoryCollection::categoryForName( const QString& name ) const
{
    return KSharedPtr<Category>( new SQLCategory( name ) );
}

QStringList SQLDB::SQLCategoryCollection::categoryNames() const
{
    return runAndReturnList( "SELECT distinct category FROM categorysetup" );
}

void SQLDB::SQLCategoryCollection::removeCategory( const QString& name )
{
    QSqlQuery query;
    query.prepare( "DELETE FROM categorysetup WHERE category = :category" );
    query.bindValue( QString::fromLatin1( ":catgory" ), name );
    if ( !query.exec() )
        showError( query );
}

void SQLDB::SQLCategoryCollection::rename( const QString& oldName, const QString& newName )
{
    QSqlQuery query;
    query.prepare( "UPDATE categorysetup SET category = :newName WHERE category = :oldName" );
    query.bindValue( QString::fromLatin1( ":oldName" ), oldName );
    query.bindValue( QString::fromLatin1( ":newName" ), newName );
    if ( !query.exec() )
        showError( query );
}

QValueList<CategoryPtr> SQLDB::SQLCategoryCollection::categories() const
{
    QStringList cats = categoryNames();
    QValueList<CategoryPtr> result;
    for( QStringList::ConstIterator it = cats.begin(); it != cats.end(); ++it ) {
        result.append( categoryForName( *it ) );
    }
    return result;
}

void SQLDB::SQLCategoryCollection::addCategory( const QString& category, const QString& icon, Category::ViewSize size,
                                                Category::ViewType type, bool showIt )
{
    QSqlQuery query;
    query.prepare( QString::fromLatin1( "INSERT into categorysetup set category=:category, viewtype=:viewtype,"
                                        "viewsize=:viewsize, icon=:icon, showIt=:showIt" ) );
    query.bindValue( ":category", category );
    query.bindValue( ":icon", icon );
    query.bindValue( ":viewsize", size );
    query.bindValue( ":viewtype", type );
    query.bindValue( ":showIt", showIt );
    if ( !query.exec() )
        showError( query );
}
