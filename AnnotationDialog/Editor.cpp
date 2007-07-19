/* Copyright (C) 2003-2006 Jesper K. Pedersen <blackie@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "Editor.h"
#include <q3textedit.h>
//Added by qt3to4:
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <klocale.h>
#ifdef TEMPORARILY_REMOVED
#include <ksyntaxhighlighter.h>
#endif
#include <q3popupmenu.h>
#include <qregexp.h>
#include <kmenu.h>
#ifdef TEMPORARILY_REMOVED
#include <ksconfig.h>
#endif
#include <qcombobox.h>

using namespace AnnotationDialog;

Editor::Editor( QWidget* parent )
    :Q3TextEdit( parent )
{
#ifdef TEMPORARILY_REMOVED
    _config = new KSpellConfig( this );
    _config->hide();
    _highlighter = 0;
    createHighlighter();
    setTextFormat( PlainText );
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

void Editor::addSuggestion(const QString& text, const QStringList& lst, unsigned int)
{
    _replacements[text] = lst;
}

Q3PopupMenu * Editor::createPopupMenu( const QPoint & pos )
{
#ifdef TEMPORARILY_REMOVED
    Q3PopupMenu* menu = Q3TextEdit::createPopupMenu( pos );
    connect( menu, SIGNAL( activated( int ) ), this, SLOT( itemSelected( int ) ) );

    menu->insertSeparator();
    menu->setCheckable( true );

    QStringList titles, dicts;
    fetchDicts( &titles, &dicts );
    int index = 0;
    for( QStringList::Iterator it = titles.begin(); it != titles.end(); ++it, ++index ) {
        menu->insertItem( *it, 1000 + index );
        menu->setItemChecked( 1000 + index, _config->dictionary() == dicts[index] );
    }

    return menu;
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

QString Editor::wordAtPos( const QPoint& pos )
{
#ifdef TEMPORARILY_REMOVED
    int para, firstSpace, lastSpace;
    if ( !wordBoundaryAtPos( pos, &para, &firstSpace, &lastSpace ) )
        return QString::null;

    return text(para).mid( firstSpace, lastSpace - firstSpace );
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

Q3PopupMenu* Editor::replacementMenu( const QString& word  )
{
#ifdef TEMPORARILY_REMOVED
    _currentWord = word;

    KMenu* menu = new KMenu( this );
    menu->insertTitle( i18n("Replacements") );
    QStringList list = _replacements[word];

    int index = 0;
    if ( list.count() == 0 )
        menu->insertItem( i18n( "No Suggestions" ) );
    else {
        for( QStringList::Iterator it = list.begin(); it != list.end(); ++it, ++index )
            menu->insertItem( *it, index );
    }
    return menu;
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

void Editor::contentsContextMenuEvent( QContextMenuEvent *e )
{
    QPoint pos = viewportToContents(e->pos());
    QString word = wordAtPos( pos );
    if ( word.isEmpty() || !_replacements.contains( word ) ) {
        Q3TextEdit::contentsContextMenuEvent( e );
        return;
    }

    Q3PopupMenu* menu = replacementMenu( word );
    int id = menu->exec( e->globalPos() );
    if ( id == -1 || (int) _replacements[_currentWord].count() == 0 )
        return;

    QString replacement = _replacements[_currentWord][id];
    if ( replacement.isEmpty() )
        return;

    replaceWord( pos, replacement );
}

void Editor::replaceWord( const QPoint& pos, const QString& replacement )
{
    int para, firstSpace, lastSpace;
    if ( !wordBoundaryAtPos( pos, &para, &firstSpace, &lastSpace ) )
        return;

    setSelection( para, firstSpace, para, lastSpace );
    insert( replacement );
}

bool Editor::wordBoundaryAtPos( const QPoint& pos, int* para, int* start, int* end )
{
    int charPos;
    *para = 1;

    //Get the character at the position of the click
    charPos = charAt( viewportToContents(pos), para );
    QString paraText = text( *para );

    if( paraText.at(charPos).isSpace() )
        return false;

    //Get word right clicked on
    const QRegExp wordBoundary( QString::fromLatin1("[\\s\\W]") );
    *start = paraText.lastIndexOf( wordBoundary, charPos ) + 1;
    *end = paraText.find( wordBoundary, charPos );
    if( *end == -1 )
        *end = paraText.length();

    return true;
}

void Editor::itemSelected( int id )
{
#ifdef TEMPORARILY_REMOVED
    if ( id < 1000 )
        return;

    QStringList titles, dicts;
    fetchDicts( &titles, &dicts );

    QString dict = dicts[id-1000];
    _config->setDictionary( dict );
    createHighlighter();
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

void Editor::fetchDicts( QStringList* titles, QStringList* dicts )
{
#ifdef TEMPORARILY_REMOVED
    QComboBox* combo = new QComboBox( (QWidget*) 0 );
    _config->fillDicts( combo, dicts );
    for ( int i = 0; i < combo->count(); ++i ) {
        titles->append( combo->text( i ) );
    }
    delete combo;
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

void Editor::createHighlighter()
{
#ifdef TEMPORARILY_REMOVED
    delete _highlighter;
    _highlighter = new KDictSpellingHighlighter( this, true, true, red, false, Qt::black, Qt::black, Qt::black, Qt::black, _config );

    connect( _highlighter, SIGNAL(newSuggestions(const QString&, const QStringList&, unsigned int)),
             this, SLOT(addSuggestion(const QString&, const QStringList&, unsigned int)) );
#else
    kDebug() << "TEMPORARILY REMOVED: " << k_funcinfo << endl;
#endif
}

void Editor::keyPressEvent( QKeyEvent* event )
{
    if ( event->key() == Qt::Key_Tab ) {
        // disable tab key in text edit, and instead give focus on like normal.
        QWidget::keyPressEvent( event );
    }
    else
        Q3TextEdit::keyPressEvent( event );
}

#include "Editor.moc"
