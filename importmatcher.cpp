#include "importmatcher.h"
#include <qcheckbox.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qcombobox.h>
#include "options.h"
#include <qgrid.h>
#include <qlabel.h>
#include <klocale.h>

ImportMatcher::ImportMatcher( const QString& otherOptionGroup, const QString& myOptionGroup,
                              const QStringList& otherOptionList, const QStringList& myOptionList,
                              QWidget* parent, const char* name )
    : QScrollView( parent, name ), _otherOptionGroup( otherOptionGroup ), _myOptionGroup( myOptionGroup )
{
    setResizePolicy( AutoOneFit );

    QWidget* top = new QWidget( viewport() );
    QVBoxLayout* layout = new QVBoxLayout( top, 6 );
    QWidget* grid = new QWidget( top, "grid" );
    layout->addWidget( grid );
    layout->addStretch(1);

    QGridLayout* gridLay = new QGridLayout( grid, 2, 1, 0, 6 );
    gridLay->setColStretch( 1, 1 );
    addChild( top );

    QLabel* label = new QLabel( i18n("Key in file"), grid );
    gridLay->addWidget( label, 0,0 );
    QColor col = label->paletteBackgroundColor();
    label->setPaletteBackgroundColor( label->paletteForegroundColor() );
    label->setPaletteForegroundColor( col );
    label->setAlignment( AlignCenter );

    label = new QLabel( i18n("Key in your data base"), grid );
    gridLay->addWidget( label, 0, 1 );
    label->setPaletteBackgroundColor( label->paletteForegroundColor() );
    label->setPaletteForegroundColor( col );
    label->setAlignment( AlignCenter );

    int row = 1;
    for( QStringList::ConstIterator it = otherOptionList.begin(); it != otherOptionList.end(); ++it ) {
        OptionMatch* match = new OptionMatch( *it, myOptionList, grid, gridLay, row++ );
        _matchers.append( match );
    }
}

OptionMatch::OptionMatch( const QString& option, const QStringList& options, QWidget* parent, QGridLayout* grid, int row )
{
    _checkbox = new QCheckBox( option, parent );
    _checkbox->setChecked( true );
    grid->addWidget( _checkbox, row, 0 );

    _combobox = new QComboBox( true, parent, "combo box" );
    _combobox->insertStringList( options );
    grid->addWidget( _combobox, row, 1 );

    if ( options.contains( option ) ) {
        _combobox->setCurrentText( option );
    }
    else {
        QString match = QString::null;
        for( QStringList::ConstIterator it = options.begin(); it != options.end(); ++it ) {
            if ( (*it).contains( option ) || option.contains( *it ) ) {
                if ( match == QString::null )
                    match = *it;
                else {
                    match = QString::null;
                    break;
                }
            }
        }
        if ( match != QString::null ) {
            _combobox->setCurrentText( match );
        }
        else {
            _combobox->setCurrentText( option );
        }
        _checkbox->setPaletteForegroundColor( Qt::red );
    }
    QObject::connect( _checkbox, SIGNAL( toggled( bool ) ), _combobox, SLOT( setEnabled( bool ) ) );
}
