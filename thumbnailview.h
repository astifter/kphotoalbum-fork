#ifndef THUMBNAILVIEW_H
#define THUMBNAILVIEW_H
#include <qiconview.h>
#include "imageinfo.h"
class ImageManager;

class ThumbNailView :public QIconView {
    Q_OBJECT
    friend class ThumbNail;

public:
    ThumbNailView( QWidget* parent,  const char* name = 0 );
    void load( ImageInfoList* list );

public slots:
    void reload();
    void slotSelectAll();
    void slotCut();
    void slotPaste();

signals:
    void changed();

protected slots:
    void showImage( QIconViewItem* );
    virtual void startDrag();

protected:
    virtual void contentsDragMoveEvent( QDragMoveEvent *e );
    virtual void contentsDropEvent( QDropEvent* e );
    void setHighlighted( ThumbNail* item );
    QPtrList<ThumbNail> selected() const;

//    void contentsDragEnterEvent( QDragEnterEvent *e );


    void reorder( ImageInfo* item, const ImageInfoList& list, bool after );

private:
    ImageInfoList* _imageList;
    ThumbNail* _currentHighlighted;
    ImageInfoList _cutList;
};

#endif /* THUMBNAILVIEW_H */

