#include "ImageDB.h"
#include "XMLDB/Database.h"
#include <kinputdialog.h>
#include <kpassdlg.h>
#include <klocale.h>
#include <qfileinfo.h>
#include "Browser/BrowserWidget.h"
#include "DB/CategoryCollection.h"
#include "SQLDB/Database.h"
#include <qprogressdialog.h>
#include <qapplication.h>
#include <qeventloop.h>
#include <kdebug.h>
#include <config.h>
#include "NewImageFinder.h"
#include <DB/MediaCount.h>

using namespace DB;

ImageDB* ImageDB::_instance = 0;


ImageDB* DB::ImageDB::instance()
{
    if ( _instance == 0 )
        qFatal("ImageDB::instance must not be called before ImageDB::setup");
    return _instance;
}

void ImageDB::setupXMLDB( const QString& configFile )
{
    _instance = new XMLDB::Database( configFile );
    connectSlots();
}

void ImageDB::setupSQLDB( const QString& userName, const QString& password )
{
#ifdef SQLDB_SUPPORT
    _instance = new SQLDB::Database( userName, password );
#else
    qFatal("SQLDB not compiled in");
#endif // SQLDB_SUPPORT

    connectSlots();
}

void ImageDB::connectSlots()
{
    connect( _instance->categoryCollection(), SIGNAL( itemRemoved( DB::Category*, const QString& ) ),
             _instance, SLOT( deleteItem( DB::Category*, const QString& ) ) );
    connect( _instance->categoryCollection(), SIGNAL( itemRenamed( DB::Category*, const QString&, const QString& ) ),
             _instance, SLOT( renameItem( DB::Category*, const QString&, const QString& ) ) );
    connect( Settings::SettingsData::instance(), SIGNAL( locked( bool, bool ) ), _instance, SLOT( lockDB( bool, bool ) ) );
}

QString ImageDB::NONE()
{
    return i18n("**NONE**");
}

QStringList ImageDB::currentScope( bool requireOnDisk ) const
{
    return search( Browser::BrowserWidget::instance()->currentContext(), requireOnDisk );
}

void ImageDB::setDateRange( const ImageDate& range, bool includeFuzzyCounts )
{
    _selectionRange = range;
    _includeFuzzyCounts = includeFuzzyCounts;
}

void ImageDB::clearDateRange()
{
    _selectionRange = ImageDate();
}

void ImageDB::slotRescan()
{
    bool newImages = NewImageFinder().findImages();
    if ( newImages )
        emit dirty();

    emit totalChanged( totalCount() );
}

void ImageDB::slotRecalcCheckSums( QStringList list )
{
    if ( list.isEmpty() ) {
        list = images();
        md5Map()->clear();
    }

    bool d = NewImageFinder().calculateMD5sums( list );
    if ( d )
        emit dirty();

    // To avoid deciding if the new images are shown in a given thumbnail view or in a given search
    // we rather just go to home.
    Browser::BrowserWidget::instance()->home();

    emit totalChanged( totalCount() );
}

ImageDB::ImageDB()
{
}

DB::MediaCount ImageDB::count( const ImageSearchInfo& searchInfo )
{
    QStringList list = search( searchInfo );
    int images = 0;
    int videos = 0;
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it ) {
        ImageInfoPtr inf = info( *it );
        if ( inf->mediaType() == Image )
            ++images;
        else
            ++videos;
    }
    return MediaCount( images, videos );
}

void ImageDB::convertBackend()
{
#ifdef SQLDB_SUPPORT
    QStringList allImages = images();

    QString userName = QString::null;
    QString password = QString::null;
    bool ok = false;
    userName = KInputDialog::getText( i18n("SQL database username"),
                                      i18n("Username:"), QString::null, &ok );
    if (!ok)
        return;
    if (userName.isEmpty()) {
        userName = QString::null;
    }
    else {
        QCString passwd;
        if ( KPasswordDialog::getPassword(passwd, i18n("Password for SQL database"))
             == KPasswordDialog::Accepted ) {
            password = passwd;
        }
        else
            return;
    }

    QProgressDialog dialog( 0 );
    dialog.setLabelText( i18n( "Converting Backend" ) );
    dialog.setTotalSteps( allImages.count() );
    SQLDB::Database* newBackend = new SQLDB::Database(userName, password);

    // Convert the Category info
    CategoryCollection* origCategories = categoryCollection();
    CategoryCollection* newCategories = newBackend->categoryCollection();

    QValueList<CategoryPtr> categories = origCategories->categories();
    for( QValueList<CategoryPtr>::ConstIterator it = categories.begin(); it != categories.end(); ++it ) {
        newCategories->addCategory( (*it)->text(), (*it)->iconName(), (*it)->viewSize(), (*it)->viewType(), (*it)->doShow() );
        newCategories->categoryForName( (*it)->text() )->setItems( (*it)->items() );
    }

    kdDebug() << "Also save membermaps" << endl;

    // Convert all images to the new back end
    int count = 0;
    ImageInfoList list;
    for( QStringList::ConstIterator it = allImages.begin(); it != allImages.end(); ++it ) {
        dialog.setProgress( count++ );
        qApp->processEvents();
        list.append( info(*it) );
        if ( count % 1000 == 0 ) {
            newBackend->addImages( list );
            list.clear();
        }
    }
    if ( list.count() != 0 )
        newBackend->addImages( list );
#endif // SQLDB_SUPPORT
}

void ImageDB::slotReread( const QStringList& list, int mode)
{
    // Do here a reread of the exif info and change the info correctly in the database without loss of previous added data
    QProgressDialog  dialog( i18n("Loading information from images"),
                             i18n("Cancel"), list.count(), 0, "progress dialog", true );

    int count=0;
    for( QStringList::ConstIterator it = list.begin(); it != list.end(); ++it, ++count  ) {
        if ( count % 10 == 0 ) {
            dialog.setProgress( count ); // ensure to call setProgress(0)
            qApp->eventLoop()->processEvents( QEventLoop::AllEvents );

            if ( dialog.wasCanceled() )
                return;
        }

        QFileInfo fi( *it );

        if (fi.exists())
            info(*it)->readExif(*it, mode);
        emit dirty();
    }

}

#include "ImageDB.moc"
