#pragma once

#include <QDialog>

#include <VTFLib.h>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

class VtfPropertiesDialog final : public QDialog {
    Q_OBJECT

public:
    struct Result {
        vlUInt flags = 0;
        vlUInt minorVersion = 0;
        vlUInt startFrame = 0;
        vlSingle bumpScale = 1.0f;
        bool computeReflectivity = false;
        vlSingle reflectivityX = 0.0f;
        vlSingle reflectivityY = 0.0f;
        vlSingle reflectivityZ = 0.0f;

        bool generateMipmaps = false;
        VTFMipmapFilter mipmapFilter = MIPMAP_FILTER_DEFAULT;

        bool generateThumbnail = false;
        bool thumbnailSrgb = false;
    };

    explicit VtfPropertiesDialog(
        QString filePath,
        vlUInt width,
        vlUInt height,
        vlUInt depth,
        vlUInt frames,
        vlUInt faces,
        vlUInt mipmaps,
        vlUInt majorVersion,
        vlUInt minorVersion,
        VTFImageFormat format,
        vlUInt flags,
        vlUInt startFrame,
        vlSingle bumpScale,
        vlSingle reflectivityX,
        vlSingle reflectivityY,
        vlSingle reflectivityZ,
        QString mipQualityText,
        QWidget *parent = nullptr);

    ~VtfPropertiesDialog() override;

    Result result() const;

private slots:
    void editFlags();
    void updateFlagsLabel();
    void computeReflectivityToggled(bool checked);

private:
    void addMipmapFilters();

    QString filePath_;
    vlUInt initialFlags_ = 0;
    vlUInt flags_ = 0;

    QLabel *lblFile_ = nullptr;
    QLabel *lblDims_ = nullptr;
    QLabel *lblCounts_ = nullptr;
    QLabel *lblVersion_ = nullptr;
    QLabel *lblFormat_ = nullptr;
    QLabel *lblMipQuality_ = nullptr;

    QLabel *lblFlags_ = nullptr;
    QPushButton *btnEditFlags_ = nullptr;

    QComboBox *minorVersion_ = nullptr;
    vlUInt majorVersion_ = 7;
    vlUInt initialMinorVersion_ = 0;

    QSpinBox *startFrame_ = nullptr;
    QDoubleSpinBox *bumpScale_ = nullptr;
    QCheckBox *computeReflectivity_ = nullptr;
    QDoubleSpinBox *reflX_ = nullptr;
    QDoubleSpinBox *reflY_ = nullptr;
    QDoubleSpinBox *reflZ_ = nullptr;

    QCheckBox *genMipmaps_ = nullptr;
    QComboBox *mipmapFilter_ = nullptr;

    QCheckBox *genThumbnail_ = nullptr;
    QCheckBox *thumbSrgb_ = nullptr;

    QDialogButtonBox *buttons_ = nullptr;
};
