/* Copyright (C) 2003-2004 Jesper K. Pedersen <blackie@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "optionsdialog.h"
#include <kfiledialog.h>
#include <klocale.h>
#include <qlayout.h>
#include <qlabel.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <qspinbox.h>
#include "options.h"
#include <kicondialog.h>
#include <qlistbox.h>
#include <kmessagebox.h>
#include "imagedb.h"
#include <qcheckbox.h>
#include <kinputdialog.h>
#include <qwhatsthis.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include "viewersizeconfig.h"
#include <limits.h>
#ifdef HASKIPI
#  include <libkipi/pluginloader.h>
#endif
#include <kdebug.h>
#include <kcolorbutton.h>

OptionsDialog::OptionsDialog( QWidget* parent, const char* name )
    :KDialogBase( IconList, i18n( "Options" ), Ok | Cancel, Ok, parent, name ), _currentCategory( QString::null ), _currentGroup( QString::null )
{
    createGeneralPage();
    createThumbNailPage();
    createOptionGroupsPage();
    createGroupConfig();
    createViewerPage();
#ifdef HASKIPI
    createPluginPage();
#endif
    connect( this, SIGNAL( aboutToShowPage( QWidget* ) ), this, SLOT( slotPageChange() ) );
    connect( this, SIGNAL( okClicked() ), this, SLOT( slotMyOK() ) );
}

void OptionsDialog::createGeneralPage()
{
    QWidget* top = addPage( i18n("General" ), i18n("General" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "kimdaba" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    // Thrust time stamps
    QLabel* timeStampLabel = new QLabel( i18n("Trust image dates:"), top );
    _trustTimeStamps = new KComboBox( top );
    _trustTimeStamps->insertStringList( QStringList() << i18n("Always") << i18n("Ask") << i18n("Never") );
    QHBoxLayout* lay3 = new QHBoxLayout( lay1, 6 );
    lay3->addWidget( timeStampLabel );
    lay3->addWidget( _trustTimeStamps );
    lay3->addStretch( 1 );

    // Do EXIF rotate
    _useEXIFRotate = new QCheckBox( i18n( "Use EXIF orientation information" ), top );
    lay1->addWidget( _useEXIFRotate );

    _useEXIFComments = new QCheckBox( i18n( "Use EXIF description" ), top );
    lay1->addWidget( _useEXIFComments );

    // Search for images on startup
    _searchForImagesOnStartup = new QCheckBox( i18n("Search for new images on startup"), top );
    lay1->addWidget( _searchForImagesOnStartup );

    // Auto save
    QLabel* label = new QLabel( i18n("Auto save every:"), top );
    _autosave = new QSpinBox( 1, 120, 1, top );
    _autosave->setSuffix( i18n( "min." ) );
    QHBoxLayout* lay6 = new QHBoxLayout( lay1, 6 );
    lay6->addWidget( label );
    lay6->addWidget( _autosave );
    lay6->addStretch( 1 );

    // Album Category
    QLabel* albumCategoryLabel = new QLabel( i18n("Category for virtual albums:" ), top, "albumCategoryLabel" );
    _albumCategory = new QComboBox( top, "_albumCategory" );
    QHBoxLayout* lay7 = new QHBoxLayout( lay1, 6 );
    lay7->addWidget( albumCategoryLabel );
    lay7->addWidget( _albumCategory );
    _albumCategory->insertStringList( Options::instance()->optionGroups() );

    lay1->addStretch( 1 );


    // Whats This
    QString txt;

    txt = i18n( "<qt><p>KimDaBa will try to read the image date from EXIF information in the image. "
                "If that fails it will try to get the date from the file's time stamp.</p>"
                "<p>However, this information will be wrong if the image was scanned in (you want the date the image "
                "was taken, not the date of the scan).</p>"
                "<p>If you only scan images, in contrast to sometimes using "
                "a digital camera, you should reply <b>no</b>. If you never scan images, you should reply <b>yes</b>, "
                "otherwise reply <b>ask</b>. This will allow you to decide whether the images are from "
                "the scanner or the camera, from session to session.</p></qt>" );
    QWhatsThis::add( timeStampLabel, txt );
    QWhatsThis::add( _trustTimeStamps, txt );

    txt = i18n( "<qt><p>JPEG images may contain information about rotation. "
                "If you have a reason for not using this information to get a default rotation of "
                "your images, uncheck this check box.</p>"
                "<p>Note: Your digital camera may not write this information into the images at all.</p></qt>" );
    QWhatsThis::add( _useEXIFRotate, txt );

    txt = i18n( "<qt><p>JPEG images may contain a description."
               "Using this checkbox you specify if you want to use this as a "
               "default description for your images.</p></qt>" );
    QWhatsThis::add( _useEXIFComments, txt );

    txt = i18n( "<qt><p>KimDaBa is capable of searching for new images itself when started, this does, "
                "however, take some time, so instead you may wish to manually tell KimDaBa to search for new images "
                "using <tt>Maintenance->Rescan for new images</tt></qt>");
    QWhatsThis::add( _searchForImagesOnStartup, txt );

    txt = i18n("<qt><p>KimDaBa shares plugins with other imaging applications, some of which have the concept of albums. "
               "KimDaBa do not have this concept; nevertheless, for certain plugins to function, KimDaBa behaves "
               "to the plugin system as if it did.</p>"
               "<p>KimDaBa does this by defining the current album to be the current view - that is, all the images the "
               "browser offers to display.</p>"
               "<p>In addition to the current album, KimDaBa must also be able to give a list of all albums; "
               "the list of all albums is defined in the following way:"
               "<ul><li>When KimDaBa's browser displays the content of a category, say all Persons, then each item in this category "
               "will look like an album to the plugin."
               "<li>Otherwise, the category you specify using this option will be used; e.g. if you specify Persons "
               "with this option, then KimDaBa will act as if you had just chosen to display persons and then invoke "
               "the plugin which needs to know about all albums.</p>"
               "<p>Most users would probably want to specify Keywords here.</p></qt>");
    QWhatsThis::add( albumCategoryLabel, txt );
    QWhatsThis::add( _albumCategory, txt );
}

void OptionsDialog::createThumbNailPage()
{
    QWidget* top = addPage( i18n("Thumbnail View" ), i18n("Thumbnail View" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "view_icon" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    // Thumbnail size
    QLabel* label = new QLabel( i18n("Thumbnail size:"), top );
    _thumbnailSize = new QSpinBox( 16, 512, 8, top );
    QHBoxLayout* lay2 = new QHBoxLayout( lay1, 6 );
    lay2->addWidget( label );
    lay2->addWidget( _thumbnailSize );
    lay2->addStretch(1);

    // Preview size
    QLabel* previewSizeLabel = new QLabel( i18n("Preview image size:" ), top, "previewSizeLabel" );
    _previewSize = new QSpinBox( 0, 2000, 10, top, "_previewSize" );
    _previewSize->setSpecialValueText( i18n("No Image Preview") );
    lay2 = new QHBoxLayout( lay1, 6 );
    lay2->addWidget( previewSizeLabel );
    lay2->addWidget( _previewSize );
    lay2->addStretch( 1 );

    // Max images to show per page
    QLabel* maxImagesLabel = new QLabel( i18n("Maximum images to show per page:"), top );
    _maxImages = new QSpinBox( 10, 10000, 1, top ) ;
    QHBoxLayout* lay4 = new QHBoxLayout( lay1, 6 );
    lay4->addWidget( maxImagesLabel );
    lay4->addWidget( _maxImages );
    lay4->addStretch(1);

    // Display Labels
    _displayLabels = new QCheckBox( i18n("Display labels in thumbnail view" ), top, "displayLabels" );
    lay1->addWidget( _displayLabels );

    // Background Color
    QLabel* backgroundColorLabel = new QLabel( i18n( "Background color:" ), top, "backgroundColorLabel" );
    _backgroundColor = new KColorButton( black, top, "_backgroundColor" );
    QHBoxLayout* lay5 = new QHBoxLayout( lay1, 6 );
    lay5->addWidget( backgroundColorLabel );
    lay5->addWidget( _backgroundColor );
    lay5->addStretch( 1 );

    // Auto Show Thumbnail view
    _autoShowThumbnailView = new QCheckBox( i18n("Show thumbnail view when images matches gets below a single page"), top );
    lay1->addWidget( _autoShowThumbnailView );

    lay1->addStretch(1);

    // Whats This
    QString txt;

    txt = i18n( "<qt><p>If you select <tt>Help|Show Tooltips</tt> in the thumbnail view, then you will see a small tool tip window "
                "displaying information about the thumbnails. This window includes a small preview image. "
                "This option configures the image size</p></qt>" );
    QWhatsThis::add( previewSizeLabel, txt );
    QWhatsThis::add( _previewSize, txt );

    txt = i18n( "<qt><p>A 128x128 pixel thumbnail will take up approximately 64KB of memory from your X server. "
                "This may not sound like much but try changing the number of images to be displayed to 3000. "
                "You will see that your X server requires approximately 200Mb of memory, just to show the KimDaBa "
                "thumbnail overview.</p>"
                "<p>Besides, showing 3000 thumbnails will take time to display, and be of little use. "
                "The conclusion therefore is to keep this value to a reasonable limit that fits your needs, and "
                "the amount of installed memory in your system.</p></qt>" );

    QWhatsThis::add( maxImagesLabel, txt );
    QWhatsThis::add( _maxImages, txt );

    txt = i18n("<qt>Checking this option will show the base name for the file under "
               "thumbnails in the thumbnail view</qt>");
    QWhatsThis::add( _displayLabels, txt );

    txt = i18n("<qt><p>If you select this option, then the thumbnail view will be shown, when the number of images you have browsed to "
               "gets below the number of images that can be shown in a single view. The alternative is to continue showing the "
               "browser until you press <i>Show Images</i>.<p>"
               "<p>With this option on, you can choose to see the browser by pressing ctrl+mouse button.<br>"
               "With this option off, you can choose to see the images by pressing ctrl+mouse button.</p></qt>");
    QWhatsThis::add( _autoShowThumbnailView, txt );
}


class OptionGroupItem :public QListBoxText
{
public:
    OptionGroupItem( const QString& optionGroup, const QString& text, const QString& icon,
                     Options::ViewSize size, Options::ViewType type, QListBox* parent );
    void setLabel( const QString& label );

    QString _optionGroupOrig, _textOrig, _iconOrig;
    QString _text, _icon;
    Options::ViewSize _size, _sizeOrig;
    Options::ViewType _type, _typeOrig;
};

OptionGroupItem::OptionGroupItem( const QString& optionGroup, const QString& text, const QString& icon,
                                  Options::ViewSize size, Options::ViewType type, QListBox* parent )
    :QListBoxText( parent, text ),
     _optionGroupOrig( optionGroup ), _textOrig( text ), _iconOrig( icon ),
     _text( text ), _icon( icon ), _size( size ), _sizeOrig( size ), _type( type ), _typeOrig( type )
{
}

void OptionGroupItem::setLabel( const QString& label )
{
    setText( label );
    _text = label;

    // unfortunately setText do not call updateItem, so we need to force it with this hack.
    bool b = listBox()->isSelected( this );
    listBox()->setSelected( this, !b );
    listBox()->setSelected( this, b );
}


void OptionsDialog::createOptionGroupsPage()
{
    QWidget* top = addPage( i18n("Option Groups"), i18n("Option Groups"),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "identity" ),
                                                             KIcon::Desktop, 32 ) );

    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );
    QHBoxLayout* lay2 = new QHBoxLayout( lay1, 6 );

    _optionGroups = new QListBox( top );
    connect( _optionGroups, SIGNAL( clicked( QListBoxItem* ) ), this, SLOT( edit( QListBoxItem* ) ) );
    lay2->addWidget( _optionGroups );

    QVBoxLayout* lay3 = new QVBoxLayout( lay2, 6 );
    QLabel* label = new QLabel( i18n( "Label:" ), top );
    lay3->addWidget( label );

    _text = new QLineEdit( top );
    connect( _text, SIGNAL( textChanged( const QString& ) ),
             this, SLOT( slotLabelChanged( const QString& ) ) );

    lay3->addWidget( _text );

    _icon = new KIconButton(  top );
    QHBoxLayout* lay5 = new QHBoxLayout( lay3 );
    lay5->addWidget( _icon );
    lay5->addStretch(1);
    _icon->setIconSize(32);
    _icon->setIcon( QString::fromLatin1( "personsIcon" ) );
    connect( _icon, SIGNAL( iconChanged( QString ) ), this, SLOT( slotIconChanged( QString ) ) );
    lay3->addStretch(1);

    // Prefered View
    _preferredViewLabel = new QLabel( i18n("Preferred view:"), top );
    lay3->addWidget( _preferredViewLabel );

    _preferredView = new QComboBox( top );
    lay3->addWidget( _preferredView );
    QStringList list;
    list << i18n("Small List View") << i18n("Large List View") << i18n("Small Icon View") << i18n("Large Icon View");
    _preferredView->insertStringList( list );
    connect( _preferredView, SIGNAL( activated( int ) ), this, SLOT( slotPreferredViewChanged( int ) ) );

    QHBoxLayout* lay4 = new QHBoxLayout( lay1, 6 );
    KPushButton* newItem = new KPushButton( i18n("New"), top );
    connect( newItem, SIGNAL( clicked() ), this, SLOT( slotNewItem() ) );

    _delItem = new KPushButton( i18n("Delete"), top );
    connect( _delItem, SIGNAL( clicked() ), this, SLOT( slotDeleteCurrent() ) );

    lay4->addStretch(1);
    lay4->addWidget( newItem );
    lay4->addWidget( _delItem );

    _current = 0;
}



void OptionsDialog::show()
{
    Options* opt = Options::instance();

    // General page
    _thumbnailSize->setValue( opt->thumbSize() );
    _previewSize->setValue( opt->previewSize() );
    _trustTimeStamps->setCurrentItem( opt->tTimeStamps() );
    _useEXIFRotate->setChecked( opt->useEXIFRotate() );
    _useEXIFComments->setChecked( opt->useEXIFComments() );
    _searchForImagesOnStartup->setChecked( opt->searchForImagesOnStartup() );
    _autosave->setValue( opt->autoSave() );
    _albumCategory->setCurrentText( opt->albumCategory() );
    _displayLabels->setChecked( opt->displayLabels() );
    _backgroundColor->setColor( opt->thumbNailBackgroundColor() );
    _maxImages->setValue( opt->maxImages() );
    _viewImageSetup->setSize( opt->viewerSize() );
    _viewImageSetup->setLaunchFullScreen( opt->launchViewerFullScreen() );
    _slideShowSetup->setSize( opt->slideShowSize() );
    _slideShowSetup->setLaunchFullScreen( opt->launchSlideShowFullScreen() );
    _slideShowInterval->setValue( opt->slideShowInterval() );
    _cacheSize->setValue( opt->viewerCacheSize() );
    _autoShowThumbnailView->setChecked( opt->autoShowThumbnailView() );

    // Config Groups page
    _optionGroups->clear();
    QStringList grps = opt->optionGroups();
    for( QStringList::Iterator it = grps.begin(); it != grps.end(); ++it ) {
        if( *it == QString::fromLatin1( "Folder") ) {
            continue; // make it impossible to change the name or the icon of the folder
        }
        new OptionGroupItem( *it, opt->textForOptionGroup( *it ), opt->iconNameForOptionGroup( *it ),
                             opt->viewSize( *it ), opt->viewType( *it ), _optionGroups );
    }
    enableDisable( false );
    KDialogBase::show();
}



// KDialogBase has a slotOK which we do not want to override.
void OptionsDialog::slotMyOK()
{
    Options* opt = Options::instance();

    // General
    opt->setThumbSize( _thumbnailSize->value() );
    opt->setPreviewSize( _previewSize->value() );
    opt->setTTimeStamps( (Options::TimeStampTrust) _trustTimeStamps->currentItem() );
    opt->setUseEXIFRotate( _useEXIFRotate->isChecked() );
    opt->setUseEXIFComments( _useEXIFComments->isChecked() );
    opt->setSearchForImagesOnStartup( _searchForImagesOnStartup->isChecked() );
    opt->setAutoSave( _autosave->value() );
    opt->setAlbumCategory( _albumCategory->currentText() );
    opt->setDisplayLabels( _displayLabels->isChecked() );
    opt->setThumbNailBackgroundColor( _backgroundColor->color() );
    opt->setMaxImages( _maxImages->value() );
    opt->setViewerSize( _viewImageSetup->size() );
    opt->setLaunchViewerFullScreen( _viewImageSetup->launchFullScreen() );
    opt->setSlideShowInterval( _slideShowInterval->value() );
    opt->setViewerCacheSize( _cacheSize->value() );
    opt->setSlideShowSize( _slideShowSetup->size() );
    opt->setLaunchSlideShowFullScreen( _slideShowSetup->launchFullScreen() );
    opt->setAutoShowThumbnailView( _autoShowThumbnailView->isChecked() );

    // ----------------------------------------------------------------------
    // Option Groups

    // Delete items
    for( QValueList<OptionGroupItem*>::Iterator it = _deleted.begin(); it != _deleted.end(); ++it ) {
        if ( !(*it)->_optionGroupOrig.isNull() ) {
            // the Options instance knows about the item.
            opt->deleteOptionGroup( (*it)->_optionGroupOrig );
        }
    }

    // Created or Modified items
    for ( QListBoxItem* i = _optionGroups->firstItem(); i; i = i->next() ) {
        OptionGroupItem* item = static_cast<OptionGroupItem*>( i );
        if ( item->_optionGroupOrig.isNull() ) {
            // New Item
            opt->addOptionGroup( item->_text, item->_icon, item->_size, item->_type );
        }
        else {
            if ( item->_text != item->_textOrig ) {
                opt->renameOptionGroup( item->_optionGroupOrig, item->_text );
                item->_optionGroupOrig =item->_text;
            }
            if ( item->_icon != item->_iconOrig ) {
                opt->setIconForOptionGroup( item->_optionGroupOrig, item->_icon );
            }
            if ( item->_size != item->_sizeOrig ) {
                opt->setViewSize( item->_optionGroupOrig, item->_size );
            }
            if ( item->_type != item->_typeOrig ) {
                opt->setViewType( item->_optionGroupOrig, item->_type );
            }
        }
    }

    saveOldGroup();
    opt->setMemberMap( _memberMap );

    // misc stuff
#ifdef HASKIPI
    _pluginConfig->apply();
#endif
    emit changed();
}


void OptionsDialog::edit( QListBoxItem* i )
{
    if ( i == 0 )
        return;

    OptionGroupItem* item = static_cast<OptionGroupItem*>(i);
    _current = item;
    _text->setText( item->_text );
    _icon->setIcon( item->_icon );
    _preferredView->setCurrentItem( (int) item->_size + 2 * (int) item->_type );
    enableDisable( true );
}

void OptionsDialog::slotLabelChanged( const QString& label)
{
    if( _current )
        _current->setLabel( label );
}

void OptionsDialog::slotPreferredViewChanged( int i )
{
    if ( _current ) {
        if ( i < 2 )
            _current->_type = Options::ListView;
        else
            _current->_type = Options::IconView;

        if ( i % 2 == 1 )
            _current->_size = Options::Large;
        else
            _current->_size = Options::Small;
    }
}



void OptionsDialog::slotIconChanged( QString icon )
{
    if( _current )
        _current->_icon = icon;
}

void OptionsDialog::slotNewItem()
{
    _current = new OptionGroupItem( QString::null, QString::null, QString::null, Options::Small, Options::ListView, _optionGroups );
    _text->setText( QString::fromLatin1( "" ) );
    _icon->setIcon( QString::null );
    enableDisable( true );
    _optionGroups->setSelected( _current, true );
    _text->setFocus();
}

void OptionsDialog::slotDeleteCurrent()
{
    int count = ImageDB::instance()->countItemsOfOptionGroup( _current->_optionGroupOrig );
    int answer = KMessageBox::Yes;
    if ( count != 0 )
        KMessageBox::questionYesNo( this,
                                    i18n("<qt>Really delete group '%1'?<br>"
                                         "%2 images contains information in that group")
                                    .arg( _current->_text).arg(count) );
    if ( answer == KMessageBox::No )
        return;

    _deleted.append( _current );
    _optionGroups->takeItem( _current );
    _current = 0;
    _text->setText( QString::fromLatin1( "" ) );
    _icon->setIcon( QString::null );
    enableDisable(false);
}

void OptionsDialog::enableDisable( bool b )
{
    _delItem->setEnabled( b );
    _text->setEnabled( b );
    _icon->setEnabled( b );
    _preferredViewLabel->setEnabled( b );
    _preferredView->setEnabled( b );
}













void OptionsDialog::createGroupConfig()
{
    Options* opt = Options::instance();

    QWidget* top = addPage( i18n("Member Groups" ), i18n("Member Groups" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "kuser" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    // Category
    QHBoxLayout* lay2 = new QHBoxLayout( lay1, 6 );
    QLabel* label = new QLabel( i18n( "Category:" ), top );
    lay2->addWidget( label );
    _category = new QComboBox( top );
    lay2->addWidget( _category );
    lay2->addStretch(1);

    QHBoxLayout* lay3 = new QHBoxLayout( lay1, 6 );

    // Groups
    QVBoxLayout* lay4 = new QVBoxLayout( lay3, 6 );
    label = new QLabel( i18n( "Groups:" ), top );
    lay4->addWidget( label );
    _groups = new QListBox( top );
    lay4->addWidget( _groups );

    // Members
    QVBoxLayout* lay5 = new QVBoxLayout( lay3, 6 );
    label = new QLabel( i18n( "Members:" ), top );
    lay5->addWidget( label );
    _members = new QListBox( top );
    lay5->addWidget( _members );

    // Buttons
    QHBoxLayout* lay6 = new QHBoxLayout( lay1, 6 );
    lay6->addStretch(1);

    QPushButton* add = new QPushButton( i18n("Add Group..." ), top );
    lay6->addWidget( add );
    _rename = new QPushButton( i18n( "Rename Group..."), top );
    lay6->addWidget( _rename );
    _del = new QPushButton( i18n("Delete Group" ), top );
    lay6->addWidget( _del );

    // Setup the actions
    _memberMap = opt->memberMap();
    connect( _category, SIGNAL( activated( const QString& ) ), this, SLOT( slotCategoryChanged( const QString& ) ) );
    connect( _groups, SIGNAL( clicked( QListBoxItem* ) ), this, SLOT( slotGroupSelected( QListBoxItem* ) ) );
    connect( _rename, SIGNAL( clicked() ), this, SLOT( slotRenameGroup() ) );
    connect( add, SIGNAL( clicked() ), this, SLOT( slotAddGroup() ) );
    connect( _del, SIGNAL( clicked() ), this, SLOT( slotDelGroup() ) );

    _members->setSelectionMode( QListBox::Multi );
}

/**
   When the user selects a new optionGroup from the combo box then this method is called
   Its purpose is too fill the groups and members listboxes.
*/
void OptionsDialog::slotCategoryChanged( const QString& name )
{
    slotCategoryChanged( name, true );
}

void OptionsDialog::slotCategoryChanged( const QString& name, bool saveGroups )
{
    if ( saveGroups ) {
        // We do not want to save groups when renaming categories
        saveOldGroup();
    }

    _groups->clear();
    _currentCategory = name;
    QStringList groupList = _memberMap.groups( name );
    _groups->insertStringList( groupList );

    _members->clear();
    QStringList list = Options::instance()->optionValue(name);
    list += _memberMap.groups( name );
    QStringList uniq;
    for( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        if ( !uniq.contains(*it) )
            uniq << *it;
    }

    uniq.sort();
    _members->insertStringList( uniq );

    if ( !groupList.isEmpty() ) {
        _groups->setSelected( 0, true );
        selectMembers( _groups->text(0) );
    }
    else
        _currentGroup = QString::null;

    setButtonStates();
}

void OptionsDialog::slotGroupSelected( QListBoxItem* item )
{
    saveOldGroup();
    if ( item )
        selectMembers( item->text() );
}

void OptionsDialog::slotAddGroup()
{
    bool ok;
    QString text = KInputDialog::getText( i18n( "New Group" ), i18n("Group name:"), QString::null, &ok );
    if ( ok ) {
        saveOldGroup();
        QListBoxItem* item = new QListBoxText( _groups, text );
        _groups->setCurrentItem( item );
        selectMembers( text );
        Options::instance()->addOption( _currentCategory, text );
        _memberMap.setMembers(_currentCategory, text, QStringList() );
        slotCategoryChanged( _currentCategory, false );
    }
}

void OptionsDialog::slotRenameGroup()
{
    Q_ASSERT( !_currentGroup.isNull() );
    bool ok;
    QListBoxItem* item = _groups->item( _groups->currentItem() );
    QString currentValue = item->text();
    QString text = KInputDialog::getText( i18n( "New Group" ), i18n("Group name:"), currentValue, &ok );
    if ( ok ) {
        saveOldGroup();
        _memberMap.renameGroup( _currentCategory, currentValue, text );
        Options::instance()->renameOption( _currentCategory, currentValue, text );
        slotCategoryChanged( _currentCategory, false );
    }
}

void OptionsDialog::slotDelGroup()
{
    Q_ASSERT( !_currentGroup.isNull() );
    int res = KMessageBox::warningYesNo( this, i18n( "Really delete group %1" ).arg( _currentGroup ) );
    if ( res == KMessageBox::No )
        return;

    saveOldGroup();

    QListBoxItem* item = _groups->findItem( _currentGroup );
    delete item;

    _memberMap.deleteGroup( _currentCategory, _currentGroup );
    Options::instance()->removeOption( _currentCategory, _currentGroup );
    _currentGroup = _groups->text(0);
    slotCategoryChanged( _currentCategory, false );
    selectMembers( _currentGroup );
    setButtonStates();
}

void OptionsDialog::saveOldGroup()
{
    if ( _currentCategory.isNull() || _currentGroup.isNull() )
        return;

    QStringList list;
    for( QListBoxItem* item = _members->firstItem(); item; item = item->next() ) {
        if ( item->isSelected() )
            list << item->text();
    }

    _memberMap.setMembers(_currentCategory, _currentGroup, list);
}

void OptionsDialog::selectMembers( const QString& group )
{
    _currentGroup = group;
    QStringList list = _memberMap.members(_currentCategory,group, false );
    for( QListBoxItem* item = _members->firstItem(); item; item = item->next() ) {
        _members->setSelected( item, list.contains( item->text() ) );
    }
    setButtonStates();
}


int OptionsDialog::exec()
{
    slotCategoryChanged( _currentCategory, false );
    return KDialogBase::exec();
}

void OptionsDialog::setButtonStates()
{
    bool b = !_currentGroup.isNull();
    _rename->setEnabled( b );
    _del->setEnabled( b );
}


void OptionsDialog::slotPageChange()
{
    _category->clear();
    QStringList l = Options::instance()->optionGroups();

    // We do not want to configure folders in the member groups.
    l.remove( QString::fromLatin1("Folder") );
    _category->insertStringList( l );
    slotCategoryChanged( _category->currentText() );
}






void OptionsDialog::createViewerPage()
{
    QWidget* top = addPage( i18n("Viewer" ), i18n("Viewer" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "viewmag" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );

    _slideShowSetup = new ViewerSizeConfig( i18n( "Running Slide Show From Thumbnail View" ), top, "_slideShowSetup" );
    lay1->addWidget( _slideShowSetup );

    _viewImageSetup = new ViewerSizeConfig( i18n( "Viewing Images From Thumbnail View" ), top, "_viewImageSetup" );
    lay1->addWidget( _viewImageSetup );

    QGridLayout* glay = new QGridLayout( lay1, 2, 2, 6 );

    QLabel* label = new QLabel( i18n("Slideshow interval:" ), top );
    glay->addWidget( label, 0, 0 );

    _slideShowInterval = new QSpinBox( 1, INT_MAX, 1, top );
    glay->addWidget( _slideShowInterval, 0, 1 );
    _slideShowInterval->setSuffix( i18n( " sec" ) );

    label = new QLabel( i18n("Image cache:"), top );
    glay->addWidget( label, 1, 0 );

    _cacheSize = new QSpinBox( 0, 2000, 10, top, "_cacheSize" );
    _cacheSize->setSuffix( i18n(" Mbytes") );
    glay->addWidget( _cacheSize, 1, 1 );
}


void OptionsDialog::createPluginPage()
{
#ifdef HASKIPI
    QWidget* top = addPage( i18n("Plugins" ), i18n("Plugins" ),
                            KGlobal::iconLoader()->loadIcon( QString::fromLatin1( "share" ),
                                                             KIcon::Desktop, 32 ) );
    QVBoxLayout* lay1 = new QVBoxLayout( top, 6 );
    _pluginConfig = KIPI::PluginLoader::instance()->configWidget( top );
    lay1->addWidget( _pluginConfig );
#endif
}

#include "optionsdialog.moc"
