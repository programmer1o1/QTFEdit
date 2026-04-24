#include "VtfPropertiesDialog.h"

#include "VtfFlagsDialog.h"
#include "QtUtil.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <utility>

namespace {

QString hexFlags(vlUInt flags) {
    return QString("0x%1").arg(static_cast<qulonglong>(flags), 8, 16, QChar('0')).toUpper();
}

QString formatName(VTFImageFormat format) {
    const SVTFImageFormatInfo *info = vlImageGetImageFormatInfo(format);
    if(info && info->lpName) return QString::fromUtf8(info->lpName);
    return QString("UNKNOWN(%1)").arg(static_cast<int>(format));
}

} // namespace

VtfPropertiesDialog::VtfPropertiesDialog(
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
    QWidget *parent)
    : QDialog(parent),
      filePath_(std::move(filePath)),
      initialFlags_(flags),
      flags_(flags),
      majorVersion_(majorVersion),
      initialMinorVersion_(minorVersion) {
    setWindowTitle("VTF Properties");
    setModal(true);
    resize(520, 520);

    auto *root = new QVBoxLayout(this);

    auto *summary = new QGroupBox("Summary", this);
    auto *summaryLayout = new QFormLayout(summary);
    summaryLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    lblFile_ = new QLabel(filePath_.isEmpty() ? "-" : filePath_, this);
    lblFile_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    summaryLayout->addRow("File:", lblFile_);

    lblDims_ = new QLabel(QString("%1 × %2 × %3").arg(width).arg(height).arg(depth), this);
    summaryLayout->addRow("Dimensions:", lblDims_);

    lblCounts_ = new QLabel(QString("Frames %1, Faces %2, Mipmaps %3").arg(frames).arg(faces).arg(mipmaps), this);
    summaryLayout->addRow("Counts:", lblCounts_);

    lblVersion_ = new QLabel(QString("%1.%2").arg(majorVersion).arg(minorVersion), this);
    summaryLayout->addRow("Version:", lblVersion_);

    lblFormat_ = new QLabel(formatName(format), this);
    summaryLayout->addRow("Format:", lblFormat_);

    if(!mipQualityText.isEmpty()) {
        lblMipQuality_ = new QLabel(mipQualityText, this);
        lblMipQuality_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lblMipQuality_->setWordWrap(true);
        lblMipQuality_->setToolTip("PSNR of each mip vs. the upscaled next-coarser mip. Higher is better.");
        summaryLayout->addRow("Mip quality:", lblMipQuality_);
    }

    root->addWidget(summary);

    auto *header = new QGroupBox("Header", this);
    auto *headerLayout = new QFormLayout(header);

    minorVersion_ = new QComboBox(this);
    for(int v = 0; v <= 5; ++v) {
        minorVersion_->addItem(QString::number(v), v);
    }
    const int idx = minorVersion_->findData(static_cast<int>(initialMinorVersion_));
    if(idx >= 0) minorVersion_->setCurrentIndex(idx);
    connect(minorVersion_, &QComboBox::currentIndexChanged, this, [this] {
        const vlUInt mv = static_cast<vlUInt>(minorVersion_->currentData().toInt());
        if(lblVersion_) lblVersion_->setText(QString("%1.%2").arg(majorVersion_).arg(mv));
    });
    headerLayout->addRow("Minor version:", minorVersion_);

    auto *flagsRow = new QWidget(this);
    auto *flagsRowLayout = new QHBoxLayout(flagsRow);
    flagsRowLayout->setContentsMargins(0, 0, 0, 0);
    lblFlags_ = new QLabel(this);
    lblFlags_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    btnEditFlags_ = new QPushButton("Edit…", this);
    connect(btnEditFlags_, &QPushButton::clicked, this, &VtfPropertiesDialog::editFlags);
    flagsRowLayout->addWidget(lblFlags_, 1);
    flagsRowLayout->addWidget(btnEditFlags_);
    headerLayout->addRow("Flags:", flagsRow);

    startFrame_ = new QSpinBox(this);
    startFrame_->setRange(0, frames > 0 ? static_cast<int>(frames - 1) : 0);
    startFrame_->setValue(static_cast<int>(startFrame));
    headerLayout->addRow("Start frame:", startFrame_);

    bumpScale_ = new QDoubleSpinBox(this);
    bumpScale_->setDecimals(4);
    bumpScale_->setRange(0.0, 1000.0);
    bumpScale_->setValue(static_cast<double>(bumpScale));
    headerLayout->addRow("Bump scale:", bumpScale_);

    computeReflectivity_ = new QCheckBox("Compute from image on apply", this);
    connect(computeReflectivity_, &QCheckBox::toggled, this, &VtfPropertiesDialog::computeReflectivityToggled);
    headerLayout->addRow("Reflectivity:", computeReflectivity_);

    auto *reflRow = new QWidget(this);
    auto *reflLayout = new QHBoxLayout(reflRow);
    reflLayout->setContentsMargins(0, 0, 0, 0);
    reflX_ = new QDoubleSpinBox(this);
    reflY_ = new QDoubleSpinBox(this);
    reflZ_ = new QDoubleSpinBox(this);
    for(auto *spin : {reflX_, reflY_, reflZ_}) {
        spin->setDecimals(6);
        spin->setRange(-1000.0, 1000.0);
        spin->setSingleStep(0.01);
    }
    reflX_->setValue(static_cast<double>(reflectivityX));
    reflY_->setValue(static_cast<double>(reflectivityY));
    reflZ_->setValue(static_cast<double>(reflectivityZ));
    reflLayout->addWidget(new QLabel("X", this));
    reflLayout->addWidget(reflX_);
    reflLayout->addWidget(new QLabel("Y", this));
    reflLayout->addWidget(reflY_);
    reflLayout->addWidget(new QLabel("Z", this));
    reflLayout->addWidget(reflZ_);
    headerLayout->addRow("Manual:", reflRow);

    root->addWidget(header);

    auto *ops = new QGroupBox("Operations", this);
    auto *opsLayout = new QFormLayout(ops);

    genMipmaps_ = new QCheckBox("Generate all mipmaps on apply", this);
    opsLayout->addRow("Mipmaps:", genMipmaps_);

    mipmapFilter_ = new QComboBox(this);
    addMipmapFilters();
    opsLayout->addRow("Mipmap filter:", mipmapFilter_);

    genThumbnail_ = new QCheckBox("Generate thumbnail on apply", this);
    if(!vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
        genThumbnail_->setEnabled(false);
        genThumbnail_->setToolTip("Thumbnail generation requires DXT1 encoding (Compressonator).");
    }
    opsLayout->addRow("Thumbnail:", genThumbnail_);

    thumbSrgb_ = new QCheckBox("Treat as sRGB", this);
    opsLayout->addRow("Thumbnail sRGB:", thumbSrgb_);

    root->addWidget(ops);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons_);

    updateFlagsLabel();
    computeReflectivityToggled(false);
}

VtfPropertiesDialog::~VtfPropertiesDialog() = default;

void VtfPropertiesDialog::addMipmapFilters() {
    mipmapFilter_->addItem("Default", static_cast<int>(MIPMAP_FILTER_DEFAULT));
    mipmapFilter_->addItem("Box", static_cast<int>(MIPMAP_FILTER_BOX));
    mipmapFilter_->addItem("Triangle", static_cast<int>(MIPMAP_FILTER_TRIANGLE));
    mipmapFilter_->addItem("Cubic", static_cast<int>(MIPMAP_FILTER_CUBIC));
    mipmapFilter_->addItem("Catrom", static_cast<int>(MIPMAP_FILTER_CATROM));
    mipmapFilter_->addItem("Mitchell", static_cast<int>(MIPMAP_FILTER_MITCHELL));
    mipmapFilter_->addItem("Point", static_cast<int>(MIPMAP_FILTER_POINT));
    mipmapFilter_->addItem("Gaussian", static_cast<int>(MIPMAP_FILTER_GAUSSIAN));
    mipmapFilter_->addItem("Quadratic", static_cast<int>(MIPMAP_FILTER_QUADRATIC));
    mipmapFilter_->addItem("Sinc", static_cast<int>(MIPMAP_FILTER_SINC));
    mipmapFilter_->addItem("Bessel", static_cast<int>(MIPMAP_FILTER_BESSEL));
    mipmapFilter_->addItem("Hanning", static_cast<int>(MIPMAP_FILTER_HANNING));
    mipmapFilter_->addItem("Hamming", static_cast<int>(MIPMAP_FILTER_HAMMING));
    mipmapFilter_->addItem("Blackman", static_cast<int>(MIPMAP_FILTER_BLACKMAN));
    mipmapFilter_->addItem("Kaiser", static_cast<int>(MIPMAP_FILTER_KAISER));
}

VtfPropertiesDialog::Result VtfPropertiesDialog::result() const {
    Result r;
    r.flags = flags_;
    r.minorVersion = static_cast<vlUInt>(minorVersion_->currentData().toInt());
    r.startFrame = static_cast<vlUInt>(startFrame_->value());
    r.bumpScale = static_cast<vlSingle>(bumpScale_->value());
    r.computeReflectivity = computeReflectivity_->isChecked();
    r.reflectivityX = static_cast<vlSingle>(reflX_->value());
    r.reflectivityY = static_cast<vlSingle>(reflY_->value());
    r.reflectivityZ = static_cast<vlSingle>(reflZ_->value());
    r.generateMipmaps = genMipmaps_->isChecked();
    r.mipmapFilter = static_cast<VTFMipmapFilter>(mipmapFilter_->currentData().toInt());
    r.generateThumbnail = genThumbnail_->isChecked();
    r.thumbnailSrgb = thumbSrgb_->isChecked();
    return r;
}

void VtfPropertiesDialog::editFlags() {
    VtfFlagsDialog dlg(flags_, this);
    if(dlg.exec() != QDialog::Accepted) return;
    flags_ = dlg.flags();
    updateFlagsLabel();
}

void VtfPropertiesDialog::updateFlagsLabel() {
    const vlUInt changed = flags_ ^ initialFlags_;
    lblFlags_->setText(QString("%1 (changed %2)").arg(hexFlags(flags_), hexFlags(changed)));
}

void VtfPropertiesDialog::computeReflectivityToggled(bool checked) {
    for(auto *spin : {reflX_, reflY_, reflZ_}) {
        spin->setEnabled(!checked);
    }
}
