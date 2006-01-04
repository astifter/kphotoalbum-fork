#include "ThumbnailView.h"
#include <qpixmapcache.h>
#include <qpainter.h>
#include "math.h"
#include "options.h"
#include "ThumbnailRequest.h"
#include "imagemanager.h"
#include "imageinfoptr.h"
#include "imagedb.h"
#include "util.h"
#include <qcursor.h>
#include <qapplication.h>
#include "ThumbnailToolTip.h"

// PENDING(blackie) TODO:
// - show file names

/**
 * \namespace ThumbnailView
 * The thumbnail view.
 */

/**
 * \class ThumbnailView
 * This is the widget which shows the thumbnails.
 *
 * In previous versions this was implemented using a QIconView, but there
 * simply was too many problems, so after years of tears and pains I
 * rewrote it.
 */

ThumbnailView::ThumbnailView* ThumbnailView::ThumbnailView::_instance = 0;
ThumbnailView::ThumbnailView::ThumbnailView( QWidget* parent, const char* name )
    :QGridView( parent, name ),
     _gridResizeInteraction( this ),
    _dragInteraction( this ),
     _mouseTrackingHandler( this ),
     _mouseHandler( &_mouseTrackingHandler )
{
    _instance = this;
    int size = Options::instance()->thumbSize() + SPACE;
    setCellWidth( size );
    setCellHeight( size +2 );
    setFocusPolicy( WheelFocus );

    // It beats me why I need to set mouse tracking on both, but without it doesn't work.
    viewport()->setMouseTracking( true );
    setMouseTracking( true );

    connect( this, SIGNAL( contentsMoving( int, int ) ), this, SLOT( emitDateChange( int, int ) ) );

    _toolTip = new ThumbnailToolTip( this );
}


void ThumbnailView::ThumbnailView::paintCell( QPainter * p, int row, int col )
{
    QPixmap doubleBuffer( cellRect().size() );
    QPainter painter( &doubleBuffer );
    paintCellBackground( &painter, row, col );

    QString fileName = fileNameInCell( row, col );
    if ( !fileName.isNull() && !_mouseHandler->isResizingGrid() ) {
        QPixmap* pix = pixmapCache().find( fileNameInCell( row, col ) );
        if ( pix ) {
            QRect rect = iconGeometry( row, col );
            Q_ASSERT( !rect.isNull() );
            painter.drawPixmap( rect, *pix );
            if ( _selectedFiles.contains( fileName ) )
                painter.fillRect( rect, QBrush( palette().active().highlight(), Dense4Pattern ) );
        }
        else {
            int size = Options::instance()->thumbSize();
            int angle = ImageDB::instance()->info( fileName )->angle();
            ThumbnailRequest* request = new ThumbnailRequest( fileName, QSize( size, size ), angle, this );
            request->setCache();
            ImageManager::instance()->load( request );
        }
    }

    painter.end();
    p->drawPixmap( cellRect(), doubleBuffer );
}


void ThumbnailView::ThumbnailView::setImageList( const QStringList& list )
{
    _imageList = list;
    if ( isVisible() ) {
        updateGridSize();
        update();
    }
}


/**
 * Calculate the about of thumbnails per row.
 */
int ThumbnailView::ThumbnailView::thumbnailsPerRow() const
{
    return width() / cellWidth();
}


/**
 * Return the file name shown in cell (row,col) if a thumbnail is shown in this cell or null otherwise.
 */
QString ThumbnailView::ThumbnailView::fileNameInCell( int row, int col ) const
{
    uint index = row * thumbnailsPerRow() + col;
    if ( index >= _imageList.count() )
        return QString::null;
    else
        return _imageList[index];
}

/**
 * Returns the file name shown at viewport position (x,y) if a thumbnail is shown at this position or QString::null otherwise.
 */
QString ThumbnailView::ThumbnailView::fileNameAtViewportPos( const QPoint& viewpointPos ) const
{
    QPoint contentsPos = viewportToContents( viewpointPos );
    int col = columnAt( contentsPos.x() );
    int row = rowAt( contentsPos.y() );

    QRect cellRect = const_cast<ThumbnailView*>(this)->cellGeometry( row, col );
    QRect iconRect = iconGeometry( row, col );

    // map iconRect from local coordinates within the cell to contents coordinates
    iconRect.moveBy( cellRect.x(), cellRect.y() );

    if ( iconRect.contains( contentsPos ) )
        return fileNameInCell( row, col );
    else
        return QString::null;
}

/**
 * Return the geometry for the icon in the cell (row,col). The coordinates are local to the cell.
 */
QRect ThumbnailView::ThumbnailView::iconGeometry( int row, int col ) const
{
    QString fileName = fileNameInCell( row, col );
    if ( fileName.isNull() ) // empty cell
        return QRect();

    QPixmap* pix = const_cast<ThumbnailView*>(this)->pixmapCache().find( fileName );
    if ( !pix )
        return QRect();

    int size = Options::instance()->thumbSize() + SPACE;
    int xoff = 1 + (size - pix->width())/2; // 1 is for the border at the left
    int yoff = 1 + (size - pix->height() )/2;
    return QRect( xoff, yoff, pix->width(), pix->height() );
}


/**
 * Return a pixmap cache to use for caching the thumbnails.
 */
QPixmapCache& ThumbnailView::ThumbnailView::pixmapCache()
{
    static QPixmapCache cache;
    static int lastSize = -1;
    cache.setCacheLimit( 4* 1024 ); // PENDING(blackie) make this size customizable
    int currentThumbSize = Options::instance()->thumbSize();
    if (lastSize != currentThumbSize) {
        cache.clear();
        lastSize = currentThumbSize;
    }
    return cache;
}

void ThumbnailView::ThumbnailView::pixmapLoaded( const QString& fileName, const QSize& size, const QSize& fullSize, int, const QImage& image,
                                  bool loadedOK )
{
    QPixmap* pixmap = new QPixmap( size );
    if ( loadedOK && !image.isNull() )
        pixmap->convertFromImage( image );

    else if ( !loadedOK)
        pixmap->fill(Qt::gray);

    ImageInfoPtr imageInfo = ImageDB::instance()->info( fileName );

    if ( !loadedOK || !imageInfo->imageOnDisk() ) {
        QPainter p( pixmap );
        p.setBrush( white );
        p.setWindow( 0, 0, 100, 100 );
        QPointArray pts;
        pts.setPoints( 3, 70,-1,  100,-1,  100,30 );
        p.drawConvexPolygon( pts );
    }

    if ( fullSize.isValid() )
        imageInfo->setSize( fullSize );

    pixmapCache().insert( fileName, pixmap );
    repaintCell( fileName );
}

/**
 * Request a repaint of the cell showing filename
 */
void ThumbnailView::ThumbnailView::repaintCell( const QString& fileName )
{
    // This ought really be updateCell, but I get errors where cells are
    // not repainted though requested, during scrolling.
    QPoint pos = positionForFileName( fileName );
    repaintCell( pos.y(), pos.x() );
}

/**
 * Update the grid size depending on the size of the widget
 */
void ThumbnailView::ThumbnailView::updateGridSize()
{
    setNumCols( thumbnailsPerRow() );
    setNumRows( QMAX( numRowsPerPage(), (int) ceil( 1.0 * _imageList.size() / thumbnailsPerRow() ) ) );
}

void ThumbnailView::ThumbnailView::showEvent( QShowEvent* )
{
    updateGridSize();
}

/**
 * Paint the cell back ground, and the outline
 */
void ThumbnailView::ThumbnailView::paintCellBackground( QPainter* p, int row, int col )
{
    QRect rect = cellRect();
    p->fillRect( rect, black );
    p->fillRect( QRect( QPoint( rect.left(), rect.top()+1), QPoint( rect.right(), rect.bottom()-1 ) ), white );
    p->setPen( gray );

    // left of frame
    if ( col != 0 )
        p->drawLine( rect.left(), rect.top()+1, rect.left(), rect.bottom()-1 );

    // bottom line
    if ( row != numRows() -1 ) {
        // Draw a grey line to give an illusion of a raised box.
        p->drawLine( rect.left(), rect.bottom() -1, rect.right(), rect.bottom()-1 );
    }
    else {
        // We don't want a seperating line at the bottom, but we've already drawn the black one
        // during the fillRect above, so lets undraw it now.
        p->setPen( white );
        p->drawLine( rect.left(), rect.bottom(), rect.right(), rect.bottom() );
    }

    if ( row == 0 ) {
        // undraw the topmost line of row zero
        p->setPen( white );
        p->drawLine( rect.left(), rect.top(), rect.right(), rect.top() );
    }
}

void ThumbnailView::ThumbnailView::keyPressEvent( QKeyEvent* event )
{
    if ( isMovementKey( event->key() ) )
        keyboardMoveEvent( event );

    if ( event->key() == Key_Return )
        emit showImage( _currentItem );

    if ( event->key() == Key_Space )
        toggleSelection( _currentItem );

    possibleEmitSelectionChanged();
}

void ThumbnailView::ThumbnailView::keyboardMoveEvent( QKeyEvent* event )
{
    static QString startPossition;

    if ( !( event->state()& ShiftButton ) && !( event->state() &  ControlButton ) ) {
        clearSelection();
    }

    // Decide the next keyboard focus cell
    QPoint currentPos(0,0);
    if ( !_currentItem.isNull() )
        currentPos = positionForFileName( _currentItem );

    QPoint newPos;
    switch (event->key() ) {
    case Key_Left:
        newPos = currentPos;
        newPos.rx()--;

        if ( newPos.x() < 0 )
            newPos = QPoint( thumbnailsPerRow()-1, newPos.y()-1 );
        break;

    case Key_Right:
        newPos = currentPos;
        newPos.rx()++;
        if ( newPos.x() == thumbnailsPerRow() )
            newPos = QPoint( 0, newPos.y()+1 );
        break;

    case Key_Down:
        newPos = QPoint( currentPos.x(), currentPos.y() + 1 );
        break;

    case Key_Up:
        newPos = QPoint( currentPos.x(), currentPos.y() - 1 );
        break;

    case Key_PageDown:
    case Key_PageUp:
    {
        int rows = (event->key() == Key_PageDown) ? 1 : -1;
        if ( event->state() & AltButton )
            rows *= numRows() / 20;
        else
            rows *= numRowsPerPage();

        newPos = QPoint( currentPos.x(), currentPos.y() + rows );
        break;
    }
    case Key_Home:
        newPos = QPoint( 0, 0 );
        break;

    case Key_End:
        newPos = lastCell();
        break;
    }

    // Check for overruns
    if ( newPos > lastCell() )
        newPos = lastCell();
    if ( newPos < QPoint(0,0) )
        newPos = QPoint(0,0);

    // Update focus cell, and set selection
    if ( event->state() & ShiftButton )
        selectItems( positionForFileName( startPossition ), newPos );

    else if ( ! (event->state() & ControlButton ) ) {
        selectCell( newPos );
        repaintCell( currentPos.y(), currentPos.x() );
    }
    _currentItem = fileNameInCell( newPos );
    if ( !( event->state() & ShiftButton ) )
        startPossition = _currentItem;

    // Scroll if necesary
    if ( newPos.y() > lastVisibleRow( ThumbnailView::FullyVisible ) )
        setContentsPos( contentsX(), cellGeometry( newPos.y(), newPos.x() ).top() -
                               (numRowsPerPage()-1)*cellHeight()  );

    if  ( newPos.y() < firstVisibleRow( ThumbnailView::FullyVisible ) )
        setContentsPos( contentsX(), cellGeometry( newPos.y(), newPos.x() ).top() );
}

/**
 * Update selection to include files from start to end
 */
void ThumbnailView::ThumbnailView::selectItems( const QPoint& start, const QPoint& end )
{
    Set<QString> oldSelection = _selectedFiles;
    _selectedFiles.clear();

    selectAllCellsBetween( start, end, false );

    for( Set<QString>::Iterator it = oldSelection.begin(); it != oldSelection.end(); ++it ) {
        if ( !_selectedFiles.contains( *it ) )
            repaintCell( *it );
    }

    for( Set<QString>::Iterator it = _selectedFiles.begin(); it != _selectedFiles.end(); ++it ) {
        if ( !oldSelection.contains( *it ) )
            repaintCell( *it );
    }

}
/**
 * Return the number of complete rows per page
 */
int ThumbnailView::ThumbnailView::numRowsPerPage() const
{
    return height() / cellHeight();
}

void ThumbnailView::ThumbnailView::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == RightButton )
        return;
    else if (event->button() & MidButton )
        _mouseHandler = &_gridResizeInteraction;
    else
        _mouseHandler = &_dragInteraction;

    _mouseHandler->mousePressEvent( event );
}

void ThumbnailView::ThumbnailView::mouseMoveEvent( QMouseEvent* event )
{
    _mouseHandler->mouseMoveEvent( event );
}

void ThumbnailView::ThumbnailView::mouseReleaseEvent( QMouseEvent* event )
{
    _mouseHandler->mouseReleaseEvent( event );
    _mouseHandler = &_mouseTrackingHandler;
}

void ThumbnailView::ThumbnailView::mouseDoubleClickEvent( QMouseEvent * event )
{
    QString fileName = fileNameAtViewportPos( event->pos() );
    if ( !fileName.isNull() )
        emit showImage( fileName );
}


void ThumbnailView::ThumbnailView::emitDateChange( int x, int y )
{
    if ( _isSettingDate )
        return;

    // Unfortunately the contentsMoving signal is emitted *before* the move, so we need to find out what is on the new position ourself.
    QString fileName = fileNameInCell( rowAt(y), columnAt(x) );
    if ( fileName.isNull() )
        return;

    static QDateTime lastDate;
    QDateTime date = ImageDB::instance()->info( fileName )->date().start();
    if ( date != lastDate ) {
        lastDate = date;
        if ( date.date().year() != 1900 )
            emit currentDateChanged( date );
    }
}

/**
 * scroll to the date specified with the parameter date.
 * The boolean includeRanges tells whether we accept range matches or not.
 */
void ThumbnailView::ThumbnailView::gotoDate( const ImageDateRange& date, bool includeRanges )
{
    _isSettingDate = true;
    QString candidate;
    for( QStringList::Iterator imageIt = _imageList.begin(); imageIt != _imageList.end(); ++imageIt ) {
        ImageInfoPtr info = ImageDB::instance()->info( *imageIt );

        ImageDateRange::MatchType match = info->dateRange().isIncludedIn( date );
        if ( match == ImageDateRange::ExactMatch || ( match == ImageDateRange::RangeMatch && includeRanges ) ) {
            if ( !candidate.isNull() ) {
                if ( info->date().start() < ImageDB::instance()->info(candidate)->date().start() )
                    candidate = *imageIt;
            }
            else
                candidate = *imageIt;
        }
    }
    if ( !candidate.isNull() ) {
        QPoint pos = positionForFileName( candidate );
        QRect contentsRect = cellGeometry( pos.y(), pos.x() );
        setContentsPos( contentsRect.x(), contentsRect.y() );
        _currentItem = candidate;
    }
    _isSettingDate = false;
}

/**
 * return the position (row,col) for the given file name
 */
QPoint ThumbnailView::ThumbnailView::positionForFileName( const QString& fileName ) const
{
    Q_ASSERT( !fileName.isNull() );
    int index = _imageList.findIndex( fileName );
    Q_ASSERT ( index != -1 );
#ifdef TEMPORARILY_REMOVED // I'll bet you I'll get in trouble here again
    if ( index == -1 )
        qDebug("Ups");
#endif

    int row = index / thumbnailsPerRow();
    int col = index % thumbnailsPerRow();
    return QPoint( col, row );
}

/**
 * \brief Returns whether the thumbnail for fileName is still needed.
 *
 * If the user scrolls down through the view, a back log of thumbnail
 * request may build up, which will slow down scrolling a lot. Therefore
 * the \ref ImageManger has the capability to check whether a thumbnail
 * request is really needed, when it gets to load the given thumbnail.
 */
bool ThumbnailView::ThumbnailView::thumbnailStillNeeded( const QString& fileName ) const
{
    QPoint pos = positionForFileName( fileName );
    return pos.y() >= firstVisibleRow( PartlyVisible ) && pos.y() <= lastVisibleRow( PartlyVisible );
}

/*
 * Returns the first row that is at least partly visible.
 */
int ThumbnailView::ThumbnailView::firstVisibleRow( VisibleState state ) const
{
    int firstRow = rowAt( contentsY() );
    if ( state == FullyVisible && rowAt( contentsY() + cellHeight()-1 ) != firstRow )
        firstRow += 1;

    return firstRow;
}

int ThumbnailView::ThumbnailView::lastVisibleRow( VisibleState state ) const
{
    int lastRow = rowAt( contentsY() + visibleHeight() );
    if ( state == FullyVisible && rowAt( contentsY() + visibleHeight() - cellHeight() -1 ) != lastRow )
        lastRow -= 1;
    return lastRow;
}

void ThumbnailView::ThumbnailView::selectCell( const QPoint& pos )
{
    selectCell( pos.y(), pos.x() );
}

void ThumbnailView::ThumbnailView::selectCell( int row, int col, bool repaint )
{
    QString file = fileNameInCell( row, col );
    if ( !file.isNull() ) {
        _selectedFiles.insert( file );
        if ( repaint )
            repaintCell( row, col );
    }
}

QPoint ThumbnailView::ThumbnailView::cellAtViewportPos( const QPoint& pos ) const
{
    QPoint contentsPos = viewportToContents( pos );
    int col = columnAt( contentsPos.x() );
    int row = rowAt( contentsPos.y() );
    return QPoint( col, row );
}

void ThumbnailView::ThumbnailView::selectAllCellsBetween( QPoint pos1, QPoint pos2, bool repaint )
{
    Util::ensurePosSorted( pos1, pos2 );

    if ( pos1.y() == pos2.y() ) {
        // This is the case where images from only one row is selected.
        for ( int col = pos1.x(); col <= pos2.x(); ++ col )
            selectCell( pos1.y(), col, repaint );
    }
    else {
        // We know we have at least two rows.

        // first row
        for ( int col = pos1.x(); col < thumbnailsPerRow(); ++ col )
            selectCell( pos1.y(), col, repaint );

        // rows in between
        for ( int row = pos1.y()+1; row < pos2.y(); ++row )
            for ( int col = 0; col < thumbnailsPerRow(); ++ col )
                selectCell( row, col, repaint );

        // last row
        for ( int col = 0; col <= pos2.x(); ++ col )
            selectCell( pos2.y(), col, repaint );
    }
}

void ThumbnailView::ThumbnailView::resizeEvent( QResizeEvent* e )
{
    QGridView::resizeEvent( e );
    updateGridSize();
}

void ThumbnailView::ThumbnailView::clearSelection()
{
    Set<QString> oldSelection = _selectedFiles;
    _selectedFiles.clear();
    for( Set<QString>::Iterator fileIt = oldSelection.begin(); fileIt != oldSelection.end(); ++fileIt ) {
        repaintCell( *fileIt );
    }
}

QString ThumbnailView::ThumbnailView::fileNameInCell( const QPoint& cell ) const
{
    return fileNameInCell( cell.y(), cell.x() );
}

bool ThumbnailView::ThumbnailView::isFocusAtLastCell() const
{
    return positionForFileName(_currentItem) == lastCell();
}

bool ThumbnailView::ThumbnailView::isFocusAtFirstCell() const
{
    return positionForFileName(_currentItem).x() == 0 && positionForFileName(_currentItem).y() == 0;
}

/**
 * Return the coordinates of the last cell with a thumbnail in
 */
QPoint ThumbnailView::ThumbnailView::lastCell() const
{
    return QPoint( _imageList.count() % thumbnailsPerRow() -1,
                   _imageList.count() / thumbnailsPerRow() );
}

bool ThumbnailView::ThumbnailView::isMovementKey( int key )
{
    return ( key == Key_Up || key == Key_Down || key == Key_Left || key == Key_Right ||
             key == Key_Home || key == Key_End || key == Key_PageUp || key == Key_PageDown );
}

void ThumbnailView::ThumbnailView::toggleSelection( const QString& fileName )
{
    if ( _selectedFiles.contains( fileName ) )
        _selectedFiles.remove( fileName );
    else
        _selectedFiles.insert( fileName );

    repaintCell( fileName );
}

QStringList ThumbnailView::ThumbnailView::selection() const
{
    QStringList res;
    for( QStringList::ConstIterator it = _imageList.begin(); it != _imageList.end(); ++it ) {
        if ( _selectedFiles.contains( *it ) )
            res.append( *it );
    }
    return res;
}

void ThumbnailView::ThumbnailView::possibleEmitSelectionChanged()
{
    static Set<QString> oldSelection;
    if ( oldSelection != _selectedFiles ) {
        oldSelection = _selectedFiles;
        emit selectionChanged();
    }
}

QStringList ThumbnailView::ThumbnailView::imageList() const
{
    return _imageList;
}

void ThumbnailView::ThumbnailView::selectAll()
{
    _selectedFiles.clear();
    for( QStringList::ConstIterator it = _imageList.begin(); it != _imageList.end(); ++it ) {
        _selectedFiles.insert(*it);
    }
    repaintScreen();
}

void ThumbnailView::ThumbnailView::repaintCell( int row, int col )
{
    QGridView::repaintCell( row, col );
}

void ThumbnailView::ThumbnailView::reload()
{
    pixmapCache().clear();
    _selectedFiles.clear();
    repaintScreen();
}

void ThumbnailView::ThumbnailView::repaintScreen()
{
    for ( int row = firstVisibleRow( PartlyVisible ); row <= lastVisibleRow( PartlyVisible ); ++row )
        for ( int col = 0; col < thumbnailsPerRow(); ++col )
            repaintCell( row, col );
}

QString ThumbnailView::ThumbnailView::fileNameUnderCursor() const
{
    return fileNameAtViewportPos( mapFromGlobal( QCursor::pos() ) );
}

QString ThumbnailView::ThumbnailView::currentItem() const
{
    return _currentItem;
}

ThumbnailView::ThumbnailView* ThumbnailView::ThumbnailView::theThumbnailView()
{
    return _instance;
}

void ThumbnailView::ThumbnailView::setCurrentItem( const QString& fileName )
{
    QPoint pos = positionForFileName( fileName );
    _currentItem = fileName;
    _selectedFiles.clear();
    _selectedFiles.insert( fileName );
    repaintCell( fileName );
    ensureCellVisible( pos.y(), pos.x() );
}

void ThumbnailView::ThumbnailView::showToolTipsOnImages( bool on )
{
    _toolTip->setActive( on );
}

