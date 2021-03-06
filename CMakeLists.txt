cmake_minimum_required(VERSION 2.8.9)
project(kphotoalbum)

if(POLICY CMP0017)
    # Prefer files from the CMake module directory when including from there:
    CMAKE_POLICY(SET CMP0017 NEW)
endif()

# provide drop-down menu for build-type in cmake-gui:
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ";Debug;Release;RelWithDebInfo;MinSizeRel")

SET(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# in case we are installing to a non-standard path, expect that other libs are installed there as well:
LIST(INSERT CMAKE_PREFIX_PATH 0 "${CMAKE_INSTALL_PREFIX}")
LIST(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_INSTALL_PREFIX}/share/apps/cmake/modules")

find_package(KDE4 REQUIRED)
include(KDE4Defaults)

# This seems to be needed for KPA to compile with KDE 4.0.0
set(INSTALL_TARGETS_DEFAULT_ARGS
    RUNTIME DESTINATION "${BIN_INSTALL_DIR}"
    LIBRARY DESTINATION "${LIB_INSTALL_DIR}"
    ARCHIVE DESTINATION "${LIB_INSTALL_DIR}"
    COMPONENT Devel
)

ADD_DEFINITIONS(
    ${QT_DEFINITIONS}
    ${KDE4_DEFINITIONS}
    ${KDE4_ENABLE_EXCEPTIONS}
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    #-DDEBUG_JOBMANAGER
)

include(CheckCXX11Features)

SET(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} ${CXX11_COMPILER_FLAGS}"
)

INCLUDE_DIRECTORIES(
    ${KDE4_INCLUDES}
    ${QT_INCLUDES}
)

find_package(JPEG REQUIRED)
if(JPEG_FOUND)
    INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
endif (JPEG_FOUND)

macro_optional_find_package(Kipi)
macro_bool_to_01(KIPI_FOUND HASKIPI)
if(KIPI_FOUND)
    ADD_DEFINITIONS(${KIPI_DEFINITIONS})
    INCLUDE_DIRECTORIES(BEFORE ${KIPI_INCLUDE_DIR})
endif(KIPI_FOUND)

macro_optional_find_package(Kdcraw)
macro_bool_to_01(KDCRAW_FOUND HAVE_KDCRAW)
if(KDCRAW_FOUND)
    ADD_DEFINITIONS(${KDCRAW_DEFINITIONS})
    INCLUDE_DIRECTORIES(BEFORE ${KDCRAW_INCLUDE_DIR})
endif(KDCRAW_FOUND)

macro_optional_find_package(Exiv2)
macro_bool_to_01(EXIV2_FOUND HAVE_EXIV2)

macro_optional_find_package(Marble)
macro_bool_to_01(MARBLE_FOUND HAVE_MARBLE)
if(MARBLE_FOUND)
    INCLUDE_DIRECTORIES(${MARBLE_INCLUDE_DIR})
endif(MARBLE_FOUND)

# Kface 3.5.0 ships with a usable kface-config.cmake
# and face detection/recognition support in KPA depends on KFace >= 3.5.0, anyway.
macro_optional_find_package(Kface 3.5.0 QUIET CONFIG)
macro_bool_to_01(Kface_FOUND HAVE_KFACE)
if(Kface_FOUND)
    message(STATUS "Found KFace version ${Kface_VERSION} (optional)")
    message(STATUS "  Face detection and recognition features will be built.")
    INCLUDE_DIRECTORIES(${Kface_INCLUDE_DIR})
else()
    message(STATUS "Did not find KFace >= 3.5.0 (optional)")
    message(STATUS "  Face detection and recognition features won't be built.")
endif(Kface_FOUND)

if(EXIV2_FOUND)
    macro_optional_find_package(KGeoMap QUIET)
else()
	message(STATUS "Not searching KGeoMap because of missing dependency Exiv2...")
endif()
macro_bool_to_01(KGEOMAP_FOUND HAVE_KGEOMAP)
if(KGEOMAP_FOUND)
    message(STATUS "Found KGeoMap ${KGEOMAP_VERSION} (optional)")
    message(STATUS "  Geographic map features will be built.")
    INCLUDE_DIRECTORIES(${KGEOMAP_INCLUDE_DIR})
else()
    message(STATUS "Did not find KGeoMap (optional)")
    message(STATUS "  Geographic map features won't be built.")
endif(KGEOMAP_FOUND)

find_package(PCRE)
if (PCRE_FOUND) 
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_PCRE")
    message(STATUS "added -DUSE_PCRE to preprocessor defines")
endif()

add_custom_target(
    UpdateVersion ALL
    COMMAND ${CMAKE_COMMAND} -DBASE_DIR="${CMAKE_CURRENT_SOURCE_DIR}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules/UpdateVersion.cmake"
    COMMENT "Updating version header."
)

# For config-kpa-*.h
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

set(libdatebar_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/DateBar/DateBarWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DateBar/ViewHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DateBar/MouseHandler.cpp
)

set(libSettings_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/SettingsData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/SettingsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/ViewerSizeConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/CategoryItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/CategoryPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/TagGroupsPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/GeneralPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/FileVersionDetectionPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/ThumbnailsPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/ViewerPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/DatabaseBackendPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/UntaggedGroupBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/CategoriesGroupsWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/BirthdayPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Settings/DateTableWidgetItem.cpp
)

set(libxmldb_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/Database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/XMLCategoryCollection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/XMLCategory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/XMLImageDateCollection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/NumberedBackup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/FileReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/FileWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/ElementWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/XmlReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/XMLDB/CompressFileInfo.cpp
)

set(libThumbnailView_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailToolTip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/GridResizeInteraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/GridResizeSlider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/SelectionInteraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/MouseTrackingInteraction.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/CellGeometry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailFacade.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/KeyboardEventHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/ThumbnailDND.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/Delegate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/SelectionMaintainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailView/VideoThumbnailCycler.cpp
)

set(libPlugins_SRCS)
if(KIPI_FOUND)
    set(libPlugins_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Interface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/ImageCollection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/ImageInfo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/CategoryImageCollection.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/ImageCollectionSelector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Settings/PluginsPage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/UploadWidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Plugins/UploadImageCollection.cpp
    )
endif(KIPI_FOUND)

set(libViewer_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/ViewerWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/ImageDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/ViewHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/SpeedDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/InfoBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/CategoryImageConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/AbstractDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/VideoDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/TextDisplay.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/InfoBoxResizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/VisibleOptionsMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/VideoShooter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Viewer/TaggedArea.cpp
)

set(libCategoryListView_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/CategoryListView/DragableTreeWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CategoryListView/CheckDropItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CategoryListView/DragItemInfo.cpp
)

set(libHTMLGenerator_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/HTMLGenerator/HTMLDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HTMLGenerator/Generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HTMLGenerator/Setup.cpp
)

set(libUtilities_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/ShowBusyCursor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/List.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Graph.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/UniqFilenameMapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Util.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/BooleanGuard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Set.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/Process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/DeleteFiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Utilities/ToolTip.cpp
)

set(libMainWindow_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/DeleteDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/RunDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/FeatureDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/InvalidDateFinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/AutoStackImages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/TokenEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/WelcomeDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/SplashScreen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/ExternalPopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/CategoryImagePopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/SearchBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/ImageCounter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/DirtyIndicator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/StatisticsDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/BreadcrumbViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/StatusBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/UpdateVideoThumbnail.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/DuplicateMerger/DuplicateMerger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/DuplicateMerger/DuplicateMatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/DuplicateMerger/MergeToolTip.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/MainWindow/CopyPopup.cpp
)

set(libImageManager_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ImageLoaderThread.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/AsyncLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ImageRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ImageClientInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ImageDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/RawImageDecoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/RequestQueue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ThumbnailCache.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ImageEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ThumbnailBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/PreloadRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/CancelEvent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/VideoImageRescaleRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/VideoThumbnails.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/VideoLengthExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImageManager/ExtractOneVideoFrame.cpp
)

set(libDB_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ImageInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/Category.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/CategoryCollection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ExactCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ImageDate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/MD5Map.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/MemberMap.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ImageInfoList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ImageDB.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/FileInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/NegationCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/NewImageFinder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/NoTagCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/GroupCounter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/CategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ImageSearchInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/CategoryItem.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ContainerCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/ValueCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/OrCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/AndCategoryMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/FastDir.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/FileName.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DB/FileNameList.cpp
)

set(libImportExport_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/Export.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/Import.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/ImportMatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/XMLHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/MiniViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/ImportHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/ImageRow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/ImportDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/ImportSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/KimFileReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ImportExport/MD5CheckPage.cpp
)

set(libAnnotationDialog_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/Dialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ListSelect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ImagePreview.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ImagePreviewWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/KDateEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/CompletableLineEdit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ListViewItemHider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ShowSelectionOnlyManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ShortCutManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ResizableFrame.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/DescriptionEdit.cpp
)

set(libBrowser_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/BrowserWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/BrowserPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/OverviewPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/CategoryPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/ImageViewPage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/TreeFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/Breadcrumb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/BreadcrumbList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/AbstractCategoryModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/FlatCategoryModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/TreeCategoryModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Browser/CenteringIconView.cpp
)

set(libexif_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/Database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/InfoDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/SearchDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/SearchInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/TreeView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/Info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/RangeWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/DatabaseElement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/ReReadDialog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Exif/Grid.cpp
)

set(libBackgroundTaskManager_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/StatusIndicator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobViewer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/CompletedJobInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/PriorityQueue.cpp
)

kde4_add_ui_files(libBackgroundTaskManager_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundTaskManager/JobViewer.ui
)

set(libBackgroundJobs_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundJobs/SearchForVideosWithoutLengthInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundJobs/ReadVideoLengthJob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundJobs/SearchForVideosWithoutVideoThumbnailsJob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundJobs/HandleVideoThumbnailRequestJob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BackgroundJobs/ExtractOneThumbnailJob.cpp
)

set(libRemoteControl_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/RemoteCommand.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/RemoteConnection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/Server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/RemoteInterface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/SearchInfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/RemoteImageRequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/ImageNameStore.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/RemoteControl/ConnectionIndicator.cpp
)

set(libFaceManagement_SRCS)
if(Kface_FOUND)
    set(libFaceManagement_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/FaceManagement/Detector.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/FaceManagement/Recognizer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Settings/FaceManagementPage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/AnnotationDialog/ProposedFaceDialog.cpp
    )
endif(Kface_FOUND)

set(libMap_SRCS)
if(KGEOMAP_FOUND)
    set(libMap_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/Map/MapView.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Map/MapMarkerModelHelper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Map/SearchMarkerTiler.cpp
    )
endif(KGEOMAP_FOUND)

add_subdirectory(images)
add_subdirectory(icons)
add_subdirectory(demo)
add_subdirectory(themes)
add_subdirectory(script)
add_subdirectory(doc)

set(EXIV2_SRCS)
if(EXIV2_FOUND AND QT_QTSQL_FOUND)
    set(EXIV2_SRCS
        #${CMAKE_CURRENT_SOURCE_DIR}/Browser/ExifFolder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Settings/ExifPage.cpp
        ${libexif_SRCS}
    )
endif(EXIV2_FOUND AND QT_QTSQL_FOUND)

########### next target ###############

set(kphotoalbum_SRCS
    main.cpp
    ${libdatebar_SRCS}
    ${libSettings_SRCS}
    ${libsurvey_SRCS}
    ${libxmldb_SRCS}
    ${libThumbnailView_SRCS}
    ${libPlugins_SRCS}
    ${libViewer_SRCS}
    ${libCategoryListView_SRCS}
    ${libHTMLGenerator_SRCS}
    ${libMainWindow_SRCS}
    ${libImageManager_SRCS}
    ${libDB_SRCS}
    ${libImportExport_SRCS}
    ${libAnnotationDialog_SRCS}
    ${EXIV2_SRCS}
    ${libBrowser_SRCS}
    ${libBackgroundTaskManager_SRCS}
    ${libBackgroundJobs_SRCS}
    ${libRemoteControl_SRCS}
    ${libFaceManagement_SRCS}
    ${libMap_SRCS}
)

kde4_add_executable(kphotoalbum ${kphotoalbum_SRCS})

kde4_add_library(Utilities STATIC ${libUtilities_SRCS})

# Internal components
target_link_libraries(kphotoalbum Utilities)

# External components
target_link_libraries(kphotoalbum ${KDE4_KIO_LIBS} ${JPEG_LIBRARY} ${KDE4_TARGET_PREFIX}kmediaplayer ${KDE4_PHONON_LIBS})

if(KIPI_FOUND)
    target_link_libraries(kphotoalbum ${KIPI_LIBRARIES})
endif(KIPI_FOUND)

if(KDCRAW_FOUND)
    target_link_libraries(kphotoalbum ${KDCRAW_LIBRARIES})
endif(KDCRAW_FOUND)

if(EXIV2_FOUND AND QT_QTSQL_FOUND)
    target_link_libraries(kphotoalbum ${EXIV2_LIBRARIES} ${QT_QTSQL_LIBRARY})
endif(EXIV2_FOUND AND QT_QTSQL_FOUND)

if(MARBLE_FOUND)
    target_link_libraries(kphotoalbum ${MARBLE_LIBRARIES})
endif(MARBLE_FOUND)

if(Kface_FOUND)
	target_link_libraries(kphotoalbum ${KFACE_LIBRARIES} )
endif(Kface_FOUND)

if(KGEOMAP_FOUND)
    target_link_libraries(kphotoalbum ${KGEOMAP_LIBRARIES} )
endif()

if (PCRE_FOUND) 
    target_link_libraries(kphotoalbum pcre)
endif()

install(TARGETS kphotoalbum ${INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install(PROGRAMS kphotoalbum.desktop kphotoalbum-import.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES kphotoalbumrc DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES tips DESTINATION ${DATA_INSTALL_DIR}/kphotoalbum)
install(FILES kphotoalbumui.rc default-setup default-layout.xml DESTINATION ${DATA_INSTALL_DIR}/kphotoalbum)

configure_file(config-kpa-kdcraw.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-kdcraw.h)
configure_file(config-kpa-exiv2.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-exiv2.h)
configure_file(config-kpa-kipi.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-kipi.h)
configure_file(config-kpa-marble.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-marble.h)
configure_file(config-kpa-kface.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-kface.h)
configure_file(config-kpa-kgeomap.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kpa-kgeomap.h)
