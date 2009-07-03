#ifndef THUMBNAILPAGE_H
#define THUMBNAILPAGE_H
#include <QWidget>

class QLabel;
class QCheckBox;
class KComboBox;
class QSpinBox;
namespace Settings {
class SettingsData;

class ThumbnailsPage :public QWidget
{
Q_OBJECT

public:
    ThumbnailsPage( QWidget* parent );
    void loadSettings( Settings::SettingsData* );
    void saveSettings( Settings::SettingsData* );

private slots:
    void thumbnailCacheScreenChanged(int);

private:
    QSpinBox* _previewSize;
    QSpinBox* _thumbnailSize;
    KComboBox* _thumbnailAspectRatio;
    QSpinBox* _thumbnailSpace;
    QCheckBox* _thumbnailDarkBackground;
    QCheckBox* _thumbnailDisplayGrid;
    QCheckBox* _displayLabels;
    QCheckBox* _displayCategories;
    QSpinBox* _autoShowThumbnailView;
    QLabel* _thumbnailMegabyteInfo;
    QSpinBox* _thumbnailCacheScreens;
};

}


#endif /* THUMBNAILPAGE_H */
