#ifndef DISPLAYAREAHANDLER_H
#define DISPLAYAREAHANDLER_H
class DisplayArea;
class QMouseEvent;
#include <qnamespace.h>

class DisplayAreaHandler :public Qt
{
public:
    DisplayAreaHandler( DisplayArea* display ) : _display( display ) {}
    virtual bool mousePressEvent ( QMouseEvent* e ) = 0;
    virtual bool mouseReleaseEvent ( QMouseEvent* e ) = 0;
    virtual bool mouseMoveEvent ( QMouseEvent* e ) = 0;

protected:
    DisplayArea* _display;
};

#endif /* DISPLAYAREAHANDLER_H */

