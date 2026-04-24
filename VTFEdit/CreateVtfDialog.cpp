#include "CreateVtfDialog.h"

#include "QtUtil.h"
#include "WindowsVtfFormats.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QStandardItemModel>
#include <QVariant>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

#include <algorithm>

namespace {

static void addMipmapFilters(QComboBox *combo) {
    combo->addItem("Default", static_cast<int>(MIPMAP_FILTER_DEFAULT));
    combo->addItem("Box", static_cast<int>(MIPMAP_FILTER_BOX));
    combo->addItem("Triangle", static_cast<int>(MIPMAP_FILTER_TRIANGLE));
    combo->addItem("Cubic", static_cast<int>(MIPMAP_FILTER_CUBIC));
    combo->addItem("Catrom", static_cast<int>(MIPMAP_FILTER_CATROM));
    combo->addItem("Mitchell", static_cast<int>(MIPMAP_FILTER_MITCHELL));
    combo->addItem("Point", static_cast<int>(MIPMAP_FILTER_POINT));
    combo->addItem("Gaussian", static_cast<int>(MIPMAP_FILTER_GAUSSIAN));
    combo->addItem("Quadratic", static_cast<int>(MIPMAP_FILTER_QUADRATIC));
    combo->addItem("Sinc", static_cast<int>(MIPMAP_FILTER_SINC));
    combo->addItem("Bessel", static_cast<int>(MIPMAP_FILTER_BESSEL));
    combo->addItem("Hanning", static_cast<int>(MIPMAP_FILTER_HANNING));
    combo->addItem("Hamming", static_cast<int>(MIPMAP_FILTER_HAMMING));
    combo->addItem("Blackman", static_cast<int>(MIPMAP_FILTER_BLACKMAN));
    combo->addItem("Kaiser", static_cast<int>(MIPMAP_FILTER_KAISER));
    combo->addItem("Nice (sRGB-aware box)", static_cast<int>(MIPMAP_FILTER_NICE));
}

struct FlagItem {
    const char *name;
    vlUInt mask;
    bool checkedByDefault;
};

constexpr FlagItem kFlags[] = {
    {"Point sample", TEXTUREFLAGS_POINTSAMPLE, false},
    {"Trilinear", TEXTUREFLAGS_TRILINEAR, true},
    {"Anisotropic", TEXTUREFLAGS_ANISOTROPIC, false},
    {"Clamp S", TEXTUREFLAGS_CLAMPS, false},
    {"Clamp T", TEXTUREFLAGS_CLAMPT, false},
    {"Clamp U", TEXTUREFLAGS_CLAMPU, false},
    {"Normal map", TEXTUREFLAGS_NORMAL, false},
    {"No mipmaps (flag)", TEXTUREFLAGS_NOMIP, false},
    {"No LOD", TEXTUREFLAGS_NOLOD, false},
    {"Envmap", TEXTUREFLAGS_ENVMAP, false},
    {"Render target", TEXTUREFLAGS_RENDERTARGET, false},
    {"Border", TEXTUREFLAGS_BORDER, false},
};

} // namespace

CreateVtfDialog::CreateVtfDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Create VTF Options");
    setModal(true);

    auto *layout = new QFormLayout(this);

    // Preset bar
    {
        auto *row = new QWidget(this);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        presetCombo_ = new QComboBox(row);
        presetCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        h->addWidget(presetCombo_, 1);
        auto *saveBtn = new QPushButton("Save As…", row);
        auto *deleteBtn = new QPushButton("Delete", row);
        h->addWidget(saveBtn);
        h->addWidget(deleteBtn);
        layout->addRow("Preset:", row);
        connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CreateVtfDialog::onPresetSelected);
        connect(saveBtn, &QPushButton::clicked, this, &CreateVtfDialog::onSavePresetAs);
        connect(deleteBtn, &QPushButton::clicked, this, &CreateVtfDialog::onDeletePreset);
    }

    textureType_ = new QComboBox(this);
    textureType_->addItem("Animated Texture", static_cast<int>(TextureType::Animated));
    textureType_->addItem("Environment Map", static_cast<int>(TextureType::EnvironmentMap));
    textureType_->addItem("Volume Texture", static_cast<int>(TextureType::VolumeTexture));
    textureType_->setCurrentIndex(0);
    layout->addRow("Texture type:", textureType_);

    format_ = new QComboBox(this);
    for(const auto &item : kWindowsVtfFormats) {
        format_->addItem(QString::fromUtf8(item.label), static_cast<int>(item.format));
    }
    format_->setCurrentIndex(std::max(0, format_->findData(static_cast<int>(IMAGE_FORMAT_RGBA8888))));
    layout->addRow("Format:", format_);

    compressionNote_ = new QLabel(this);
    compressionNote_->setWordWrap(true);
    compressionNote_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    compressionNote_->setStyleSheet("color: palette(mid);");
    layout->addRow("", compressionNote_);

    useAlphaFormat_ = new QCheckBox("Use alpha format when input has transparency", this);
    useAlphaFormat_->setChecked(false);
    connect(useAlphaFormat_, &QCheckBox::toggled, this, [this](bool checked) {
        if(alphaFormat_) alphaFormat_->setEnabled(checked);
    });
    layout->addRow("", useAlphaFormat_);

    alphaFormat_ = new QComboBox(this);
    for(const auto &item : kWindowsVtfFormats) {
        alphaFormat_->addItem(QString::fromUtf8(item.label), static_cast<int>(item.format));
    }
    {
        const auto preferred = vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888;
        const int idx = alphaFormat_->findData(static_cast<int>(preferred));
        alphaFormat_->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    alphaFormat_->setEnabled(false);
    layout->addRow("Alpha format:", alphaFormat_);

    // Disable formats we can’t encode (DXT typically requires a compressor; P8 is not supported).
    if(auto *model = qobject_cast<QStandardItemModel *>(format_->model())) {
        bool dxtDisabled = false;
        for(int i = 0; i < format_->count(); ++i) {
            const auto fmt = static_cast<VTFImageFormat>(format_->itemData(i).toInt());
            const bool canEncode = vtflibCanEncode(fmt);
            if(!canEncode) {
                if(auto *it = model->item(i)) {
                    it->setEnabled(false);
                    if(fmt == IMAGE_FORMAT_DXT1 || fmt == IMAGE_FORMAT_DXT3 || fmt == IMAGE_FORMAT_DXT5 ||
                       fmt == IMAGE_FORMAT_DXT1_ONEBITALPHA) {
                        it->setToolTip("DXT encoding requires a compressor (e.g. Compressonator).");
                        dxtDisabled = true;
                    } else {
                        it->setToolTip("This image format is not supported for creation by this build of VTFLib.");
                    }
                }
            }
        }
        if(dxtDisabled) {
            compressionNote_->setText("DXT compression is unavailable on this build (Compressonator not found). "
                                      "Choose an uncompressed format (RGBA8888/BGRA8888) or install Compressonator and rebuild.");
        } else {
            compressionNote_->setText({});
        }
    }
    if(auto *model = qobject_cast<QStandardItemModel *>(alphaFormat_->model())) {
        for(int i = 0; i < alphaFormat_->count(); ++i) {
            const auto fmt = static_cast<VTFImageFormat>(alphaFormat_->itemData(i).toInt());
            const bool canEncode = vtflibCanEncode(fmt);
            if(!canEncode) {
                if(auto *it = model->item(i)) {
                    it->setEnabled(false);
                    if(fmt == IMAGE_FORMAT_DXT1 || fmt == IMAGE_FORMAT_DXT3 || fmt == IMAGE_FORMAT_DXT5 ||
                       fmt == IMAGE_FORMAT_DXT1_ONEBITALPHA) {
                        it->setToolTip("DXT encoding requires a compressor (e.g. Compressonator).");
                    } else {
                        it->setToolTip("This image format is not supported for creation by this build of VTFLib.");
                    }
                }
            }
        }
    }

    versionMinor_ = new QSpinBox(this);
    versionMinor_->setRange(0, 5);
    versionMinor_->setValue(5);
    layout->addRow("VTF version:", versionMinor_);

    mipmaps_ = new QCheckBox("Generate mipmaps", this);
    mipmaps_->setChecked(true);
    layout->addRow("", mipmaps_);

    mipmapFilter_ = new QComboBox(this);
    addMipmapFilters(mipmapFilter_);
    layout->addRow("Mipmap filter:", mipmapFilter_);

    thumbnail_ = new QCheckBox("Generate thumbnail", this);
    thumbnail_->setChecked(vtflibCanEncode(IMAGE_FORMAT_DXT1));
    if(!vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
        thumbnail_->setEnabled(false);
        thumbnail_->setToolTip("Thumbnail generation requires DXT1 encoding (Compressonator).");
    }
    layout->addRow("", thumbnail_);

    reflectivity_ = new QCheckBox("Compute reflectivity", this);
    reflectivity_->setChecked(true);
    layout->addRow("", reflectivity_);

    sphereMap_ = new QCheckBox("Generate sphere map (cubemap)", this);
    sphereMap_->setChecked(false);
    layout->addRow("", sphereMap_);

    srgb_ = new QCheckBox("sRGB (treat input as sRGB)", this);
    srgb_->setChecked(false);
    layout->addRow("", srgb_);

    gammaCorrection_ = new QCheckBox("Gamma correction", this);
    gammaCorrection_->setChecked(false);
    layout->addRow("", gammaCorrection_);

    gammaValue_ = new QDoubleSpinBox(this);
    gammaValue_->setRange(0.1, 5.0);
    gammaValue_->setSingleStep(0.05);
    gammaValue_->setDecimals(2);
    gammaValue_->setValue(2.2);
    gammaValue_->setEnabled(false);
    connect(gammaCorrection_, &QCheckBox::toggled, this, [this](bool checked) {
        if(gammaValue_) gammaValue_->setEnabled(checked);
    });
    layout->addRow("Gamma:", gammaValue_);

    flagsGroup_ = new QGroupBox("VTF Flags", this);
    auto *flagsLayout = new QVBoxLayout(flagsGroup_);
    flagsLayout->setContentsMargins(8, 8, 8, 8);

    auto *flagsContainer = new QWidget(flagsGroup_);
    auto *flagsContainerLayout = new QVBoxLayout(flagsContainer);
    flagsContainerLayout->setContentsMargins(0, 0, 0, 0);
    flagsContainerLayout->setSpacing(4);

    for(const auto &flag : kFlags) {
        auto *cb = new QCheckBox(QString::fromUtf8(flag.name), flagsContainer);
        cb->setChecked(flag.checkedByDefault);
        cb->setProperty("vtf_flag_mask", QVariant::fromValue<qulonglong>(flag.mask));
        flagChecks_.push_back(cb);
        flagsContainerLayout->addWidget(cb);
    }
    flagsContainerLayout->addStretch(1);

    flagsScroll_ = new QScrollArea(flagsGroup_);
    flagsScroll_->setWidgetResizable(true);
    flagsScroll_->setFrameShape(QFrame::NoFrame);
    flagsScroll_->setWidget(flagsContainer);

    flagsLayout->addWidget(flagsScroll_);
    layout->addRow(flagsGroup_);

    resizeMethod_ = new QComboBox(this);
    resizeMethod_->addItem("No resize", static_cast<int>(RESIZE_COUNT));
    resizeMethod_->addItem("Nearest power of two", static_cast<int>(RESIZE_NEAREST_POWER2));
    resizeMethod_->addItem("Biggest power of two", static_cast<int>(RESIZE_BIGGEST_POWER2));
    resizeMethod_->addItem("Smallest power of two", static_cast<int>(RESIZE_SMALLEST_POWER2));
    resizeMethod_->addItem("Nearest multiple of 4", static_cast<int>(RESIZE_NEAREST_MULTIPLE4));
    resizeMethod_->setCurrentIndex(0);
    layout->addRow("Resize:", resizeMethod_);

    resizeFilter_ = new QComboBox(this);
    addMipmapFilters(resizeFilter_);
    layout->addRow("Resize filter:", resizeFilter_);

    resizeClamp_ = new QCheckBox("Clamp resize maximum dimensions", this);
    resizeClamp_->setChecked(false);
    layout->addRow("", resizeClamp_);

    resizeClampW_ = new QSpinBox(this);
    resizeClampW_->setRange(1, 16384);
    resizeClampW_->setValue(1024);
    resizeClampW_->setEnabled(false);
    layout->addRow("Max width:", resizeClampW_);

    resizeClampH_ = new QSpinBox(this);
    resizeClampH_->setRange(1, 16384);
    resizeClampH_->setValue(1024);
    resizeClampH_->setEnabled(false);
    layout->addRow("Max height:", resizeClampH_);

    connect(resizeClamp_, &QCheckBox::toggled, this, [this](bool checked) {
        if(resizeClampW_) resizeClampW_->setEnabled(checked);
        if(resizeClampH_) resizeClampH_->setEnabled(checked);
    });

    alphaThreshold_ = new QSpinBox(this);
    alphaThreshold_->setRange(0, 255);
    alphaThreshold_->setValue(0);
    layout->addRow("Alpha threshold:", alphaThreshold_);

    // Load global defaults (mirrors Windows "Options" behavior).
    {
        QSettings s;
        const int texType = s.value("options/create/textureType", static_cast<int>(TextureType::Animated)).toInt();
        const int normalFmt = s.value("options/create/normalFormat", static_cast<int>(format_->currentData().toInt())).toInt();
        const int alphaFmt = s.value("options/create/alphaFormat",
                                     static_cast<int>(vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888))
                                 .toInt();
        const bool useAlpha = s.value("options/create/useAlphaFormat", false).toBool();
        const int verMinor = s.value("options/create/versionMinor", 5).toInt();
        const bool mip = s.value("options/create/mipmaps", true).toBool();
        const int mipFilter = s.value("options/create/mipmapFilter", static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt();
        const bool thumb = s.value("options/create/thumbnail", thumbnail_->isChecked()).toBool();
        const bool refl = s.value("options/create/reflectivity", true).toBool();
        const bool sphere = s.value("options/create/sphereMap", false).toBool();
        const bool srgb = s.value("options/create/srgb", false).toBool();
        const bool gammaOn = s.value("options/create/gammaEnabled", false).toBool();
        const double gammaVal = s.value("options/create/gammaValue", 2.2).toDouble();
        const bool resizeEnabled = s.contains("options/create/resizeEnabled") ? s.value("options/create/resizeEnabled").toBool() : true;
        const int resizeMethod = resizeEnabled ? s.value("options/create/resizeMethod", static_cast<int>(RESIZE_NEAREST_POWER2)).toInt()
                                               : static_cast<int>(RESIZE_COUNT);
        const int resizeFilter = s.value("options/create/resizeFilter", mipFilter).toInt();
        const bool clamp = resizeEnabled && s.value("options/create/resizeClamp", false).toBool();
        const int clampW = s.value("options/create/resizeClampWidth", 1024).toInt();
        const int clampH = s.value("options/create/resizeClampHeight", 1024).toInt();
        const int alphaThresh = s.value("options/create/alphaThreshold", 0).toInt();

        textureType_->setCurrentIndex(std::clamp(texType, 0, 2));
        if(const int idx = format_->findData(normalFmt); idx >= 0) format_->setCurrentIndex(idx);
        if(const int idx = alphaFormat_->findData(alphaFmt); idx >= 0) alphaFormat_->setCurrentIndex(idx);
        useAlphaFormat_->setChecked(useAlpha);
        alphaFormat_->setEnabled(useAlpha);

        versionMinor_->setValue(std::clamp(verMinor, 0, 5));
        mipmaps_->setChecked(mip);
        if(const int idx = mipmapFilter_->findData(mipFilter); idx >= 0) mipmapFilter_->setCurrentIndex(idx);
        if(thumbnail_->isEnabled()) thumbnail_->setChecked(thumb);
        reflectivity_->setChecked(refl);
        sphereMap_->setChecked(sphere);
        srgb_->setChecked(srgb);

        gammaCorrection_->setChecked(gammaOn);
        gammaValue_->setValue(std::clamp(gammaVal, 0.1, 5.0));
        gammaValue_->setEnabled(gammaOn);

        if(const int idx = resizeMethod_->findData(resizeMethod); idx >= 0) resizeMethod_->setCurrentIndex(idx);
        if(const int idx = resizeFilter_->findData(resizeFilter); idx >= 0) resizeFilter_->setCurrentIndex(idx);
        resizeClamp_->setChecked(clamp);
        resizeClampW_->setValue(std::clamp(clampW, 1, 16384));
        resizeClampH_->setValue(std::clamp(clampH, 1, 16384));
        resizeClampW_->setEnabled(clamp);
        resizeClampH_->setEnabled(clamp);

        alphaThreshold_->setValue(std::clamp(alphaThresh, 0, 255));
    }

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addRow(buttons_);

    rebuildPresetCombo();
}

void CreateVtfDialog::rebuildPresetCombo(const QString &selectName) {
    if(!presetCombo_) return;
    QSignalBlocker b(presetCombo_);
    presetCombo_->clear();
    presetCombo_->addItem("— none —");
    QSettings s;
    const QStringList names = s.value("presets/export/names").toStringList();
    for(const auto &n : names) presetCombo_->addItem(n);
    if(!selectName.isEmpty()) {
        const int idx = presetCombo_->findText(selectName);
        if(idx >= 0) presetCombo_->setCurrentIndex(idx);
    }
}

void CreateVtfDialog::applyValuesFromPrefix(const QString &prefix) {
    QSettings s;
    auto key = [&](const char *name) { return prefix + QLatin1String(name); };

    const int texType = s.value(key("textureType"), static_cast<int>(TextureType::Animated)).toInt();
    const int normalFmt = s.value(key("normalFormat"), format_->currentData().toInt()).toInt();
    const int alphaFmt = s.value(key("alphaFormat"),
                                 static_cast<int>(vtflibCanEncode(IMAGE_FORMAT_DXT5) ? IMAGE_FORMAT_DXT5 : IMAGE_FORMAT_RGBA8888)).toInt();
    const bool useAlpha = s.value(key("useAlphaFormat"), false).toBool();
    const int verMinor = s.value(key("versionMinor"), 5).toInt();
    const bool mip = s.value(key("mipmaps"), true).toBool();
    const int mipFilter = s.value(key("mipmapFilter"), static_cast<int>(MIPMAP_FILTER_DEFAULT)).toInt();
    const bool thumb = s.value(key("thumbnail"), thumbnail_->isChecked()).toBool();
    const bool refl = s.value(key("reflectivity"), true).toBool();
    const bool sphere = s.value(key("sphereMap"), false).toBool();
    const bool srgb = s.value(key("srgb"), false).toBool();
    const bool gammaOn = s.value(key("gammaEnabled"), false).toBool();
    const double gammaVal = s.value(key("gammaValue"), 2.2).toDouble();
    const bool resizeEnabled = s.contains(key("resizeEnabled")) ? s.value(key("resizeEnabled")).toBool() : true;
    const int resizeMethod = resizeEnabled ? s.value(key("resizeMethod"), static_cast<int>(RESIZE_NEAREST_POWER2)).toInt()
                                           : static_cast<int>(RESIZE_COUNT);
    const int resizeFilter = s.value(key("resizeFilter"), mipFilter).toInt();
    const bool clamp = resizeEnabled && s.value(key("resizeClamp"), false).toBool();
    const int clampW = s.value(key("resizeClampWidth"), 1024).toInt();
    const int clampH = s.value(key("resizeClampHeight"), 1024).toInt();
    const int alphaThresh = s.value(key("alphaThreshold"), 0).toInt();

    textureType_->setCurrentIndex(std::clamp(texType, 0, 2));
    if(const int idx = format_->findData(normalFmt); idx >= 0) format_->setCurrentIndex(idx);
    if(const int idx = alphaFormat_->findData(alphaFmt); idx >= 0) alphaFormat_->setCurrentIndex(idx);
    useAlphaFormat_->setChecked(useAlpha);
    alphaFormat_->setEnabled(useAlpha);
    versionMinor_->setValue(std::clamp(verMinor, 0, 5));
    mipmaps_->setChecked(mip);
    if(const int idx = mipmapFilter_->findData(mipFilter); idx >= 0) mipmapFilter_->setCurrentIndex(idx);
    if(thumbnail_->isEnabled()) thumbnail_->setChecked(thumb);
    reflectivity_->setChecked(refl);
    sphereMap_->setChecked(sphere);
    srgb_->setChecked(srgb);
    gammaCorrection_->setChecked(gammaOn);
    gammaValue_->setValue(std::clamp(gammaVal, 0.1, 5.0));
    gammaValue_->setEnabled(gammaOn);
    if(const int idx = resizeMethod_->findData(resizeMethod); idx >= 0) resizeMethod_->setCurrentIndex(idx);
    if(const int idx = resizeFilter_->findData(resizeFilter); idx >= 0) resizeFilter_->setCurrentIndex(idx);
    resizeClamp_->setChecked(clamp);
    resizeClampW_->setValue(std::clamp(clampW, 1, 16384));
    resizeClampH_->setValue(std::clamp(clampH, 1, 16384));
    resizeClampW_->setEnabled(clamp);
    resizeClampH_->setEnabled(clamp);
    alphaThreshold_->setValue(std::clamp(alphaThresh, 0, 255));

    const QVariant flagsV = s.value(key("flagMask"));
    if(flagsV.isValid()) {
        const vlUInt mask = static_cast<vlUInt>(flagsV.toULongLong());
        for(auto *cb : flagChecks_) {
            const vlUInt m = static_cast<vlUInt>(cb->property("vtf_flag_mask").toULongLong());
            cb->setChecked((mask & m) != 0);
        }
    }
}

void CreateVtfDialog::saveValuesToPrefix(const QString &prefix) const {
    QSettings s;
    auto key = [&](const char *name) { return prefix + QLatin1String(name); };

    s.setValue(key("textureType"), textureType_->currentData().toInt());
    s.setValue(key("normalFormat"), format_->currentData().toInt());
    s.setValue(key("alphaFormat"), alphaFormat_->currentData().toInt());
    s.setValue(key("useAlphaFormat"), useAlphaFormat_->isChecked());
    s.setValue(key("versionMinor"), versionMinor_->value());
    s.setValue(key("mipmaps"), mipmaps_->isChecked());
    s.setValue(key("mipmapFilter"), mipmapFilter_->currentData().toInt());
    s.setValue(key("thumbnail"), thumbnail_->isChecked());
    s.setValue(key("reflectivity"), reflectivity_->isChecked());
    s.setValue(key("sphereMap"), sphereMap_->isChecked());
    s.setValue(key("srgb"), srgb_->isChecked());
    s.setValue(key("gammaEnabled"), gammaCorrection_->isChecked());
    s.setValue(key("gammaValue"), gammaValue_->value());
    const int rm = resizeMethod_->currentData().toInt();
    s.setValue(key("resizeEnabled"), rm != static_cast<int>(RESIZE_COUNT));
    s.setValue(key("resizeMethod"), rm);
    s.setValue(key("resizeFilter"), resizeFilter_->currentData().toInt());
    s.setValue(key("resizeClamp"), resizeClamp_->isChecked());
    s.setValue(key("resizeClampWidth"), resizeClampW_->value());
    s.setValue(key("resizeClampHeight"), resizeClampH_->value());
    s.setValue(key("alphaThreshold"), alphaThreshold_->value());

    vlUInt mask = 0;
    for(const auto *cb : flagChecks_) {
        if(cb->isChecked()) {
            mask |= static_cast<vlUInt>(cb->property("vtf_flag_mask").toULongLong());
        }
    }
    s.setValue(key("flagMask"), static_cast<qulonglong>(mask));
}

void CreateVtfDialog::onPresetSelected(int index) {
    if(index <= 0) return;
    const QString name = presetCombo_->itemText(index);
    if(name.isEmpty()) return;
    applyValuesFromPrefix(QString("presets/export/%1/").arg(name));
}

void CreateVtfDialog::onSavePresetAs() {
    bool ok = false;
    const QString name = QInputDialog::getText(this, "Save Preset",
                                               "Preset name:",
                                               QLineEdit::Normal, {}, &ok).trimmed();
    if(!ok || name.isEmpty()) return;
    if(name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, "Invalid name", "Preset name cannot contain slashes.");
        return;
    }

    saveValuesToPrefix(QString("presets/export/%1/").arg(name));
    QSettings s;
    QStringList names = s.value("presets/export/names").toStringList();
    if(!names.contains(name)) {
        names.push_back(name);
        names.sort(Qt::CaseInsensitive);
        s.setValue("presets/export/names", names);
    }
    rebuildPresetCombo(name);
}

void CreateVtfDialog::onDeletePreset() {
    if(!presetCombo_ || presetCombo_->currentIndex() <= 0) return;
    const QString name = presetCombo_->currentText();
    if(QMessageBox::question(this, "Delete preset",
                             QString("Delete preset \"%1\"?").arg(name)) != QMessageBox::Yes) return;

    QSettings s;
    s.remove(QString("presets/export/%1").arg(name));
    QStringList names = s.value("presets/export/names").toStringList();
    names.removeAll(name);
    s.setValue("presets/export/names", names);
    rebuildPresetCombo();
}

CreateVtfDialog::~CreateVtfDialog() = default;

SVTFCreateOptions CreateVtfDialog::options() const {
    SVTFCreateOptions opts{};
    vlImageCreateDefaultCreateStructure(&opts);

    opts.uiVersion[0] = VTF_MAJOR_VERSION;
    opts.uiVersion[1] = static_cast<vlUInt>(versionMinor_->value());
    opts.ImageFormat = static_cast<VTFImageFormat>(format_->currentData().toInt());
    opts.bMipmaps = mipmaps_->isChecked() ? vlTrue : vlFalse;
    opts.MipmapFilter = static_cast<VTFMipmapFilter>(mipmapFilter_->currentData().toInt());
    opts.bThumbnail = thumbnail_->isChecked() ? vlTrue : vlFalse;
    opts.bReflectivity = reflectivity_->isChecked() ? vlTrue : vlFalse;
    opts.bSRGB = srgb_->isChecked() ? vlTrue : vlFalse;
    opts.nAlphaThreshold = static_cast<vlUInt>(alphaThreshold_->value());
    opts.bGammaCorrection = gammaCorrection_->isChecked() ? vlTrue : vlFalse;
    opts.sGammaCorrection = static_cast<vlSingle>(gammaValue_->value());
    opts.bSphereMap = sphereMap_->isChecked() ? vlTrue : vlFalse;

    opts.uiFlags = 0;
    for(const auto *cb : flagChecks_) {
        if(!cb->isChecked()) continue;
        const vlUInt mask = static_cast<vlUInt>(cb->property("vtf_flag_mask").toULongLong());
        opts.uiFlags |= mask;
    }

    // Keep "sRGB-ness" consistent: bSRGB influences processing, and TEXTUREFLAGS_SRGB is the header flag.
    if(opts.bSRGB) {
        opts.uiFlags |= TEXTUREFLAGS_SRGB;
    } else {
        opts.uiFlags &= ~static_cast<vlUInt>(TEXTUREFLAGS_SRGB);
    }

    const int resizeMethod = resizeMethod_->currentData().toInt();
    if(resizeMethod != static_cast<int>(RESIZE_COUNT)) {
        opts.bResize = vlTrue;
        opts.ResizeMethod = static_cast<VTFResizeMethod>(resizeMethod);
        opts.ResizeFilter = static_cast<VTFMipmapFilter>(resizeFilter_->currentData().toInt());
    } else {
        opts.bResize = vlFalse;
    }

    opts.bResizeClamp = resizeClamp_->isChecked() ? vlTrue : vlFalse;
    opts.uiResizeClampWidth = static_cast<vlUInt>(resizeClampW_->value());
    opts.uiResizeClampHeight = static_cast<vlUInt>(resizeClampH_->value());

    return opts;
}

CreateVtfDialog::TextureType CreateVtfDialog::textureType() const {
    if(!textureType_) return TextureType::Animated;
    const int v = textureType_->currentData().toInt();
    switch(v) {
        case static_cast<int>(TextureType::EnvironmentMap): return TextureType::EnvironmentMap;
        case static_cast<int>(TextureType::VolumeTexture): return TextureType::VolumeTexture;
        default: return TextureType::Animated;
    }
}

bool CreateVtfDialog::useAlphaFormat() const {
    return useAlphaFormat_ && useAlphaFormat_->isChecked();
}

VTFImageFormat CreateVtfDialog::alphaFormat() const {
    if(!alphaFormat_) return IMAGE_FORMAT_DXT5;
    return static_cast<VTFImageFormat>(alphaFormat_->currentData().toInt());
}
