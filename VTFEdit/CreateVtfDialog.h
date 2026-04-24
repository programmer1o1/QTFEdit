#pragma once

#include <QDialog>
#include <QList>

#include <VTFLib.h>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QScrollArea;
class QSpinBox;
class QDoubleSpinBox;

class CreateVtfDialog final : public QDialog {
    Q_OBJECT

public:
    enum class TextureType {
        Animated = 0,
        EnvironmentMap = 1,
        VolumeTexture = 2,
    };

    explicit CreateVtfDialog(QWidget *parent = nullptr);
    ~CreateVtfDialog() override;

    SVTFCreateOptions options() const;
    TextureType textureType() const;
    bool useAlphaFormat() const;
    VTFImageFormat alphaFormat() const;

private slots:
    void onPresetSelected(int index);
    void onSavePresetAs();
    void onDeletePreset();

private:
    void rebuildPresetCombo(const QString &selectName = {});
    void applyValuesFromPrefix(const QString &prefix);
    void saveValuesToPrefix(const QString &prefix) const;

private:
    QComboBox *textureType_ = nullptr;
    QComboBox *format_ = nullptr;
    QCheckBox *useAlphaFormat_ = nullptr;
    QComboBox *alphaFormat_ = nullptr;
    QLabel *compressionNote_ = nullptr;
    QSpinBox *versionMinor_ = nullptr;
    QCheckBox *mipmaps_ = nullptr;
    QComboBox *mipmapFilter_ = nullptr;
    QCheckBox *thumbnail_ = nullptr;
    QCheckBox *reflectivity_ = nullptr;
    QCheckBox *sphereMap_ = nullptr;
    QCheckBox *srgb_ = nullptr;
    QCheckBox *gammaCorrection_ = nullptr;
    QDoubleSpinBox *gammaValue_ = nullptr;
    QComboBox *resizeMethod_ = nullptr;
    QComboBox *resizeFilter_ = nullptr;
    QCheckBox *resizeClamp_ = nullptr;
    QSpinBox *resizeClampW_ = nullptr;
    QSpinBox *resizeClampH_ = nullptr;
    QSpinBox *alphaThreshold_ = nullptr;
    QGroupBox *flagsGroup_ = nullptr;
    QScrollArea *flagsScroll_ = nullptr;
    QList<QCheckBox *> flagChecks_;
    QDialogButtonBox *buttons_ = nullptr;
    QComboBox *presetCombo_ = nullptr;
};
