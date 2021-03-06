/* Copyright (C) 2003-2015 Jesper K. Pedersen <blackie@kde.org>

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

#include "ListSelect.h"
#include "config-kpa-kface.h"
#include <qlayout.h>
#include <QLabel>
#include <QMenu>
#include <QList>
#include <QMouseEvent>
#include <klocale.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include "DB/ImageDB.h"
#include <kio/copyjob.h>
#include <qtoolbutton.h>
#include <QButtonGroup>
#include "DB/MemberMap.h"
#include <Utilities/Set.h>
#include "CompletableLineEdit.h"
#include "DB/CategoryItem.h"
#include "ListViewItemHider.h"
#include "ShowSelectionOnlyManager.h"
#include "CategoryListView/DragableTreeWidget.h"
#include "CategoryListView/CheckDropItem.h"
#include <qradiobutton.h>
#include <QWidgetAction>
#include <QHeaderView>
#include "Dialog.h"
#include <QDebug>

using namespace AnnotationDialog;
using CategoryListView::CheckDropItem;

AnnotationDialog::ListSelect::ListSelect( const DB::CategoryPtr& category, QWidget* parent )
    : QWidget( parent ), m_category( category ), m_baseTitle( )
{
    QVBoxLayout* layout = new QVBoxLayout( this );

    m_lineEdit = new CompletableLineEdit( this );
    m_lineEdit->setProperty( "FocusCandidate", true );
    m_lineEdit->setProperty( "WantsFocus", true );
    m_lineEdit->setObjectName( category->name() );
    layout->addWidget( m_lineEdit );

    // PENDING(blackie) rename instance variable to something better than _listView
    m_treeWidget = new CategoryListView::DragableTreeWidget( m_category, this );
    m_treeWidget->setHeaderLabel( QString::fromLatin1( "items" ) );
    m_treeWidget->header()->hide();
    connect( m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)),  this,  SLOT(itemSelected(QTreeWidgetItem*)) );
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( m_treeWidget, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(showContextMenu(QPoint)));
    connect( m_treeWidget, SIGNAL(itemsChanged()), this, SLOT(rePopulate()) );
    connect( m_treeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(updateSelectionCount()) );

    layout->addWidget( m_treeWidget );

    // Merge CheckBox
    QHBoxLayout* lay2 = new QHBoxLayout;
    layout->addLayout( lay2 );

    m_or = new QRadioButton( i18n("or"), this );
    m_and = new QRadioButton( i18n("and"), this );
    lay2->addWidget( m_or );
    lay2->addWidget( m_and );
    lay2->addStretch(1);

    // Sorting tool button
    QButtonGroup* grp = new QButtonGroup( this );
    grp->setExclusive( true );

    m_alphaTreeSort = new QToolButton;
    m_alphaTreeSort->setIcon( SmallIcon( QString::fromLatin1( "view-list-tree" ) ) );
    m_alphaTreeSort->setCheckable( true );
    m_alphaTreeSort->setToolTip( i18n("Sort Alphabetically (Tree)") );
    grp->addButton( m_alphaTreeSort );

    m_alphaFlatSort = new QToolButton;
    m_alphaFlatSort->setIcon( SmallIcon( QString::fromLatin1( "draw-text" ) ) );
    m_alphaFlatSort->setCheckable( true );
    m_alphaFlatSort->setToolTip( i18n("Sort Alphabetically (Flat)") );
    grp->addButton( m_alphaFlatSort );

    m_dateSort = new QToolButton;
    m_dateSort->setIcon( SmallIcon( QString::fromLatin1( "x-office-calendar" ) ) );
    m_dateSort->setCheckable( true );
    m_dateSort->setToolTip( i18n("Sort by date") );
    grp->addButton( m_dateSort );

    m_showSelectedOnly = new QToolButton;
    m_showSelectedOnly->setIcon( SmallIcon( QString::fromLatin1( "view-filter" ) ) );
    m_showSelectedOnly->setCheckable( true );
    m_showSelectedOnly->setToolTip( i18n("Show only selected Ctrl+S") );
    m_showSelectedOnly->setChecked( ShowSelectionOnlyManager::instance().selectionIsLimited() );

    m_alphaTreeSort->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaTree );
    m_alphaFlatSort->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaFlat );
    m_dateSort->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortLastUse );
    connect( m_dateSort, SIGNAL(clicked()), this, SLOT(slotSortDate()) );
    connect( m_alphaTreeSort, SIGNAL(clicked()), this, SLOT(slotSortAlphaTree()) );
    connect( m_alphaFlatSort, SIGNAL(clicked()), this, SLOT(slotSortAlphaFlat()) );
    connect( m_showSelectedOnly, SIGNAL(clicked()), &ShowSelectionOnlyManager::instance(), SLOT(toggle()) );

    lay2->addWidget( m_alphaTreeSort );
    lay2->addWidget( m_alphaFlatSort );
    lay2->addWidget( m_dateSort );
    lay2->addWidget( m_showSelectedOnly );

    m_lineEdit->setListView( m_treeWidget );

    connect( m_lineEdit, SIGNAL(returnPressed(QString)),  this,  SLOT(slotReturn()) );

    populate();

    connect( Settings::SettingsData::instance(), SIGNAL(viewSortTypeChanged(Settings::ViewSortType)),
             this, SLOT(setViewSortType(Settings::ViewSortType)) );
    connect( Settings::SettingsData::instance(), SIGNAL(matchTypeChanged(AnnotationDialog::MatchType)),
             this, SLOT(updateListview()) );

    connect( &ShowSelectionOnlyManager::instance(), SIGNAL(limitToSelected()), this, SLOT(limitToSelection()) );
    connect( &ShowSelectionOnlyManager::instance(), SIGNAL(broaden()), this, SLOT(showAllChildren()) );
}

void AnnotationDialog::ListSelect::slotReturn()
{
    if (isInputMode()) {
        QString enteredText = m_lineEdit->text().trimmed();
        if (enteredText.isEmpty()) {
            return;
        }

        if (searchForUntaggedImagesTagNeeded()) {
            if (enteredText == Settings::SettingsData::instance()->untaggedTag()) {
                KMessageBox::information(
                    this,
                    i18n("The tag you entered is the tag that is set automatically for newly "
                         "found, untagged images (cf. <interface>Settings|Configure KPhotoAlbum..."
                         "|Categories|Untagged Images</interface>). It will not show up here as "
                         "long as it is selected for this purpose.")
                );
                m_lineEdit->setText(QString());
                return;
            }
        }

        m_category->addItem(enteredText);
        rePopulate();

        QList<QTreeWidgetItem*> items = m_treeWidget->findItems(enteredText, Qt::MatchExactly, 0);
        if (! items.isEmpty()) {
            items.at(0)->setCheckState(0, Qt::Checked);
            if (m_positionable) {
                emit positionableTagSelected(m_category->name(), items.at(0)->text(0));
            }
        } else {
            Q_ASSERT(false);
        }

        m_lineEdit->clear();
    }
    updateSelectionCount();
}

QString AnnotationDialog::ListSelect::category() const
{
    return m_category->name();
}

void AnnotationDialog::ListSelect::setSelection( const StringSet& on, const StringSet& partiallyOn )
{
    for ( QTreeWidgetItemIterator itemIt( m_treeWidget ); *itemIt; ++itemIt ) {
        if ( partiallyOn.contains( (*itemIt)->text(0) ) )
            (*itemIt)->setCheckState( 0, Qt::PartiallyChecked );
        else
            (*itemIt)->setCheckState( 0, on.contains( (*itemIt)->text(0) ) ? Qt::Checked : Qt::Unchecked );
    }

    m_lineEdit->clear();
    updateSelectionCount();
}

bool AnnotationDialog::ListSelect::isAND() const
{
    return m_and->isChecked();
}

void AnnotationDialog::ListSelect::setMode( UsageMode mode )
{
    m_mode = mode;
    m_lineEdit->setMode( mode );
    if ( mode == SearchMode ) {
        // "0" below is sorting key which ensures that None is always at top.
        CheckDropItem* item = new CheckDropItem( m_treeWidget, DB::ImageDB::NONE(), QString::fromLatin1("0") );
        configureItem( item );
        m_and->show();
        m_or->show();
        m_or->setChecked( true );
        m_showSelectedOnly->hide();
    } else {
        m_and->hide();
        m_or->hide();
        m_showSelectedOnly->show();
    }
    for ( QTreeWidgetItemIterator itemIt( m_treeWidget ); *itemIt; ++itemIt )
        configureItem( dynamic_cast<CategoryListView::CheckDropItem*>(*itemIt) );

    // ensure that the selection count indicator matches the current mode:
    updateSelectionCount();
}


void AnnotationDialog::ListSelect::setViewSortType( Settings::ViewSortType tp )
{
    showAllChildren();

    // set sortType and redisplay with new sortType
    QString text = m_lineEdit->text();
    rePopulate();
    m_lineEdit->setText( text );
    setMode( m_mode ); // generate the ***NONE*** entry if in search mode

    m_alphaTreeSort->setChecked( tp == Settings::SortAlphaTree );
    m_alphaFlatSort->setChecked( tp == Settings::SortAlphaFlat );
    m_dateSort->setChecked( tp == Settings::SortLastUse );
}

QString AnnotationDialog::ListSelect::text() const
{
    return m_lineEdit->text();
}

void AnnotationDialog::ListSelect::setText( const QString& text )
{
    m_lineEdit->setText( text );
    m_treeWidget->clearSelection();
}

void AnnotationDialog::ListSelect::itemSelected(QTreeWidgetItem *item )
{
    if ( !item ) {
        // click outside any item
        return;
    }

    if ( m_mode == SearchMode )  {
        QString txt = item->text(0);
        QString res;
        QRegExp regEnd( QString::fromLatin1("\\s*[&|!]\\s*$") );
        QRegExp regStart( QString::fromLatin1("^\\s*[&|!]\\s*") );

        if (item->checkState(0) == Qt::Checked) {
            int matchPos = m_lineEdit->text().indexOf(txt);
            if (matchPos != -1) {
                return;
            }

            int index = m_lineEdit->cursorPosition();
            QString start = m_lineEdit->text().left(index);
            QString end = m_lineEdit->text().mid(index);

            res = start;
            if (! start.isEmpty() && ! start.contains(regEnd)) {
                res += isAND() ? QString::fromLatin1(" & ") : QString::fromLatin1(" | ") ;
            }
            res += txt;
            if (! end.isEmpty() && ! end.contains(regStart)) {
                res += isAND() ? QString::fromLatin1(" & ") : QString::fromLatin1(" | ") ;
            }
            res += end;
        } else {
            int index = m_lineEdit->text().indexOf( txt );
            if ( index == -1 )
                return;

            QString start = m_lineEdit->text().left(index);
            QString end =  m_lineEdit->text().mid(index + txt.length() );
            if ( start.contains( regEnd ) )
                start.replace( regEnd, QString::fromLatin1("") );
            else
                end.replace( regStart,  QString::fromLatin1("") );

            res = start + end;
        }
        m_lineEdit->setText( res );
    }

    else {
        if (m_positionable) {
            if (item->checkState(0) == Qt::Checked) {
                emit positionableTagSelected(m_category->name(), item->text(0));
            } else {
                emit positionableTagDeselected(m_category->name(), item->text(0));
            }
        }

        m_lineEdit->clear();
        showAllChildren();
        ensureAllInstancesAreStateChanged( item );
    }
}

void AnnotationDialog::ListSelect::showContextMenu(const QPoint& pos)
{
    QMenu* menu = new QMenu( this );

    QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
    // click on any item
    QString title = i18n("No Item Selected");
    if ( item )
        title = item->text(0);

    QLabel* label = new QLabel( QString::fromLatin1("<b>%1</b>").arg(title), menu );
    label->setAlignment( Qt::AlignCenter );
    QWidgetAction* action = new QWidgetAction(menu);
    action->setDefaultWidget( label );
    menu->addAction(action);

    QAction* deleteAction = menu->addAction( SmallIcon(QString::fromLatin1("edit-delete")), i18n("Delete") );
    QAction* renameAction = menu->addAction( i18n("Rename...") );

    QLabel* categoryTitle = new QLabel( i18n("<b>Tag Groups</b>"), menu );
    categoryTitle->setAlignment( Qt::AlignCenter );
    action = new QWidgetAction( menu );
    action->setDefaultWidget( categoryTitle );
    menu->addAction( action );

    // -------------------------------------------------- Add/Remove member group
    DB::MemberMap& memberMap = DB::ImageDB::instance()->memberMap();
    QMenu* members = new QMenu( i18n( "Tag groups" ) );
    menu->addMenu( members );
    QAction* newCategoryAction = nullptr;
    if ( item ) {
        QStringList grps = memberMap.groups( m_category->name() );

        for( QStringList::ConstIterator it = grps.constBegin(); it != grps.constEnd(); ++it ) {
            if (!memberMap.canAddMemberToGroup(m_category->name(), *it, item->text(0)))
                continue;
            QAction* action = members->addAction( *it );
            action->setCheckable(true);
            action->setChecked( (bool) memberMap.members( m_category->name(), *it, false ).contains( item->text(0) ) );
            action->setData( *it );
        }

        if ( !grps.isEmpty() )
            members->addSeparator();
        newCategoryAction = members->addAction( i18n("Add this tag to a new tag group..." ) );
    }

    QAction* newSubcategoryAction = menu->addAction( i18n( "Make this tag a tag group and add a tag..." ) );

    // -------------------------------------------------- Take item out of category
    QTreeWidgetItem* parent = item ? item->parent() : nullptr;
    QAction* takeAction = nullptr;
    if ( parent )
        takeAction = menu->addAction( i18n( "Remove from tag group %1", parent->text(0) ) );

    // -------------------------------------------------- sort
    QLabel* sortTitle = new QLabel( i18n("<b>Sorting</b>") );
    sortTitle->setAlignment( Qt::AlignCenter );
    action = new QWidgetAction( menu );
    action->setDefaultWidget( sortTitle );
    menu->addAction( action );

    QAction* usageAction = menu->addAction( i18n("Usage") );
    QAction* alphaFlatAction = menu->addAction( i18n("Alphabetical (Flat)") );
    QAction* alphaTreeAction = menu->addAction( i18n("Alphabetical (Tree)") );
    usageAction->setCheckable(true);
    usageAction->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortLastUse);
    alphaFlatAction->setCheckable(true);
    alphaFlatAction->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaFlat);
    alphaTreeAction->setCheckable(true);
    alphaTreeAction->setChecked( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaTree);

    if ( !item ) {
        deleteAction->setEnabled( false );
        renameAction->setEnabled( false );
        members->setEnabled( false );
        newSubcategoryAction->setEnabled( false );
    }
    // -------------------------------------------------- exec
    QAction* which = menu->exec( m_treeWidget->mapToGlobal(pos));
    if ( which == nullptr )
        return;

    else if ( which == deleteAction ) {
        int code = KMessageBox::warningContinueCancel( this, i18n("<p>Do you really want to delete \"%1\"?<br/>"
                                                                  "Deleting the item will remove any information "
                                                                  "about it from any image containing the item.</p>"
                                                       ,item->text(0)),
                                                       i18n("Really Delete %1?",item->text(0)),
                                                       KGuiItem(i18n("&Delete"),QString::fromLatin1("editdelete")) );
        if ( code == KMessageBox::Continue ) {
            if (item->checkState(0) == Qt::Checked and m_positionable) {
                // An area could be linked against this. We can use positionableTagDeselected
                // here, as the procedure is the same as if the tag had been deselected.
                emit positionableTagDeselected(m_category->name(), item->text(0));
            }

#ifdef HAVE_KFACE
            // Also delete this tag from the recognition database (if it's there)
            if (m_positionable) {
                m_recognizer = FaceManagement::Recognizer::instance();
                m_recognizer->deleteTag(m_category->name(), item->text(0));
            }
#endif

            m_category->removeItem( item->text(0) );
            rePopulate();
        }
    }
    else if ( which == renameAction ) {
        bool ok;
        QString newStr = KInputDialog::getText( i18n("Rename Item"), i18n("Enter new name:"),
                                                item->text(0), &ok, this );

        if ( ok && newStr != item->text(0) ) {
            int code = KMessageBox::questionYesNo( this, i18n("<p>Do you really want to rename \"%1\" to \"%2\"?<br/>"
                                                              "Doing so will rename \"%3\" "
                                                              "on any image containing it.</p>"
                                               ,item->text(0),newStr,item->text(0)),
                                               i18n("Really Rename %1?",item->text(0)) );
            if ( code == KMessageBox::Yes ) {
                QString oldStr = item->text(0);
                m_category->renameItem( oldStr, newStr );
                bool checked = item->checkState(0) == Qt::Checked;
                rePopulate();
                // rePopuldate doesn't ask the backend if the item should be checked, so we need to do that.
                checkItem( newStr, checked );

                // rename the category image too
                QString oldFile = m_category->fileForCategoryImage( category(), oldStr );
                QString newFile = m_category->fileForCategoryImage( category(), newStr );
                KIO::move( KUrl(oldFile), KUrl(newFile) );

                if (m_positionable) {
#ifdef HAVE_KFACE
                    // Handle the identity name that we probably have in the recognition database
                    m_recognizer = FaceManagement::Recognizer::instance();
                    m_recognizer->changeIdentityName(m_category->name(), oldStr, newStr);
#endif
                    // Also take care of areas that could be linked against this
                    emit positionableTagRenamed(m_category->name(), oldStr, newStr);
                }
            }
        }
    }
    else if ( which == usageAction ) {
        Settings::SettingsData::instance()->setViewSortType( Settings::SortLastUse );
    }
    else if ( which == alphaTreeAction ) {
        Settings::SettingsData::instance()->setViewSortType( Settings::SortAlphaTree );
    }
    else if ( which == alphaFlatAction ) {
        Settings::SettingsData::instance()->setViewSortType( Settings::SortAlphaFlat );
    }
    else if ( which == newCategoryAction ) {
        QString superCategory = KInputDialog::getText(
            i18n("New tag group"),
            i18n("Name for the new tag group the tag will be added to:")
        );
        if ( superCategory.isEmpty() )
            return;
        memberMap.addGroup( m_category->name(), superCategory );
        memberMap.addMemberToGroup( m_category->name(), superCategory, item->text(0) );
        //DB::ImageDB::instance()->setMemberMap( memberMap );
        rePopulate();
    }
    else if ( which == newSubcategoryAction ) {
        QString subCategory = KInputDialog::getText(
            i18n("Add a tag"),
            i18n("Name for the tag to be added to this tag group:")
        );
        if ( subCategory.isEmpty() )
            return;

         m_category->addItem( subCategory );
         memberMap.addGroup( m_category->name(), item->text(0) );
         memberMap.addMemberToGroup( m_category->name(), item->text(0), subCategory );
         //DB::ImageDB::instance()->setMemberMap( memberMap );
        if ( isInputMode() )
            m_category->addItem( subCategory );

        rePopulate();
        if ( isInputMode() )
            checkItem( subCategory, true );
    }
    else if ( which == takeAction ) {
        memberMap.removeMemberFromGroup( m_category->name(), parent->text(0), item->text(0) );
        rePopulate();
    }
    else {
        QString checkedItem = which->data().value<QString>();
        if ( which->isChecked() ) // choosing the item doesn't check it, so this is the value before.
            memberMap.addMemberToGroup( m_category->name(), checkedItem, item->text(0) );
        else
            memberMap.removeMemberFromGroup( m_category->name(), checkedItem, item->text(0) );
        rePopulate();
    }

    delete menu;
}


void AnnotationDialog::ListSelect::addItems( DB::CategoryItem* item, QTreeWidgetItem* parent )
{
    for( QList<DB::CategoryItem*>::ConstIterator subcategoryIt = item->mp_subcategories.constBegin(); subcategoryIt != item->mp_subcategories.constEnd(); ++subcategoryIt ) {
        CheckDropItem* newItem = nullptr;

        if ( parent == nullptr )
            newItem = new CheckDropItem( m_treeWidget, (*subcategoryIt)->mp_name, QString() );
        else
            newItem = new CheckDropItem( m_treeWidget, parent, (*subcategoryIt)->mp_name, QString() );

        newItem->setExpanded(true);
        configureItem( newItem );

        addItems( *subcategoryIt, newItem );
    }
}

void AnnotationDialog::ListSelect::populate()
{
    m_treeWidget->clear();

    if ( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaTree )
        populateAlphaTree();
    else if ( Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaFlat )
        populateAlphaFlat();
    else
        populateMRU();

    hideUntaggedImagesTag();
}

bool AnnotationDialog::ListSelect::searchForUntaggedImagesTagNeeded()
{
    if (!Settings::SettingsData::instance()->hasUntaggedCategoryFeatureConfigured()
            || Settings::SettingsData::instance()->untaggedImagesTagVisible()) {
        return false;
    }

    if (Settings::SettingsData::instance()->untaggedCategory() != category()) {
        return false;
    }

    return true;
}

void AnnotationDialog::ListSelect::hideUntaggedImagesTag()
{
    if (! searchForUntaggedImagesTagNeeded()) {
        return;
    }

    QTreeWidgetItem* untaggedImagesTag = getUntaggedImagesTag();
    if (untaggedImagesTag) {
        untaggedImagesTag->setHidden(true);
    }
}

void AnnotationDialog::ListSelect::slotSortDate()
{
    Settings::SettingsData::instance()->setViewSortType( Settings::SortLastUse );
}

void AnnotationDialog::ListSelect::slotSortAlphaTree()
{
    Settings::SettingsData::instance()->setViewSortType( Settings::SortAlphaTree );
}

void AnnotationDialog::ListSelect::slotSortAlphaFlat()
{
    Settings::SettingsData::instance()->setViewSortType( Settings::SortAlphaFlat );
}

void AnnotationDialog::ListSelect::rePopulate()
{
    const StringSet on = itemsOn();
    const StringSet noChange = itemsUnchanged();
    populate();
    setSelection( on, noChange );

    if( ShowSelectionOnlyManager::instance().selectionIsLimited() )
        limitToSelection();
}

void AnnotationDialog::ListSelect::showOnlyItemsMatching( const QString& text )
{
    ListViewTextMatchHider dummy( text, Settings::SettingsData::instance()->matchType(), m_treeWidget );
    ShowSelectionOnlyManager::instance().unlimitFromSelection();
}

void AnnotationDialog::ListSelect::populateAlphaTree()
{
    DB::CategoryItemPtr item = m_category->itemsCategories();

    m_treeWidget->setRootIsDecorated( true );
    addItems( item.data(), 0 );
    m_treeWidget->sortByColumn(0, Qt::AscendingOrder);
    m_treeWidget->setSortingEnabled(true);
}

void AnnotationDialog::ListSelect::populateAlphaFlat()
{
    QStringList items = m_category->itemsInclCategories();
    items.sort();

    m_treeWidget->setRootIsDecorated( false );
    for( QStringList::ConstIterator itemIt = items.constBegin(); itemIt != items.constEnd(); ++itemIt ) {
        CheckDropItem* item = new CheckDropItem( m_treeWidget, *itemIt, *itemIt );
        configureItem( item );
    }
    m_treeWidget->sortByColumn(1, Qt::AscendingOrder);
    m_treeWidget->setSortingEnabled(true);
}

void AnnotationDialog::ListSelect::populateMRU()
{
    QStringList items = m_category->itemsInclCategories();

    m_treeWidget->setRootIsDecorated( false );
    int index = 100000; // This counter will be converted to a string, and compared, and we don't want "1111" to be less than "2"
    for( QStringList::ConstIterator itemIt = items.constBegin(); itemIt != items.constEnd(); ++itemIt ) {
        ++index;
        CheckDropItem* item = new CheckDropItem( m_treeWidget, *itemIt, QString::number( index ) );
        configureItem( item );
    }
    m_treeWidget->sortByColumn(1, Qt::AscendingOrder);
    m_treeWidget->setSortingEnabled(true);
}

void AnnotationDialog::ListSelect::toggleSortType()
{
    Settings::SettingsData* data = Settings::SettingsData::instance();
    if ( data->viewSortType() == Settings::SortLastUse )
        data->setViewSortType( Settings::SortAlphaTree );
    else if ( data->viewSortType() == Settings::SortAlphaTree )
        data->setViewSortType( Settings::SortAlphaFlat );
    else
        data->setViewSortType( Settings::SortLastUse );
}

void AnnotationDialog::ListSelect::updateListview()
{
    // update item list (e.g. when MatchType changes):
    showOnlyItemsMatching( text() );
}

void AnnotationDialog::ListSelect::limitToSelection()
{
    if ( !isInputMode() )
        return;

    m_showSelectedOnly->setChecked( true );
    ListViewCheckedHider dummy( m_treeWidget );

    hideUntaggedImagesTag();
}

void AnnotationDialog::ListSelect::showAllChildren()
{
    m_showSelectedOnly->setChecked( false );
    showOnlyItemsMatching( QString() );
    hideUntaggedImagesTag();
}

QTreeWidgetItem* AnnotationDialog::ListSelect::getUntaggedImagesTag()
{
    QList<QTreeWidgetItem*> matchingTags = m_treeWidget->findItems(
        Settings::SettingsData::instance()->untaggedTag(),
        Qt::MatchExactly | Qt::MatchRecursive, 0
    );

    // Be sure not to crash here in case the config points to a non-existent tag
    if (matchingTags.at(0) == nullptr) {
        return 0;
    } else {
        return matchingTags.at(0);
    }
}

void AnnotationDialog::ListSelect::updateSelectionCount()
{
    if (m_baseTitle.isEmpty() /* --> first time */
        || ! parentWidget()->windowTitle().startsWith(m_baseTitle) /* --> title has changed */) {

        // save the original parentWidget title
        m_baseTitle = parentWidget()->windowTitle();
    }

    int itemsOnCount = itemsOn().size();
    // Don't count the untagged images tag:
    if (searchForUntaggedImagesTagNeeded()) {
        QTreeWidgetItem* untaggedImagesTag = getUntaggedImagesTag();
        if (untaggedImagesTag) {
            if (untaggedImagesTag->checkState(0) != Qt::Unchecked) {
                itemsOnCount--;
            }
        }
    }

    switch(m_mode) {
    case InputMultiImageConfigMode:
        if (itemsUnchanged().size() > 0) {
            // if min != max
            // tri-state selection -> show min-max (selected items vs. partially selected items):
            parentWidget()->setWindowTitle(QString::fromUtf8("%1 (%2-%3)")
                .arg(m_baseTitle)
                .arg(itemsOnCount)
                .arg(itemsOnCount + itemsUnchanged().size()));
            break;
        } // else fall through and only show one number:
    case InputSingleImageConfigMode:
        if (itemsOnCount > 0) {
            // if any tags have been selected
            // "normal" on/off states -> show selected items
            parentWidget()->setWindowTitle(QString::fromUtf8("%1 (%2)")
                .arg(m_baseTitle)
                .arg(itemsOnCount));
            break;
        } // else fall through and only show category
    case SearchMode:
        // no indicator while searching
        parentWidget()->setWindowTitle(m_baseTitle);
        break;
    }
}

void AnnotationDialog::ListSelect::configureItem( CategoryListView::CheckDropItem* item )
{
    bool isDNDAllowed = Settings::SettingsData::instance()->viewSortType() == Settings::SortAlphaTree;
    item->setDNDEnabled( isDNDAllowed && ! m_category->isSpecialCategory() );
}

bool AnnotationDialog::ListSelect::isInputMode() const
{
    return m_mode != SearchMode;
}

StringSet AnnotationDialog::ListSelect::itemsOn() const
{
    return itemsOfState( Qt::Checked );
}

StringSet AnnotationDialog::ListSelect::itemsOff() const
{
    return itemsOfState( Qt::Unchecked );
}

StringSet AnnotationDialog::ListSelect::itemsOfState(Qt::CheckState state ) const
{
    StringSet res;
    for ( QTreeWidgetItemIterator itemIt( m_treeWidget ); *itemIt; ++itemIt ) {
        if ( (*itemIt)->checkState(0) == state )
            res.insert( (*itemIt)->text(0) );
    }
    return res;
}

StringSet AnnotationDialog::ListSelect::itemsUnchanged() const
{
    return itemsOfState( Qt::PartiallyChecked );
}

void AnnotationDialog::ListSelect::checkItem( const QString itemText, bool b )
{
    QList<QTreeWidgetItem*> items = m_treeWidget->findItems( itemText, Qt::MatchExactly | Qt::MatchRecursive );
    if ( !items.isEmpty() )
        items.at(0)->setCheckState(0, b ? Qt::Checked : Qt::Unchecked );
    else
        Q_ASSERT( false );
}

/**
 * An item may be member of a number of categories. Mike may be a member of coworkers and friends.
 * Selecting the item in one subcategory, should select him in all.
 */
void AnnotationDialog::ListSelect::ensureAllInstancesAreStateChanged(QTreeWidgetItem *item )
{
    const bool on = item->checkState(0) == Qt::Checked;
    for ( QTreeWidgetItemIterator itemIt( m_treeWidget ); *itemIt; ++itemIt ) {
        if ( (*itemIt) != item && (*itemIt)->text(0) == item->text(0) )
            (*itemIt)->setCheckState(0, on ? Qt::Checked : Qt::Unchecked);
    }
}

QWidget* AnnotationDialog::ListSelect::lineEdit() const
{
    return m_lineEdit;
}

void AnnotationDialog::ListSelect::setPositionable(bool positionableState)
{
    m_positionable = positionableState;
}

bool AnnotationDialog::ListSelect::positionable() const
{
    return m_positionable;
}

bool AnnotationDialog::ListSelect::tagIsChecked(QString tag) const
{
    QList<QTreeWidgetItem *> matchingTags = m_treeWidget->findItems(tag, Qt::MatchExactly | Qt::MatchRecursive, 0);

    if(matchingTags.isEmpty()) {
        return false;
    }

    return (bool) matchingTags.first()->checkState(0);
}

void AnnotationDialog::ListSelect::ensureTagIsSelected(QString category, QString tag)
{
    if (category != m_lineEdit->objectName()) {
        // The selected tag's category does not belong to this ListSelect
        return;
    }

    // Be sure that tag is actually checked
    QList<QTreeWidgetItem *> matchingTags = m_treeWidget->findItems(tag, Qt::MatchExactly | Qt::MatchRecursive, 0);

    // If we have the requested category, but not this tag, add it.
    // This should only happen if the recognition database is copied from another database
    // or has been changed outside of KPA. But this _can_ happen and simply adding a
    // missing tag does not hurt ;-)
    if(matchingTags.isEmpty()) {
        m_category->addItem(tag);
        rePopulate();
        // Now, we find it
        matchingTags = m_treeWidget->findItems(tag, Qt::MatchExactly | Qt::MatchRecursive, 0);
    }

    matchingTags.first()->setCheckState(0, Qt::Checked);
}

#include "ListSelect.moc"
// vi:expandtab:tabstop=4 shiftwidth=4:
