#include "MainWindow.h"

#include "CreateVtfDialog.h"
#include "CreateVmtDialog.h"
#include "BatchConvertDialog.h"
#include "BatchConvertRunDialog.h"
#include "QtUtil.h"
#include "VtfFlagsDialog.h"
#include "VtfPropertiesDialog.h"
#include "VtfResourcesDialog.h"
#include "VtfSheet.h"
#include "VtfSheetDialog.h"
#include "VmtHighlighter.h"
#include "ResourceHexDialog.h"
#include "OptionsDialog.h"
#include "WadConvertDialog.h"

#include <VTFLib.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QCloseEvent>
#include <QColorDialog>
#include <QContextMenuEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QPainter>
#include <QPalette>
#include <QSettings>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QTreeWidget>
#include <QScrollBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QSlider>
#include <QSignalBlocker>
#include <QScreen>
#include <QScrollArea>
#include <QSizeF>
#include <QStatusBar>
#include <QWheelEvent>
#include <QUrl>
#include <QComboBox>
#include <QDir>
#include <QRegularExpression>
#include <QFile>
#include <QDirIterator>
#include <QtGlobal>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QTextBlock>
#include <QButtonGroup>
#include <QTextCursor>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>

namespace {

QIcon solidColorIcon(const QColor &color, const QSize &size = QSize(14, 14)) {
    QPixmap pm(size);
    pm.fill(color);
    return QIcon(pm);
}

QPointF localPosF(const QWheelEvent *e) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->posF();
#endif
}

QPointF localPosF(const QMouseEvent *e) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

bool labelHasPixmap(const QLabel *label) {
    if(!label) return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return !label->pixmap(Qt::ReturnByValue).isNull();
#else
    const QPixmap *pm = label->pixmap();
    return pm && !pm->isNull();
#endif
}

struct FindReplaceParams {
    QString findText;
    QString replaceText;
    bool caseSensitive = false;
    bool wholeWord = false;
};

bool showFindDialog(QWidget *parent, FindReplaceParams *io, bool withReplace) {
    if(!io) return false;

    QDialog dlg(parent);
    dlg.setWindowTitle(withReplace ? "Replace" : "Find");
    dlg.setModal(true);

    auto *layout = new QFormLayout(&dlg);
    auto *findEdit = new QLineEdit(io->findText, &dlg);
    layout->addRow("Find:", findEdit);

    QLineEdit *replaceEdit = nullptr;
    if(withReplace) {
        replaceEdit = new QLineEdit(io->replaceText, &dlg);
        layout->addRow("Replace with:", replaceEdit);
    }

    auto *caseCb = new QCheckBox("Case sensitive", &dlg);
    caseCb->setChecked(io->caseSensitive);
    layout->addRow("", caseCb);

    auto *wholeCb = new QCheckBox("Whole words", &dlg);
    wholeCb->setChecked(io->wholeWord);
    layout->addRow("", wholeCb);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(buttons);

    findEdit->setFocus();
    findEdit->selectAll();
    if(dlg.exec() != QDialog::Accepted) return false;

    io->findText = findEdit->text();
    if(withReplace && replaceEdit) io->replaceText = replaceEdit->text();
    io->caseSensitive = caseCb->isChecked();
    io->wholeWord = wholeCb->isChecked();
    return true;
}

QString vtfImageFormatName(VTFImageFormat format) {
    switch(format) {
        case IMAGE_FORMAT_RGBA8888: return "RGBA8888";
        case IMAGE_FORMAT_ABGR8888: return "ABGR8888";
        case IMAGE_FORMAT_RGB888: return "RGB888";
        case IMAGE_FORMAT_BGR888: return "BGR888";
        case IMAGE_FORMAT_RGB565: return "RGB565";
        case IMAGE_FORMAT_I8: return "I8";
        case IMAGE_FORMAT_IA88: return "IA88";
        case IMAGE_FORMAT_P8: return "P8";
        case IMAGE_FORMAT_A8: return "A8";
        case IMAGE_FORMAT_RGB888_BLUESCREEN: return "RGB888_BLUESCREEN";
        case IMAGE_FORMAT_BGR888_BLUESCREEN: return "BGR888_BLUESCREEN";
        case IMAGE_FORMAT_ARGB8888: return "ARGB8888";
        case IMAGE_FORMAT_BGRA8888: return "BGRA8888";
        case IMAGE_FORMAT_DXT1: return "DXT1";
        case IMAGE_FORMAT_DXT3: return "DXT3";
        case IMAGE_FORMAT_DXT5: return "DXT5";
        case IMAGE_FORMAT_BGRX8888: return "BGRX8888";
        case IMAGE_FORMAT_BGR565: return "BGR565";
        case IMAGE_FORMAT_BGRX5551: return "BGRX5551";
        case IMAGE_FORMAT_BGRA4444: return "BGRA4444";
        case IMAGE_FORMAT_DXT1_ONEBITALPHA: return "DXT1_ONEBITALPHA";
        case IMAGE_FORMAT_BGRA5551: return "BGRA5551";
        case IMAGE_FORMAT_UV88: return "UV88";
        case IMAGE_FORMAT_UVWQ8888: return "UVWQ8888";
        case IMAGE_FORMAT_RGBA16161616F: return "RGBA16161616F";
        case IMAGE_FORMAT_RGBA16161616: return "RGBA16161616";
        case IMAGE_FORMAT_UVLX8888: return "UVLX8888";
        case IMAGE_FORMAT_R32F: return "R32F";
        case IMAGE_FORMAT_RGB323232F: return "RGB323232F";
        case IMAGE_FORMAT_RGBA32323232F: return "RGBA32323232F";
        case IMAGE_FORMAT_NV_DST16: return "NV_DST16";
        case IMAGE_FORMAT_NV_DST24: return "NV_DST24";
        case IMAGE_FORMAT_NV_INTZ: return "NV_INTZ";
        case IMAGE_FORMAT_NV_RAWZ: return "NV_RAWZ";
        case IMAGE_FORMAT_ATI_DST16: return "ATI_DST16";
        case IMAGE_FORMAT_ATI_DST24: return "ATI_DST24";
        case IMAGE_FORMAT_NV_NULL: return "NV_NULL";
        case IMAGE_FORMAT_ATI2N: return "ATI2N";
        case IMAGE_FORMAT_ATI1N: return "ATI1N";
        case IMAGE_FORMAT_HDR_BGRA8888: return "HDR_BGRA8888";
        case IMAGE_FORMAT_NONE: return "NONE";
        default: return QString("UNKNOWN(%1)").arg(static_cast<int>(format));
    }
}

QImage vtfSelectionToRgba(unsigned int frame, unsigned int face, unsigned int slice, unsigned int mip, QString *error) {
    const vlUInt baseW = vlImageGetWidth();
    const vlUInt baseH = vlImageGetHeight();
    const vlUInt baseD = vlImageGetDepth();
    vlUInt width = 0, height = 0, depth = 0;
    vlImageComputeMipmapDimensions(baseW, baseH, baseD, mip, &width, &height, &depth);
    (void)depth;
    const auto iwidth = static_cast<int>(width);
    const auto iheight = static_cast<int>(height);
    if(iwidth <= 0 || iheight <= 0) {
        if(error) *error = "VTF has invalid dimensions.";
        return {};
    }

    const auto format = vlImageGetFormat();
    vlByte *src = vlImageGetData(frame, face, slice, mip);
    if(!src) {
        if(error) {
            *error = QString("VTF contains no image data (frame %1, face %2, slice %3, mip %4).")
                         .arg(frame)
                         .arg(face)
                         .arg(slice)
                         .arg(mip);
        }
        return {};
    }

    std::vector<vlByte> rgba(static_cast<size_t>(iwidth) * static_cast<size_t>(iheight) * 4);
    if(!vlImageConvertToRGBA8888(src, rgba.data(), static_cast<vlUInt>(iwidth), static_cast<vlUInt>(iheight), format)) {
        if(error) *error = QString("Failed to convert VTF data to RGBA8888: %1").arg(vlGetLastError());
        return {};
    }

    QImage image(reinterpret_cast<const uchar *>(rgba.data()), iwidth, iheight, QImage::Format_RGBA8888);
    return image.copy();
}

bool vtfFormatIsHdr(VTFImageFormat fmt) {
    switch(fmt) {
        case IMAGE_FORMAT_RGBA16161616F:
        case IMAGE_FORMAT_HDR_BGRA8888:
        case IMAGE_FORMAT_R32F:
        case IMAGE_FORMAT_RGB323232F:
        case IMAGE_FORMAT_RGBA32323232F:
            return true;
        default:
            return false;
    }
}

static inline float halfToFloat(std::uint16_t h) {
    const std::uint32_t sign = (static_cast<std::uint32_t>(h) & 0x8000u) << 16u;
    std::uint32_t exp = (static_cast<std::uint32_t>(h) & 0x7C00u) >> 10u;
    std::uint32_t mant = (static_cast<std::uint32_t>(h) & 0x03FFu);

    std::uint32_t fbits = 0;
    if(exp == 0) {
        if(mant == 0) {
            fbits = sign;
        } else {
            exp = 127u - 15u + 1u;
            while((mant & 0x0400u) == 0) {
                mant <<= 1u;
                exp -= 1u;
            }
            mant &= 0x03FFu;
            fbits = sign | (exp << 23u) | (mant << 13u);
        }
    } else if(exp == 0x1Fu) {
        fbits = sign | 0x7F800000u | (mant << 13u);
    } else {
        exp = exp + (127u - 15u);
        fbits = sign | (exp << 23u) | (mant << 13u);
    }

    float out = 0.0f;
    std::memcpy(&out, &fbits, sizeof(out));
    return out;
}

static inline float tonemapOne(float x, MainWindow::HdrTonemap algo) {
    if(!(x > 0.0f)) return 0.0f;
    switch(algo) {
        case MainWindow::HdrTonemap::Clamp:
            return x > 1.0f ? 1.0f : x;
        case MainWindow::HdrTonemap::VTFLib:
        case MainWindow::HdrTonemap::Reinhard:
            return x / (1.0f + x);
        case MainWindow::HdrTonemap::ACES: {
            const float a = 2.51f;
            const float b = 0.03f;
            const float c = 2.43f;
            const float d = 0.59f;
            const float e = 0.14f;
            const float y = (x * (a * x + b)) / (x * (c * x + d) + e);
            if(y < 0.0f) return 0.0f;
            if(y > 1.0f) return 1.0f;
            return y;
        }
        default:
            return x / (1.0f + x);
    }
}

static inline float linearToSrgb(float x) {
    if(x <= 0.0f) return 0.0f;
    if(x >= 1.0f) return 1.0f;
    if(x <= 0.0031308f) return x * 12.92f;
    return 1.055f * std::pow(x, 1.0f / 2.4f) - 0.055f;
}

static inline std::uint8_t toU8(float x) {
    if(x <= 0.0f) return 0;
    if(x >= 1.0f) return 255;
    return static_cast<std::uint8_t>(std::lround(x * 255.0f));
}

QImage vtfSelectionToHdrTonemappedRgba(
    unsigned int frame,
    unsigned int face,
    unsigned int slice,
    unsigned int mip,
    double exposure,
    MainWindow::HdrTonemap tonemap,
    QString *error
) {
    const vlUInt baseW = vlImageGetWidth();
    const vlUInt baseH = vlImageGetHeight();
    const vlUInt baseD = vlImageGetDepth();
    vlUInt width = 0, height = 0, depth = 0;
    vlImageComputeMipmapDimensions(baseW, baseH, baseD, mip, &width, &height, &depth);
    (void)depth;
    const int w = static_cast<int>(width);
    const int h = static_cast<int>(height);
    if(w <= 0 || h <= 0) {
        if(error) *error = "VTF has invalid dimensions.";
        return {};
    }

    const auto format = vlImageGetFormat();
    vlByte *src = vlImageGetData(frame, face, slice, mip);
    if(!src) {
        if(error) {
            *error = QString("VTF contains no image data (frame %1, face %2, slice %3, mip %4).")
                         .arg(frame)
                         .arg(face)
                         .arg(slice)
                         .arg(mip);
        }
        return {};
    }

    const float exposureMul = static_cast<float>(exposure);
    QImage out(w, h, QImage::Format_RGBA8888);
    if(out.isNull()) {
        if(error) *error = "Failed to allocate preview image.";
        return {};
    }

    auto writePixel = [&](int x, int y, float r, float g, float b, float a) {
        if(!std::isfinite(r)) r = 0.0f;
        if(!std::isfinite(g)) g = 0.0f;
        if(!std::isfinite(b)) b = 0.0f;
        if(!std::isfinite(a)) a = 1.0f;

        r = tonemapOne(r * exposureMul, tonemap);
        g = tonemapOne(g * exposureMul, tonemap);
        b = tonemapOne(b * exposureMul, tonemap);
        a = std::clamp(a, 0.0f, 1.0f);

        const std::uint8_t R = toU8(linearToSrgb(r));
        const std::uint8_t G = toU8(linearToSrgb(g));
        const std::uint8_t B = toU8(linearToSrgb(b));
        const std::uint8_t A = toU8(a);

        auto *row = reinterpret_cast<std::uint8_t *>(out.scanLine(y));
        const int idx = x * 4;
        row[idx + 0] = R;
        row[idx + 1] = G;
        row[idx + 2] = B;
        row[idx + 3] = A;
    };

    switch(format) {
        case IMAGE_FORMAT_RGBA16161616F: {
            const auto *p = reinterpret_cast<const std::uint8_t *>(src);
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    const std::uint16_t hr = static_cast<std::uint16_t>(p[0] | (p[1] << 8));
                    const std::uint16_t hg = static_cast<std::uint16_t>(p[2] | (p[3] << 8));
                    const std::uint16_t hb = static_cast<std::uint16_t>(p[4] | (p[5] << 8));
                    const std::uint16_t ha = static_cast<std::uint16_t>(p[6] | (p[7] << 8));
                    p += 8;
                    writePixel(x, y, halfToFloat(hr), halfToFloat(hg), halfToFloat(hb), halfToFloat(ha));
                }
            }
            break;
        }
        case IMAGE_FORMAT_HDR_BGRA8888: {
            const auto *p = reinterpret_cast<const std::uint8_t *>(src);
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    const float b = static_cast<float>(p[0]);
                    const float g = static_cast<float>(p[1]);
                    const float r = static_cast<float>(p[2]);
                    const float e = static_cast<float>(p[3]);
                    p += 4;

                    const float mul = (e * 16.0f) / 512.0f;
                    writePixel(x, y, r * mul, g * mul, b * mul, 1.0f);
                }
            }
            break;
        }
        case IMAGE_FORMAT_R32F: {
            const auto *p = reinterpret_cast<const std::uint8_t *>(src);
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    float v = 0.0f;
                    std::memcpy(&v, p, sizeof(float));
                    p += 4;
                    writePixel(x, y, v, v, v, 1.0f);
                }
            }
            break;
        }
        case IMAGE_FORMAT_RGB323232F: {
            const auto *p = reinterpret_cast<const std::uint8_t *>(src);
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    float r = 0.0f, g = 0.0f, b = 0.0f;
                    std::memcpy(&r, p + 0, sizeof(float));
                    std::memcpy(&g, p + 4, sizeof(float));
                    std::memcpy(&b, p + 8, sizeof(float));
                    p += 12;
                    writePixel(x, y, r, g, b, 1.0f);
                }
            }
            break;
        }
        case IMAGE_FORMAT_RGBA32323232F: {
            const auto *p = reinterpret_cast<const std::uint8_t *>(src);
            for(int y = 0; y < h; ++y) {
                for(int x = 0; x < w; ++x) {
                    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
                    std::memcpy(&r, p + 0, sizeof(float));
                    std::memcpy(&g, p + 4, sizeof(float));
                    std::memcpy(&b, p + 8, sizeof(float));
                    std::memcpy(&a, p + 12, sizeof(float));
                    p += 16;
                    writePixel(x, y, r, g, b, a);
                }
            }
            break;
        }
        default:
            if(error) *error = "This format is not supported by HDR preview.";
            return {};
    }

    return out;
}

QImage vtfThumbnailToRgba(QString *error) {
    const auto width = static_cast<int>(vlImageGetThumbnailWidth());
    const auto height = static_cast<int>(vlImageGetThumbnailHeight());
    if(!vlImageGetHasThumbnail() || width <= 0 || height <= 0) {
        if(error) *error = "VTF contains no thumbnail.";
        return {};
    }

    const auto format = vlImageGetThumbnailFormat();
    vlByte *src = vlImageGetThumbnailData();
    if(!src) {
        if(error) *error = "VTF thumbnail data is missing.";
        return {};
    }

    std::vector<vlByte> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    if(!vlImageConvertToRGBA8888(src, rgba.data(), width, height, format)) {
        if(error) *error = QString("Failed to convert VTF thumbnail to RGBA8888: %1").arg(vlGetLastError());
        return {};
    }

    QImage image(reinterpret_cast<const uchar *>(rgba.data()), width, height, QImage::Format_RGBA8888);
    return image.copy();
}

QImage makeCheckerBackground(const QSize &size, const QColor &a, const QColor &b, int step) {
    QImage bg(size, QImage::Format_RGB32);
    bg.fill(a);
    QPainter p(&bg);
    p.setPen(Qt::NoPen);
    p.setBrush(b);
    for(int y = 0; y < size.height(); y += step) {
        for(int x = 0; x < size.width(); x += step) {
            if(((x / step) + (y / step)) % 2 == 0) continue;
            p.drawRect(x, y, step, step);
        }
    }
    return bg;
}

QImage compositeOverBackground(const QImage &rgba, MainWindow::BackgroundMode mode, const QColor &customColor) {
    if(rgba.isNull()) return {};

    QImage bg;
    switch(mode) {
        case MainWindow::BackgroundMode::Black:
            bg = QImage(rgba.size(), QImage::Format_RGB32);
            bg.fill(Qt::black);
            break;
        case MainWindow::BackgroundMode::White:
            bg = QImage(rgba.size(), QImage::Format_RGB32);
            bg.fill(Qt::white);
            break;
        case MainWindow::BackgroundMode::Custom: {
            bg = QImage(rgba.size(), QImage::Format_RGB32);
            bg.fill(customColor.isValid() ? customColor : Qt::white);
            break;
        }
        case MainWindow::BackgroundMode::Checker:
        default:
            bg = makeCheckerBackground(rgba.size(), QColor(205, 205, 205), QColor(145, 145, 145), 12);
            break;
    }

    QImage out = bg;
    QPainter p(&out);
    p.drawImage(0, 0, rgba);
    return out;
}

QImage applyExposure8bit(const QImage &rgba8888, double stops) {
    if(rgba8888.isNull()) return {};
    if(std::abs(stops) < 1e-12) return rgba8888;
    QImage src = rgba8888;
    if(src.format() != QImage::Format_RGBA8888 && src.format() != QImage::Format_ARGB32 && src.format() != QImage::Format_ARGB32_Premultiplied) {
        src = src.convertToFormat(QImage::Format_RGBA8888);
    }
    QImage out = src.convertToFormat(QImage::Format_RGBA8888);
    const double factor = std::pow(2.0, stops);

    for(int y = 0; y < out.height(); ++y) {
        auto *row = reinterpret_cast<uchar *>(out.scanLine(y));
        for(int x = 0; x < out.width(); ++x) {
            const int i = x * 4;
            const int r = static_cast<int>(std::lround(row[i + 0] * factor));
            const int g = static_cast<int>(std::lround(row[i + 1] * factor));
            const int b = static_cast<int>(std::lround(row[i + 2] * factor));
            row[i + 0] = static_cast<uchar>(std::clamp(r, 0, 255));
            row[i + 1] = static_cast<uchar>(std::clamp(g, 0, 255));
            row[i + 2] = static_cast<uchar>(std::clamp(b, 0, 255));
        }
    }
    return out;
}

QImage channelView(const QImage &rgba, MainWindow::ChannelMode mode) {
    if(rgba.isNull()) return {};
    QImage src = rgba;
    if(src.format() != QImage::Format_ARGB32) {
        src = src.convertToFormat(QImage::Format_ARGB32);
    }

    if(mode == MainWindow::ChannelMode::RGBA || mode == MainWindow::ChannelMode::RGB) {
        return src;
    }

    QImage out(src.size(), QImage::Format_RGB32);
    const int w = src.width();
    const int h = src.height();
    for(int y = 0; y < h; ++y) {
        const auto *srcRow = reinterpret_cast<const QRgb *>(src.constScanLine(y));
        auto *dst = reinterpret_cast<QRgb *>(out.scanLine(y));
        for(int x = 0; x < w; ++x) {
            const int r = qRed(srcRow[x]);
            const int g = qGreen(srcRow[x]);
            const int b = qBlue(srcRow[x]);
            const int a = qAlpha(srcRow[x]);
            int v = 0;
            switch(mode) {
                case MainWindow::ChannelMode::R: v = r; break;
                case MainWindow::ChannelMode::G: v = g; break;
                case MainWindow::ChannelMode::B: v = b; break;
                case MainWindow::ChannelMode::A: v = a; break;
                default: v = 0; break;
            }
            dst[x] = qRgb(v, v, v);
        }
    }
    return out;
}

bool validateVmtText(const QString &text, VMTParseMode mode, QString *error) {
    const vlInt prev = vlGetInteger(VTFLIB_VMT_PARSE_MODE);
    vlSetInteger(VTFLIB_VMT_PARSE_MODE, static_cast<vlInt>(mode));
    VTFLib::CVMTFile vmt;
    const QByteArray bytes = text.toUtf8();
    if(!vmt.Load(bytes.constData(), static_cast<vlUInt>(bytes.size()))) {
        if(error) *error = QString::fromUtf8(vlGetLastError());
        vlSetInteger(VTFLIB_VMT_PARSE_MODE, prev);
        return false;
    }
    vlSetInteger(VTFLIB_VMT_PARSE_MODE, prev);
    return true;
}

QString baseTextureFromVtfPathHeuristic(const QString &vtfPath) {
    if(vtfPath.isEmpty()) return {};
    const QString p = QDir::fromNativeSeparators(vtfPath);
    const QString needle = "/materials/";
    const int idx = p.lastIndexOf(needle, -1, Qt::CaseInsensitive);
    if(idx < 0) return {};
    QString rel = p.mid(idx + needle.size());
    if(rel.endsWith(".vtf", Qt::CaseInsensitive)) rel.chop(4);
    return rel;
}

QString baseTextureFromVmtTextHeuristic(QString text) {
    // Strip // comments to reduce false positives.
    const QRegularExpression lineCommentRe("//.*$", QRegularExpression::MultilineOption);
    text.replace(lineCommentRe, "");

    const QRegularExpression re("\"\\$?basetexture\"\\s+\"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(text);
    if(!m.hasMatch()) return {};
    QString v = m.captured(1).trimmed();
    v.replace('\\', '/');
    while(v.startsWith('/')) v.remove(0, 1);
    if(v.startsWith("materials/", Qt::CaseInsensitive)) v = v.mid(QString("materials/").size());
    if(v.endsWith(".vtf", Qt::CaseInsensitive)) v.chop(4);
    return v;
}

QString resourceTypeNameForUi(vlUInt type) {
    switch(type) {
        case VTF_LEGACY_RSRC_LOW_RES_IMAGE: return "Thumbnail Image";
        case VTF_LEGACY_RSRC_IMAGE: return "Image";
        case VTF_RSRC_SHEET: return "Sheet";
        case VTF_RSRC_CRC: return "Cyclic Redundancy Check";
        case VTF_RSRC_TEXTURE_LOD_SETTINGS: return "LOD Settings";
        case VTF_RSRC_TEXTURE_SETTINGS_EX: return "Extended Texture Settings";
        case VTF_RSRC_KEY_VALUE_DATA: return "Key/Value Data";
        default: return "Unknown";
    }
}

QString cubeFaceName(unsigned int face) {
    switch(face) {
        case CUBEMAP_FACE_RIGHT: return "+X Right";
        case CUBEMAP_FACE_LEFT: return "-X Left";
        case CUBEMAP_FACE_BACK: return "+Y Back";
        case CUBEMAP_FACE_FRONT: return "-Y Front";
        case CUBEMAP_FACE_UP: return "+Z Up";
        case CUBEMAP_FACE_DOWN: return "-Z Down";
        case CUBEMAP_FACE_SPHERE_MAP: return "Sphere Map";
        default: return {};
    }
}

QString cubeFaceFileTag(unsigned int face) {
    switch(face) {
        case CUBEMAP_FACE_RIGHT: return "posx";
        case CUBEMAP_FACE_LEFT: return "negx";
        case CUBEMAP_FACE_BACK: return "posy";
        case CUBEMAP_FACE_FRONT: return "negy";
        case CUBEMAP_FACE_UP: return "posz";
        case CUBEMAP_FACE_DOWN: return "negz";
        case CUBEMAP_FACE_SPHERE_MAP: return "spheremap";
        default: return QString("face%1").arg(face);
    }
}

int leadingSpacesCount(const QString &s) {
    int n = 0;
    while(n < s.size() && s.at(n) == QChar(' ')) ++n;
    return n;
}

bool imageHasTransparencyRgba8888(const QImage &rgba8888) {
    if(rgba8888.isNull()) return false;
    if(rgba8888.format() != QImage::Format_RGBA8888) return false;
    for(int y = 0; y < rgba8888.height(); ++y) {
        const uchar *row = rgba8888.constScanLine(y);
        const int w = rgba8888.width();
        for(int x = 0; x < w; ++x) {
            if(row[x * 4 + 3] != 255) return true;
        }
    }
    return false;
}

QString quotedKvdValue(QString s) {
    s.replace('\\', "\\\\");
    s.replace('"', "\\\"");
    s.replace('\r', "");
    s.replace('\n', "\\n");
    return QString("\"%1\"").arg(s);
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setAcceptDrops(true);
    setWindowTitle("VTFEdit (Qt)");

    {
        QSettings s;
        customBackgroundColor_ = QColor(s.value("view/customBackgroundColor", "#ffffff").toString());
        if(!customBackgroundColor_.isValid()) customBackgroundColor_ = Qt::white;
    }
    {
        QSettings s;
        vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_R, static_cast<vlSingle>(s.value("options/luminance/r", 0.299).toDouble()));
        vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_G, static_cast<vlSingle>(s.value("options/luminance/g", 0.587).toDouble()));
        vlSetFloat(VTFLIB_LUMINANCE_WEIGHT_B, static_cast<vlSingle>(s.value("options/luminance/b", 0.114).toDouble()));
    }

    actionOpen_ = new QAction("&Open…", this);
    actionOpen_->setShortcut(QKeySequence::Open);
    connect(actionOpen_, &QAction::triggered, this, &MainWindow::openFileDialog);

    actionOpenVmt_ = new QAction("Open &VMT…", this);
    actionOpenVmt_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    connect(actionOpenVmt_, &QAction::triggered, this, &MainWindow::openVmtDialog);

    actionReloadVtf_ = new QAction("&Reload VTF", this);
    actionReloadVtf_->setShortcut(QKeySequence(Qt::Key_F5));
    connect(actionReloadVtf_, &QAction::triggered, this, &MainWindow::reloadVtf);
    actionReloadVtf_->setEnabled(false);

    actionOpenContainingFolder_ = new QAction("Open Containing &Folder", this);
    connect(actionOpenContainingFolder_, &QAction::triggered, this, &MainWindow::openContainingFolder);
    actionOpenContainingFolder_->setEnabled(false);

    actionCloseVtf_ = new QAction("&Close VTF", this);
    actionCloseVtf_->setShortcut(QKeySequence::Close);
    connect(actionCloseVtf_, &QAction::triggered, this, &MainWindow::closeCurrentVtf);
    actionCloseVtf_->setEnabled(false);

    actionCloseVmt_ = new QAction("Close &VMT", this);
    connect(actionCloseVmt_, &QAction::triggered, this, &MainWindow::closeCurrentVmt);
    actionCloseVmt_->setEnabled(false);

    actionOpenMatchingVmt_ = new QAction("Open Matching &VMT", this);
    actionOpenMatchingVmt_->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M));
    connect(actionOpenMatchingVmt_, &QAction::triggered, this, &MainWindow::openMatchingVmtFromVtf);
    actionOpenMatchingVmt_->setEnabled(false);

    actionOpenLinkedVtf_ = new QAction("Open Linked &VTF", this);
    actionOpenLinkedVtf_->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T));
    connect(actionOpenLinkedVtf_, &QAction::triggered, this, &MainWindow::openLinkedVtfFromVmt);
    actionOpenLinkedVtf_->setEnabled(false);

    actionVtfProperties_ = new QAction("&Properties…", this);
    actionVtfProperties_->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
    connect(actionVtfProperties_, &QAction::triggered, this, &MainWindow::vtfPropertiesDialog);
    actionVtfProperties_->setEnabled(false);

    actionVtfResources_ = new QAction("&Resources…", this);
    connect(actionVtfResources_, &QAction::triggered, this, &MainWindow::vtfResourcesDialog);
    actionVtfResources_->setEnabled(false);

    actionEditVtfFlags_ = new QAction("Edit &Flags…", this);
    connect(actionEditVtfFlags_, &QAction::triggered, this, &MainWindow::editVtfFlagsDialog);
    actionEditVtfFlags_->setEnabled(false);

    actionSaveVtf_ = new QAction("&Save VTF", this);
    connect(actionSaveVtf_, &QAction::triggered, this, &MainWindow::saveVtf);
    actionSaveVtf_->setEnabled(false);

    actionImportImage_ = new QAction("&Import Image…", this);
    actionImportImage_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(actionImportImage_, &QAction::triggered, this, &MainWindow::importImageDialog);

    actionSaveAsVtf_ = new QAction("Save &As VTF…", this);
    actionSaveAsVtf_->setShortcut(QKeySequence::SaveAs);
    connect(actionSaveAsVtf_, &QAction::triggered, this, &MainWindow::saveAsVtfDialog);
    actionSaveAsVtf_->setEnabled(false);

    actionExportPng_ = new QAction("E&xport PNG…", this);
    actionExportPng_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_E));
    connect(actionExportPng_, &QAction::triggered, this, &MainWindow::exportPngDialog);
    actionExportPng_->setEnabled(false);

    actionExportThumbnailPng_ = new QAction("Export &Thumbnail PNG…", this);
    connect(actionExportThumbnailPng_, &QAction::triggered, this, &MainWindow::exportThumbnailPngDialog);
    actionExportThumbnailPng_->setEnabled(false);

    actionExportAs_ = new QAction("Export &As…", this);
    connect(actionExportAs_, &QAction::triggered, this, &MainWindow::exportImageAsDialog);
    actionExportAs_->setEnabled(false);

    actionCopyImage_ = new QAction("&Copy Image", this);
    actionCopyImage_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(actionCopyImage_, &QAction::triggered, this, &MainWindow::copyViewToClipboard);
    actionCopyImage_->setEnabled(false);

    actionPasteImage_ = new QAction("&Paste Image as New VTF…", this);
    actionPasteImage_->setToolTip("Create a new VTF from an image currently in the clipboard.");
    connect(actionPasteImage_, &QAction::triggered, this, &MainWindow::pasteImageFromClipboard);
    actionPasteImage_->setEnabled(false);
    {
        auto updatePasteEnabled = [this] {
            if(!actionPasteImage_) return;
            const QImage img = QGuiApplication::clipboard()->image();
            actionPasteImage_->setEnabled(!img.isNull());
        };
        connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, updatePasteEnabled);
        updatePasteEnabled();
    }

    actionExportAllFrames_ = new QAction("Export &All Frames…", this);
    connect(actionExportAllFrames_, &QAction::triggered, this, &MainWindow::exportAllFramesDialog);
    actionExportAllFrames_->setEnabled(false);

    actionExportAllFaces_ = new QAction("Export All &Faces…", this);
    actionExportAllFaces_->setToolTip("Export all faces at the current Frame/Slice/Mip.");
    connect(actionExportAllFaces_, &QAction::triggered, this, &MainWindow::exportAllFacesDialog);
    actionExportAllFaces_->setEnabled(false);

    actionExportAllSlices_ = new QAction("Export All S&lices…", this);
    actionExportAllSlices_->setToolTip("Export all slices at the current Frame/Face/Mip.");
    connect(actionExportAllSlices_, &QAction::triggered, this, &MainWindow::exportAllSlicesDialog);
    actionExportAllSlices_->setEnabled(false);

    actionExportAllMipmaps_ = new QAction("Export All &Mipmaps…", this);
    actionExportAllMipmaps_->setToolTip("Export all mipmaps at the current Frame/Face/Slice.");
    connect(actionExportAllMipmaps_, &QAction::triggered, this, &MainWindow::exportAllMipmapsDialog);
    actionExportAllMipmaps_->setEnabled(false);

    actionExportAll_ = new QAction("Export &All (Frames×Faces×Slices)…", this);
    actionExportAll_->setToolTip("Export all frames, faces and slices at a selected mipmap level.");
    connect(actionExportAll_, &QAction::triggered, this, &MainWindow::exportAllDialog);
    actionExportAll_->setEnabled(false);

    actionBatchConvert_ = new QAction("&Batch Convert Folder…", this);
    connect(actionBatchConvert_, &QAction::triggered, this, &MainWindow::batchConvertDialog);

    actionWadConvert_ = new QAction("WAD Con&vert…", this);
    connect(actionWadConvert_, &QAction::triggered, this, &MainWindow::wadConvertDialog);

    actionGenNormalFrame_ = new QAction("Generate Normal Map (Current Frame)…", this);
    connect(actionGenNormalFrame_, &QAction::triggered, this, &MainWindow::generateNormalMapCurrentFrame);
    actionGenNormalFrame_->setEnabled(false);

    actionGenNormalAll_ = new QAction("Generate Normal Map (All Frames)…", this);
    connect(actionGenNormalAll_, &QAction::triggered, this, &MainWindow::generateNormalMapsAllFrames);
    actionGenNormalAll_->setEnabled(false);

    actionGenSphereMap_ = new QAction("Generate Sphere Map (Cubemap)…", this);
    connect(actionGenSphereMap_, &QAction::triggered, this, &MainWindow::generateSphereMap);
    actionGenSphereMap_->setEnabled(false);

    actionQuit_ = new QAction("&Quit", this);
    actionQuit_->setShortcut(QKeySequence::Quit);
    connect(actionQuit_, &QAction::triggered, this, &QWidget::close);

    actionZoomIn_ = new QAction("Zoom &In", this);
    actionZoomIn_->setShortcut(QKeySequence::ZoomIn);
    actionZoomIn_->setToolTip("Zoom in (Ctrl+Plus). Tip: Ctrl+Mouse Wheel also zooms.");
    connect(actionZoomIn_, &QAction::triggered, this, &MainWindow::zoomIn);
    actionZoomIn_->setEnabled(false);

    actionZoomOut_ = new QAction("Zoom &Out", this);
    actionZoomOut_->setShortcut(QKeySequence::ZoomOut);
    actionZoomOut_->setToolTip("Zoom out (Ctrl+Minus). Tip: Ctrl+Mouse Wheel also zooms.");
    connect(actionZoomOut_, &QAction::triggered, this, &MainWindow::zoomOut);
    actionZoomOut_->setEnabled(false);

    actionZoomReset_ = new QAction("&Reset Zoom", this);
    actionZoomReset_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    actionZoomReset_->setToolTip("Reset zoom to 100% (Ctrl+0).");
    connect(actionZoomReset_, &QAction::triggered, this, &MainWindow::zoomReset);
    actionZoomReset_->setEnabled(false);

    actionZoomFit_ = new QAction("&Fit to Window", this);
    actionZoomFit_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_F));
    actionZoomFit_->setCheckable(true);
    actionZoomFit_->setToolTip("Fit image to window (Ctrl+Shift+F).");
    connect(actionZoomFit_, &QAction::triggered, this, &MainWindow::zoomFit);
    actionZoomFit_->setEnabled(false);

    actionPlayPause_ = new QAction("&Play", this);
    actionPlayPause_->setShortcut(QKeySequence(Qt::Key_Space));
    connect(actionPlayPause_, &QAction::triggered, this, &MainWindow::togglePlayback);
    actionPlayPause_->setEnabled(false);

    auto *menuFile = menuBar()->addMenu("&File");
    menuFile->addAction(actionOpen_);
    menuFile->addAction(actionOpenVmt_);
    recentFilesMenu_ = menuFile->addMenu("Open &Recent Files");
    menuFile->addSeparator();
    menuFile->addAction(actionReloadVtf_);
    menuFile->addAction(actionOpenContainingFolder_);
    menuFile->addAction(actionCloseVtf_);
    menuFile->addAction(actionCloseVmt_);
    menuFile->addAction(actionImportImage_);
    menuFile->addSeparator();
    menuFile->addAction(actionSaveVtf_);
    menuFile->addAction(actionSaveAsVtf_);
    menuFile->addAction(actionExportPng_);
    menuFile->addAction(actionExportThumbnailPng_);
    menuFile->addAction(actionExportAs_);
    menuFile->addAction(actionExportAllFrames_);
    menuFile->addAction(actionExportAllFaces_);
    menuFile->addAction(actionExportAllSlices_);
    menuFile->addAction(actionExportAllMipmaps_);
    menuFile->addAction(actionExportAll_);
    menuFile->addSeparator();
    menuFile->addAction(actionQuit_);
    actionSetRecentLimit_ = new QAction("Set Recent Files &Limit…", this);
    connect(actionSetRecentLimit_, &QAction::triggered, this, [this] {
        bool ok = false;
        const int v = QInputDialog::getInt(
            this,
            "Recent Files Limit",
            "Maximum recent files:",
            recentMax_,
            1,
            kMaxRecentMax,
            1,
            &ok);
        if(!ok) return;
        setRecentFilesMax(v);
    });
    actionClearRecentFiles_ = new QAction("&Clear Recent Files", this);
    connect(actionClearRecentFiles_, &QAction::triggered, this, [this] {
        recentFiles_.clear();
        rebuildRecentMenus();
        saveUiState();
    });

    auto *menuEdit = menuBar()->addMenu("&Edit");
    {
        menuEdit->addAction(actionPasteImage_);
        menuEdit->addSeparator();

        auto *find = new QAction("&Find…", this);
        find->setShortcut(QKeySequence::Find);
        connect(find, &QAction::triggered, this, &MainWindow::findInVmt);
        menuEdit->addAction(find);

        auto *findNext = new QAction("Find &Next", this);
        findNext->setShortcut(QKeySequence::FindNext);
        connect(findNext, &QAction::triggered, this, &MainWindow::findNextInVmt);
        menuEdit->addAction(findNext);

        auto *replace = new QAction("&Replace…", this);
        replace->setShortcut(QKeySequence::Replace);
        connect(replace, &QAction::triggered, this, &MainWindow::replaceInVmt);
        menuEdit->addAction(replace);

        auto *goTo = new QAction("&Go to Line…", this);
        goTo->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_G));
        connect(goTo, &QAction::triggered, this, &MainWindow::goToLineInVmt);
        menuEdit->addAction(goTo);
    }

    auto *menuVtf = menuBar()->addMenu("&VTF");
    menuVtf->addAction(actionOpenMatchingVmt_);
    menuVtf->addAction(actionVtfProperties_);
    menuVtf->addAction(actionVtfResources_);
    menuVtf->addAction(actionEditVtfFlags_);
    menuVtf->addSeparator();
    menuVtf->addAction(actionSaveVtf_);
    menuVtf->addAction(actionSaveAsVtf_);

    auto *menuTools = menuBar()->addMenu("&Tools");
    menuTools->addAction(actionBatchConvert_);
    menuTools->addAction(actionWadConvert_);
    menuTools->addSeparator();
    menuTools->addAction(actionGenNormalFrame_);
    menuTools->addAction(actionGenNormalAll_);
    menuTools->addAction(actionGenSphereMap_);
    menuTools->addSeparator();
    {
        auto *optionsMenu = menuTools->addMenu("&Options");

        auto *globalOptions = new QAction("&Global Options…", this);
        connect(globalOptions, &QAction::triggered, this, [this] {
            OptionsDialog dlg(this);
            if(dlg.exec() != QDialog::Accepted) return;
            if(imageId_ != kInvalidImageId) {
                renderSelection();
                updateCubemapPreview();
            }
        });
        optionsMenu->addAction(globalOptions);
        optionsMenu->addSeparator();

        auto *warningPopups = new QAction("&Warning Popups", this);
        warningPopups->setCheckable(true);
        QSettings s;
        warningPopups->setChecked(s.value("options/warningPopups", true).toBool());
        connect(warningPopups, &QAction::toggled, this, [](bool checked) {
            QSettings().setValue("options/warningPopups", checked);
        });
        optionsMenu->addAction(warningPopups);

        auto *notificationSounds = new QAction("&Notification Sounds", this);
        notificationSounds->setCheckable(true);
        notificationSounds->setChecked(s.value("options/notificationSounds", false).toBool());
        connect(notificationSounds, &QAction::toggled, this, [](bool checked) {
            QSettings().setValue("options/notificationSounds", checked);
        });
        optionsMenu->addAction(notificationSounds);

        auto *autoCreateVmt = new QAction("&Auto Create VMT File", this);
        autoCreateVmt->setCheckable(true);
        autoCreateVmt->setChecked(s.value("options/autoCreateVmt", false).toBool());
        autoCreateVmt->setToolTip("Automatically create a basic VMT file when importing/creating a new VTF");
        connect(autoCreateVmt, &QAction::toggled, this, [](bool checked) {
            QSettings().setValue("options/autoCreateVmt", checked);
        });
        optionsMenu->addAction(autoCreateVmt);
    }

    auto *menuView = menuBar()->addMenu("&View");
    menuView->addAction(actionZoomIn_);
    menuView->addAction(actionZoomOut_);
    menuView->addAction(actionZoomReset_);
    menuView->addAction(actionZoomFit_);
    actionShowThumbnail_ = new QAction("Show &Thumbnail", this);
    actionShowThumbnail_->setCheckable(true);
    connect(actionShowThumbnail_, &QAction::toggled, this, [this](bool checked) {
        showThumbnail_ = checked;
        if(hdrTonemapCombo_) {
            bool enable = false;
            if(imageId_ != kInvalidImageId) {
                vlBindImage(imageId_);
                enable = hdrTonemapEnabled_ && !checked && vtfFormatIsHdr(vlImageGetFormat());
            }
            hdrTonemapCombo_->setEnabled(enable);
        }
        renderSelection();
    });
    actionShowThumbnail_->setEnabled(false);
    menuView->addSeparator();
    menuView->addAction(actionShowThumbnail_);
    actionHdrTonemap_ = new QAction("HDR &Tonemap (Float Formats)", this);
    actionHdrTonemap_->setCheckable(true);
    actionHdrTonemap_->setChecked(true);
    actionHdrTonemap_->setToolTip("When enabled, float/HDR VTF formats are previewed with exposure + tonemapping.");
    actionHdrTonemap_->setEnabled(false);
    connect(actionHdrTonemap_, &QAction::toggled, this, [this](bool checked) {
        hdrTonemapEnabled_ = checked;
        if(hdrTonemapCombo_) {
            bool enable = false;
            if(imageId_ != kInvalidImageId) {
                vlBindImage(imageId_);
                enable = checked && vtfFormatIsHdr(vlImageGetFormat()) && !showThumbnail_;
            }
            hdrTonemapCombo_->setEnabled(enable);
        }
        renderSelection();
        updateCubemapPreview();
    });
    menuView->addAction(actionHdrTonemap_);
    actionVmtHighlight_ = new QAction("VMT Syntax &Highlighting", this);
    actionVmtHighlight_->setCheckable(true);
    actionVmtHighlight_->setChecked(false);
    actionVmtHighlight_->setToolTip("Optional: enables a lightweight VMT syntax highlighter (may affect performance on huge files).");
    connect(actionVmtHighlight_, &QAction::toggled, this, [this](bool checked) {
        if(!vmtEditor_) return;
        if(checked) {
            if(!vmtHighlighter_) vmtHighlighter_ = new VmtHighlighter(vmtEditor_->document());
        } else {
            delete vmtHighlighter_;
            vmtHighlighter_ = nullptr;
        }
    });
    menuView->addAction(actionVmtHighlight_);
    actionCubemapPreview_ = new QAction("&Cubemap Preview", this);
    actionCubemapPreview_->setCheckable(true);
    actionCubemapPreview_->setChecked(false);
    actionCubemapPreview_->setEnabled(false);
    connect(actionCubemapPreview_, &QAction::toggled, this, &MainWindow::cubemapPreviewToggled);
    menuView->addAction(actionCubemapPreview_);
    menuView->addSeparator();
    actionMask_ = new QAction("&Mask", this);
    actionMask_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    actionMask_->setCheckable(true);
    actionMask_->setChecked(false);
    actionMask_->setToolTip("Toggle alpha compositing over the selected background (Ctrl+W).");
    connect(actionMask_, &QAction::toggled, this, [this](bool checked) {
        maskMode_ = checked;
        renderSelection();
    });
    menuView->addAction(actionMask_);
    actionTile_ = new QAction("&Tile", this);
    actionTile_->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_T));
    actionTile_->setCheckable(true);
    actionTile_->setChecked(false);
    actionTile_->setToolTip("Tile the texture in a 2x2 grid");
    connect(actionTile_, &QAction::toggled, this, [this](bool checked) {
        tileMode_ = checked;
        renderSelection();
    });
    menuView->addAction(actionTile_);
    menuView->addSeparator();
    menuView->addAction(actionCopyImage_);
    menuView->addSeparator();
    menuView->addAction(actionPlayPause_);

    auto *menuHelp = menuBar()->addMenu("&Help");
    {
        auto *checkUpdates = new QAction("Check for &Updates…", this);
        connect(checkUpdates, &QAction::triggered, this, [this] {
            const QString url = "https://github.com/programmer1o1/VTFEdit-Reloaded/releases";
            if(!QDesktopServices::openUrl(QUrl(url))) {
                QMessageBox::information(this, "Check for Updates",
                    QString("Could not open browser.\nPlease visit:\n%1").arg(url));
            }
        });
        menuHelp->addAction(checkUpdates);
        menuHelp->addSeparator();

        auto *about = new QAction("&About", this);
        connect(about, &QAction::triggered, this, [this] {
            QString text;
            text += "VTFEditQt (VTFEdit-Reloaded)\n\n";
            text += "Features: VTF preview/edit, VMT editor/validation, batch convert.\n";
            text += "Notes:\n";
            text += "- DXT output requires a compressor (e.g. Compressonator).\n";
            text += "- VTF resources are supported by VTF v7.3+.\n";
            QMessageBox::about(this, "About VTFEditQt", text);
        });
        menuHelp->addAction(about);
        auto *aboutQt = new QAction("About &Qt", this);
        connect(aboutQt, &QAction::triggered, this, [] { QMessageBox::aboutQt(nullptr); });
        menuHelp->addAction(aboutQt);
    }

    auto *toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->addAction(actionOpen_);
    toolbar->addAction(actionOpenVmt_);
    toolbar->addAction(actionImportImage_);
    toolbar->addSeparator();
    toolbar->addAction(actionReloadVtf_);
    toolbar->addAction(actionCloseVtf_);
    toolbar->addSeparator();
    toolbar->addAction(actionVtfProperties_);
    toolbar->addAction(actionVtfResources_);
    toolbar->addAction(actionEditVtfFlags_);
    toolbar->addSeparator();
    toolbar->addAction(actionExportPng_);
    toolbar->addAction(actionExportAs_);
    toolbar->addAction(actionExportAllFrames_);
    toolbar->addAction(actionExportAllFaces_);
    toolbar->addAction(actionCopyImage_);
    toolbar->addSeparator();
    toolbar->addAction(actionPlayPause_);
    toolbar->addSeparator();

    frameSpin_ = new QSpinBox(this);
    frameSpin_->setPrefix("Frame ");
    frameSpin_->setRange(0, 0);
    connect(frameSpin_, &QSpinBox::valueChanged, this, &MainWindow::selectionChanged);
    toolbar->addWidget(frameSpin_);

    faceSpin_ = new QSpinBox(this);
    faceSpin_->setPrefix("Face ");
    faceSpin_->setRange(0, 0);
    connect(faceSpin_, &QSpinBox::valueChanged, this, &MainWindow::selectionChanged);
    toolbar->addWidget(faceSpin_);

    sliceSpin_ = new QSpinBox(this);
    sliceSpin_->setPrefix("Slice ");
    sliceSpin_->setRange(0, 0);
    connect(sliceSpin_, &QSpinBox::valueChanged, this, &MainWindow::selectionChanged);
    toolbar->addWidget(sliceSpin_);

    mipSpin_ = new QSpinBox(this);
    mipSpin_->setPrefix("Mip ");
    mipSpin_->setRange(0, 0);
    connect(mipSpin_, &QSpinBox::valueChanged, this, &MainWindow::selectionChanged);
    toolbar->addWidget(mipSpin_);

    toolbar->addSeparator();
    channelCombo_ = new QComboBox(this);
    channelCombo_->addItem("RGBA", static_cast<int>(ChannelMode::RGBA));
    channelCombo_->addItem("RGB", static_cast<int>(ChannelMode::RGB));
    channelCombo_->addItem("R", static_cast<int>(ChannelMode::R));
    channelCombo_->addItem("G", static_cast<int>(ChannelMode::G));
    channelCombo_->addItem("B", static_cast<int>(ChannelMode::B));
    channelCombo_->addItem("A", static_cast<int>(ChannelMode::A));
    connect(channelCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::selectionChanged);
    toolbar->addWidget(channelCombo_);

    backgroundCombo_ = new QComboBox(this);
    backgroundCombo_->addItem("Checker", static_cast<int>(BackgroundMode::Checker));
    backgroundCombo_->addItem("Black", static_cast<int>(BackgroundMode::Black));
    backgroundCombo_->addItem("White", static_cast<int>(BackgroundMode::White));
    backgroundCombo_->addItem("Custom…", static_cast<int>(BackgroundMode::Custom));
    connect(backgroundCombo_, &QComboBox::currentIndexChanged, this, &MainWindow::selectionChanged);
    connect(backgroundCombo_, QOverload<int>::of(&QComboBox::activated), this, [this](int) {
        const auto bgMode = static_cast<BackgroundMode>(backgroundCombo_->currentData().toInt());
        if(bgMode == BackgroundMode::Custom) {
            selectCustomBackgroundColor();
        }
    });
    toolbar->addWidget(backgroundCombo_);
    updateBackgroundComboCustomIcon();

    exposureSlider_ = new QSlider(Qt::Horizontal, this);
    exposureSlider_->setRange(0, 8000);
    {
        QSettings s;
        const int v = s.value("options/hdr/exposure100", 2000).toInt();
        exposureSlider_->setValue(std::clamp(v, 0, 8000));
        hdrExposure_ = static_cast<double>(exposureSlider_->value()) / 100.0;
        vlSetFloat(VTFLIB_FP16_HDR_EXPOSURE, static_cast<vlSingle>(hdrExposure_));
    }
    exposureSlider_->setToolTip("HDR exposure for preview (0.00–80.00).\nMatches Windows VTFEdit's FP16 exposure scale.");
    connect(exposureSlider_, &QSlider::valueChanged, this, [this](int v) {
        hdrExposure_ = static_cast<double>(v) / 100.0;
        QSettings().setValue("options/hdr/exposure100", v);
        vlSetFloat(VTFLIB_FP16_HDR_EXPOSURE, static_cast<vlSingle>(hdrExposure_));

        if(imageId_ == kInvalidImageId) return;
        vlBindImage(imageId_);
        if(!hdrTonemapEnabled_ || showThumbnail_ || !vtfFormatIsHdr(vlImageGetFormat())) return;
        renderSelection();
        updateCubemapPreview();
    });
    toolbar->addWidget(exposureSlider_);

    hdrTonemapCombo_ = new QComboBox(this);
    hdrTonemapCombo_->addItem("VTFLib (Windows)", static_cast<int>(HdrTonemap::VTFLib));
    hdrTonemapCombo_->addItem("Reinhard", static_cast<int>(HdrTonemap::Reinhard));
    hdrTonemapCombo_->addItem("ACES", static_cast<int>(HdrTonemap::ACES));
    hdrTonemapCombo_->addItem("Clamp", static_cast<int>(HdrTonemap::Clamp));
    hdrTonemapCombo_->setToolTip("HDR preview mode. 'VTFLib (Windows)' matches the original VTFEdit preview.");
    {
        QSettings s;
        const int saved = s.value("options/hdr/previewMode", static_cast<int>(HdrTonemap::VTFLib)).toInt();
        const int idx = hdrTonemapCombo_->findData(saved);
        hdrTonemapCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
        hdrTonemap_ = static_cast<HdrTonemap>(hdrTonemapCombo_->currentData().toInt());
    }
    hdrTonemapCombo_->setEnabled(false);
    connect(hdrTonemapCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        hdrTonemap_ = static_cast<HdrTonemap>(hdrTonemapCombo_->currentData().toInt());
        QSettings().setValue("options/hdr/previewMode", static_cast<int>(hdrTonemap_));
        if(imageId_ != kInvalidImageId) {
            renderSelection();
            updateCubemapPreview();
        }
    });
    toolbar->addWidget(hdrTonemapCombo_);

    frameMsSpin_ = new QSpinBox(this);
    frameMsSpin_->setRange(1, 10000);
    frameMsSpin_->setValue(100);
    frameMsSpin_->setSuffix(" ms");
    connect(frameMsSpin_, &QSpinBox::valueChanged, this, [this](int) {
        if(isPlaying_) playbackTimer_->setInterval(frameMsSpin_->value());
    });
    toolbar->addWidget(frameMsSpin_);

    toolbar->addSeparator();
    toolbar->addAction(actionZoomOut_);
    toolbar->addAction(actionZoomIn_);
    toolbar->addAction(actionZoomReset_);
    toolbar->addAction(actionZoomFit_);
    zoomCombo_ = new QComboBox(this);
    zoomCombo_->setEditable(true);
    zoomCombo_->setInsertPolicy(QComboBox::NoInsert);
    zoomCombo_->setMinimumContentsLength(6);
    zoomCombo_->setToolTip("Type a percentage (e.g. 150%) or pick a preset.\nCtrl+Mouse Wheel also zooms.");
    zoomCombo_->addItems({"25%", "50%", "100%", "200%", "400%", "800%"});
    zoomCombo_->setCurrentText("100%");
    connect(zoomCombo_, &QComboBox::currentTextChanged, this, &MainWindow::zoomComboChanged);
    zoomCombo_->setEnabled(false);
    toolbar->addWidget(zoomCombo_);

    imageLabel_ = new QLabel;
    imageLabel_->setBackgroundRole(QPalette::Base);
    imageLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel_->setScaledContents(true);
    imageLabel_->setAlignment(Qt::AlignCenter);

    scrollArea_ = new QScrollArea;
    scrollArea_->setBackgroundRole(QPalette::Dark);
    scrollArea_->setWidget(imageLabel_);
    setCentralWidget(scrollArea_);
    scrollArea_->setFocusPolicy(Qt::StrongFocus);
    scrollArea_->viewport()->setFocusPolicy(Qt::StrongFocus);
    imageLabel_->setFocusPolicy(Qt::StrongFocus);
    scrollArea_->viewport()->setToolTip("Ctrl+Mouse Wheel: zoom\nCtrl+V: paste image as new VTF\nMiddle-drag or Alt+Left-drag: pan");
    scrollArea_->viewport()->setMouseTracking(true);
    imageLabel_->setMouseTracking(true);
    scrollArea_->viewport()->installEventFilter(this);
    imageLabel_->installEventFilter(this);

    auto *infoWidget = new QWidget(this);
    auto *infoLayout = new QFormLayout(infoWidget);
    infoLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    infoPath_ = new QLabel("-");
    infoPath_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoPath_->setWordWrap(true);
    infoPath_->setMinimumWidth(80);
    infoLayout->addRow("Path:", infoPath_);

    infoVersion_ = new QLabel("-");
    infoLayout->addRow("VTF version:", infoVersion_);

    infoDimensions_ = new QLabel("-");
    infoLayout->addRow("Dimensions:", infoDimensions_);

    infoFormat_ = new QLabel("-");
    infoLayout->addRow("Format:", infoFormat_);

    infoMipmaps_ = new QLabel("-");
    infoLayout->addRow("Mipmaps:", infoMipmaps_);

    infoFrames_ = new QLabel("-");
    infoLayout->addRow("Frames:", infoFrames_);

    infoFaces_ = new QLabel("-");
    infoLayout->addRow("Faces:", infoFaces_);

    infoFlags_ = new QLabel("-");
    infoFlags_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoFlags_->setWordWrap(true);
    infoLayout->addRow("Flags:", infoFlags_);

    infoStartFrame_ = new QLabel("-");
    infoLayout->addRow("Start frame:", infoStartFrame_);

    infoBumpScale_ = new QLabel("-");
    infoLayout->addRow("Bump scale:", infoBumpScale_);

    infoReflectivity_ = new QLabel("-");
    infoLayout->addRow("Reflectivity:", infoReflectivity_);

    auto *dock = new QDockWidget("Info", this);
    dock->setWidget(infoWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    resourcesTree_ = new QTreeWidget(this);
    resourcesTree_->setColumnCount(2);
    resourcesTree_->setHeaderLabels({"Resource", "Value"});
    resourcesTree_->setUniformRowHeights(true);
    resourcesTree_->setRootIsDecorated(true);
    resourcesTree_->setAlternatingRowColors(true);
    resourcesTree_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(resourcesTree_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        if(imageId_ == kInvalidImageId) return;
        vlBindImage(imageId_);
        if(!vlImageGetSupportsResources()) return;

        QTreeWidgetItem *item = resourcesTree_->itemAt(pos);
        while(item) {
            bool ok = false;
            const uint type = item->data(0, Qt::UserRole).toUInt(&ok);
            if(ok) {
                QMenu menu(this);
                auto *edit = menu.addAction("Edit Resources…");
                QAction *editSheet = nullptr;
                if(type == VTF_RSRC_SHEET) editSheet = menu.addAction("Edit Sheet…");
                auto *viewHex = menu.addAction("View Resource (Hex)…");
                auto *exportRes = menu.addAction("Export Resource…");
                auto *importRes = menu.addAction("Import/Replace Resource…");
                auto *removeRes = menu.addAction("Remove Resource");

                QAction *chosen = menu.exec(resourcesTree_->viewport()->mapToGlobal(pos));
                if(!chosen) return;
                if(chosen == edit) {
                    vtfResourcesDialog();
                    return;
                }
                if(editSheet && chosen == editSheet) {
                    vtfSheetDialog();
                    return;
                }

                const vlUInt t = static_cast<vlUInt>(type);
                if(chosen == viewHex) {
                    vlUInt size = 0;
                    void *data = vlImageGetResourceData(t, &size);
                    QByteArray bytes;
                    if(data && size) bytes = QByteArray(reinterpret_cast<const char *>(data), static_cast<int>(size));
                    ResourceHexDialog dlg(QString("Resource 0x%1").arg(static_cast<qulonglong>(t), 8, 16, QChar('0')).toUpper(), bytes, this);
                    dlg.exec();
                    return;
                }
                if(chosen == exportRes) {
                    vlUInt size = 0;
                    void *data = vlImageGetResourceData(t, &size);
                    if(!data || size == 0) {
                        QMessageBox::information(this, "Export Resource", "This resource has no data.");
                        return;
                    }
                    const QString suggested = QString("%1_%2.bin")
                                                  .arg(QFileInfo(currentPath_).completeBaseName())
                                                  .arg(QString::number(static_cast<qulonglong>(t), 16).toUpper());
                    const QString out = QFileDialog::getSaveFileName(this, "Export Resource", suggested, "Binary (*.bin);;All files (*.*)");
                    if(out.isEmpty()) return;
                    QFile f(out);
                    if(!f.open(QIODevice::WriteOnly)) {
                        showWarningPopup("Export failed", QString("Failed to open %1 for writing.").arg(out));
                        return;
                    }
                    f.write(reinterpret_cast<const char *>(data), static_cast<qint64>(size));
                    f.close();
                    setStatusInfo(QString("Exported resource to %1").arg(QFileInfo(out).fileName()));
                    return;
                }
                if(chosen == importRes) {
                    const QString in = QFileDialog::getOpenFileName(this, "Import Resource", {}, "Binary (*.bin);;All files (*.*)");
                    if(in.isEmpty()) return;
                    QFile f(in);
                    if(!f.open(QIODevice::ReadOnly)) {
                        showWarningPopup("Import failed", QString("Failed to open %1 for reading.").arg(in));
                        return;
                    }
                    QByteArray bytes = f.readAll();
                    f.close();
                    if(!vlImageSetResourceData(t, static_cast<vlUInt>(bytes.size()), bytes.data())) {
                        showWarningPopup("Import failed", QString::fromUtf8(vlGetLastError()));
                        return;
                    }
                    vtfDirty_ = true;
                    actionSaveVtf_->setEnabled(true);
                    updateWindowTitle();
                    rebuildResourcesTree();
                    setStatusInfo("Imported resource");
                    return;
                }
                if(chosen == removeRes) {
                    if(!vlImageSetResourceData(t, 0, nullptr)) {
                        showWarningPopup("Remove failed", QString::fromUtf8(vlGetLastError()));
                        return;
                    }
                    vtfDirty_ = true;
                    actionSaveVtf_->setEnabled(true);
                    updateWindowTitle();
                    rebuildResourcesTree();
                    setStatusInfo("Removed resource");
                    return;
                }
                return;
            }
            item = item->parent();
        }
    });
    connect(resourcesTree_, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int) {
        if(!item) return;
        bool ok = false;
        const uint type = item->data(0, Qt::UserRole).toUInt(&ok);
        if(!ok) return;
        if(type == VTF_RSRC_CRC || type == VTF_RSRC_TEXTURE_LOD_SETTINGS || type == VTF_RSRC_TEXTURE_SETTINGS_EX || type == VTF_RSRC_KEY_VALUE_DATA) {
            vtfResourcesDialog();
        } else if(type == VTF_RSRC_SHEET) {
            vtfSheetDialog();
        } else {
            if(imageId_ == kInvalidImageId) return;
            vlBindImage(imageId_);
            vlUInt size = 0;
            void *data = vlImageGetResourceData(static_cast<vlUInt>(type), &size);
            QByteArray bytes;
            if(data && size) bytes = QByteArray(reinterpret_cast<const char *>(data), static_cast<int>(size));
            ResourceHexDialog dlg(QString("Resource 0x%1").arg(static_cast<qulonglong>(type), 8, 16, QChar('0')).toUpper(), bytes, this);
            dlg.exec();
        }
    });
    resourcesDock_ = new QDockWidget("Resources", this);
    resourcesDock_->setWidget(resourcesTree_);
    addDockWidget(Qt::RightDockWidgetArea, resourcesDock_);
    tabifyDockWidget(dock, resourcesDock_);
    resourcesDock_->hide();

    {
        auto *cubemapWidget = new QWidget(this);
        auto *layout = new QVBoxLayout(cubemapWidget);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);

        auto *expRow = new QWidget(cubemapWidget);
        auto *expLayout = new QHBoxLayout(expRow);
        expLayout->setContentsMargins(0, 0, 0, 0);
        expLayout->addWidget(new QLabel("HDR Exposure:", expRow));
        cubemapExposure_ = new QSlider(Qt::Horizontal, expRow);
        cubemapExposure_->setRange(0, 8000);
        {
            QSettings s;
            const int v = s.value("options/hdr/cubemapExposure100", 2000).toInt();
            cubemapExposure_->setValue(std::clamp(v, 0, 8000));
            cubemapHdrExposure_ = static_cast<double>(cubemapExposure_->value()) / 100.0;
        }
        connect(cubemapExposure_, &QSlider::valueChanged, this, [this](int v) {
            cubemapHdrExposure_ = static_cast<double>(v) / 100.0;
            QSettings().setValue("options/hdr/cubemapExposure100", v);
            updateCubemapPreview();
        });
        expLayout->addWidget(cubemapExposure_, 1);
        layout->addWidget(expRow);

        auto *grid = new QGridLayout();
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(6);

        cubeFaceGroup_ = new QButtonGroup(cubemapWidget);
        cubeFaceGroup_->setExclusive(true);

        auto mkBtn = [&](const QString &title, int face) {
            auto *b = new QToolButton(cubemapWidget);
            b->setText(title);
            b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            b->setIconSize(QSize(96, 96));
            b->setCheckable(true);
            b->setEnabled(false);
            cubeFaceGroup_->addButton(b, face);
            connect(b, &QToolButton::clicked, this, [this, face] {
                if(faceSpin_) faceSpin_->setValue(face);
            });
            return b;
        };

        cubeFaceButtons_[CUBEMAP_FACE_UP] = mkBtn("+Z Up", CUBEMAP_FACE_UP);
        cubeFaceButtons_[CUBEMAP_FACE_LEFT] = mkBtn("-X Left", CUBEMAP_FACE_LEFT);
        cubeFaceButtons_[CUBEMAP_FACE_FRONT] = mkBtn("-Y Front", CUBEMAP_FACE_FRONT);
        cubeFaceButtons_[CUBEMAP_FACE_RIGHT] = mkBtn("+X Right", CUBEMAP_FACE_RIGHT);
        cubeFaceButtons_[CUBEMAP_FACE_BACK] = mkBtn("+Y Back", CUBEMAP_FACE_BACK);
        cubeFaceButtons_[CUBEMAP_FACE_DOWN] = mkBtn("-Z Down", CUBEMAP_FACE_DOWN);
        cubeFaceButtons_[CUBEMAP_FACE_SPHERE_MAP] = mkBtn("Sphere Map", CUBEMAP_FACE_SPHERE_MAP);

        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_UP], 0, 1);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_LEFT], 1, 0);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_FRONT], 1, 1);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_RIGHT], 1, 2);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_BACK], 1, 3);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_DOWN], 2, 1);
        grid->addWidget(cubeFaceButtons_[CUBEMAP_FACE_SPHERE_MAP], 2, 3);

        layout->addLayout(grid);
        layout->addStretch(1);

        cubemapDock_ = new QDockWidget("Cubemap Preview", this);
        cubemapDock_->setWidget(cubemapWidget);
        addDockWidget(Qt::LeftDockWidgetArea, cubemapDock_);
        cubemapDock_->hide();
        connect(cubemapDock_, &QDockWidget::visibilityChanged, this, [this](bool visible) {
            if(!actionCubemapPreview_) return;
            QSignalBlocker b(actionCubemapPreview_);
            actionCubemapPreview_->setChecked(visible);
        });
    }

    vmtEditor_ = new QPlainTextEdit(this);
    vmtEditor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    {
        const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        vmtEditor_->setFont(mono);
        vmtEditor_->setTabStopDistance(QFontMetricsF(mono).horizontalAdvance(' ') * 4);
    }
    connect(vmtEditor_->document(), &QObject::destroyed, this, [this] {
        vmtHighlighter_ = nullptr;
    });
    connect(vmtEditor_->document(), &QTextDocument::modificationChanged, this, [this](bool) {
        updateWindowTitle();
        updateVmtStatusText();
    });
    connect(vmtEditor_, &QPlainTextEdit::textChanged, this, [this] {
        if(liveValidateVmt_ && vmtValidateTimer_) vmtValidateTimer_->start();
    });
    auto *vmtDock = new QDockWidget("VMT", this);
    vmtDock->setWidget(vmtEditor_);
    addDockWidget(Qt::BottomDockWidgetArea, vmtDock);

    actionSaveVmt_ = new QAction("&Save VMT", this);
    actionSaveVmt_->setShortcut(QKeySequence::Save);
    connect(actionSaveVmt_, &QAction::triggered, this, &MainWindow::saveVmt);

    actionSaveVmtAs_ = new QAction("Save VMT &As…", this);
    connect(actionSaveVmtAs_, &QAction::triggered, this, &MainWindow::saveVmtAs);

    actionFormatVmt_ = new QAction("&Format VMT", this);
    actionFormatVmt_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_I));
    connect(actionFormatVmt_, &QAction::triggered, this, &MainWindow::formatVmt);

    actionLiveValidateVmt_ = new QAction("&Live Validate VMT (Loose)", this);
    actionLiveValidateVmt_->setCheckable(true);
    actionLiveValidateVmt_->setChecked(false);
    connect(actionLiveValidateVmt_, &QAction::toggled, this, &MainWindow::liveValidateVmtToggled);

    actionValidateVmtStrict_ = new QAction("Validate (Strict)", this);
    connect(actionValidateVmtStrict_, &QAction::triggered, this, &MainWindow::validateVmtStrict);

    actionValidateVmtLoose_ = new QAction("Validate (Loose)", this);
    connect(actionValidateVmtLoose_, &QAction::triggered, this, &MainWindow::validateVmtLoose);

    actionNewVmt_ = new QAction("&New VMT…", this);
    actionNewVmt_->setShortcut(QKeySequence::New);
    connect(actionNewVmt_, &QAction::triggered, this, &MainWindow::newVmtDialog);

    actionCreateVmtFromVtf_ = new QAction("Create VMT from Current VTF…", this);
    connect(actionCreateVmtFromVtf_, &QAction::triggered, this, &MainWindow::createVmtFromCurrentVtf);
    actionCreateVmtFromVtf_->setEnabled(false);

    auto *menuVmt = menuBar()->addMenu("&VMT");
    menuVmt->addAction(actionNewVmt_);
    menuVmt->addAction(actionOpenVmt_);
    menuVmt->addAction(actionCloseVmt_);
    menuVmt->addAction(actionOpenLinkedVtf_);
    menuVmt->addAction(actionCreateVmtFromVtf_);
    menuVmt->addSeparator();
    menuVmt->addAction(actionSaveVmt_);
    menuVmt->addAction(actionSaveVmtAs_);
    menuVmt->addSeparator();
    menuVmt->addAction(actionFormatVmt_);
    menuVmt->addAction(actionLiveValidateVmt_);
    menuVmt->addSeparator();
    menuVmt->addAction(actionValidateVmtStrict_);
    menuVmt->addAction(actionValidateVmtLoose_);

    vmtValidateTimer_ = new QTimer(this);
    vmtValidateTimer_->setSingleShot(true);
    vmtValidateTimer_->setInterval(450);
    connect(vmtValidateTimer_, &QTimer::timeout, this, &MainWindow::liveValidateVmtTick);

    playbackTimer_ = new QTimer(this);
    playbackTimer_->setInterval(frameMsSpin_->value());
    connect(playbackTimer_, &QTimer::timeout, this, &MainWindow::playbackTick);

    statusBar()->showMessage("Ready");
    statusPixel_ = new QLabel(this);
    statusPixel_->setText("-");
    statusPixel_->setMinimumWidth(170);
    statusBar()->addPermanentWidget(statusPixel_);
    statusSelection_ = new QLabel(this);
    statusSelection_->setText("-");
    statusSelection_->setMinimumWidth(190);
    statusBar()->addPermanentWidget(statusSelection_);
    statusZoom_ = new QLabel(this);
    statusZoom_->setText("Zoom: -");
    statusBar()->addPermanentWidget(statusZoom_);
    statusVmt_ = new QLabel(this);
    statusVmt_->setText("VMT: -");
    statusVmt_->setMinimumWidth(100);
    statusBar()->addPermanentWidget(statusVmt_);
    closeVtf();
    closeVmt();
    loadUiState();
}

MainWindow::~MainWindow() {
    closeVtf();
    closeVmt();
}

void MainWindow::closeVtf() {
    if(imageId_ != kInvalidImageId) {
        vlDeleteImage(imageId_);
        imageId_ = kInvalidImageId;
    }
    currentPath_.clear();
    rawRgbaImage_ = {};
    viewImage_ = {};
    vtfDirty_ = false;
    zoom_ = 1.0;
    prevManualZoom_ = 1.0;
    fitToWindow_ = false;
    if(actionZoomFit_) actionZoomFit_->setChecked(false);
    frame_ = 0;
    face_ = 0;
    slice_ = 0;
    mip_ = 0;
    isPlaying_ = false;
    playbackTimer_->stop();
    actionPlayPause_->setText("&Play");

    infoPath_->setText("-");
    infoVersion_->setText("-");
    infoDimensions_->setText("-");
    infoFormat_->setText("-");
    infoMipmaps_->setText("-");
    infoFrames_->setText("-");
    infoFaces_->setText("-");
    infoFlags_->setText("-");
    infoStartFrame_->setText("-");
    infoBumpScale_->setText("-");
    infoReflectivity_->setText("-");

    if(resourcesTree_) resourcesTree_->clear();
    if(resourcesDock_) resourcesDock_->hide();
    if(cubemapDock_) cubemapDock_->hide();
    if(actionCubemapPreview_) {
        actionCubemapPreview_->setEnabled(false);
        actionCubemapPreview_->setChecked(false);
    }
    for(auto *b : cubeFaceButtons_) {
        if(!b) continue;
        b->setEnabled(false);
        b->setIcon(QIcon());
        b->setChecked(false);
    }
    if(cubemapExposure_) cubemapExposure_->setValue(2000);
    cubemapHdrExposure_ = static_cast<double>(cubemapExposure_ ? cubemapExposure_->value() : 2000) / 100.0;

    setPreviewPlaceholderText("Drop a .vtf/.vmt/image here or use File → Open…\nTip: Ctrl+Mouse Wheel to zoom.");
    updateWindowTitle();

    if(statusPixel_) statusPixel_->setText("-");
    if(statusSelection_) statusSelection_->setText("-");
    if(statusZoom_) statusZoom_->setText("Zoom: -");
    if(statusVmt_) statusVmt_->setText("VMT: -");

    actionReloadVtf_->setEnabled(false);
    actionOpenContainingFolder_->setEnabled(false);
    actionCloseVtf_->setEnabled(false);
    actionOpenMatchingVmt_->setEnabled(false);
    actionExportPng_->setEnabled(false);
    actionExportThumbnailPng_->setEnabled(false);
    actionExportAs_->setEnabled(false);
    actionCopyImage_->setEnabled(false);
    actionExportAllFrames_->setEnabled(false);
    actionExportAllFaces_->setEnabled(false);
    actionExportAllSlices_->setEnabled(false);
    actionExportAllMipmaps_->setEnabled(false);
    actionExportAll_->setEnabled(false);
    actionSaveAsVtf_->setEnabled(false);
    actionSaveVtf_->setEnabled(false);
    actionEditVtfFlags_->setEnabled(false);
    actionVtfProperties_->setEnabled(false);
    actionVtfResources_->setEnabled(false);
    actionCreateVmtFromVtf_->setEnabled(false);
    actionZoomIn_->setEnabled(false);
    actionZoomOut_->setEnabled(false);
    actionZoomReset_->setEnabled(false);
    actionZoomFit_->setEnabled(false);
    actionShowThumbnail_->setEnabled(false);
    actionShowThumbnail_->setChecked(false);
    if(actionHdrTonemap_) actionHdrTonemap_->setEnabled(false);
    if(hdrTonemapCombo_) hdrTonemapCombo_->setEnabled(false);
    actionCubemapPreview_->setEnabled(false);
    actionCubemapPreview_->setChecked(false);
    actionPlayPause_->setEnabled(false);
    actionGenNormalFrame_->setEnabled(false);
    actionGenNormalAll_->setEnabled(false);
    actionGenSphereMap_->setEnabled(false);
    if(zoomCombo_) zoomCombo_->setEnabled(false);
    frameSpin_->setEnabled(false);
    faceSpin_->setEnabled(false);
    sliceSpin_->setEnabled(false);
    mipSpin_->setEnabled(false);
    channelCombo_->setEnabled(false);
    backgroundCombo_->setEnabled(false);
    frameMsSpin_->setEnabled(false);
}

void MainWindow::closeVmt() {
    currentVmtPath_.clear();
    if(vmtEditor_) vmtEditor_->clear();
    actionSaveVmt_->setEnabled(false);
    actionSaveVmtAs_->setEnabled(false);
    actionValidateVmtStrict_->setEnabled(false);
    actionValidateVmtLoose_->setEnabled(false);
    actionCloseVmt_->setEnabled(false);
    actionOpenLinkedVtf_->setEnabled(false);
    if(actionFormatVmt_) actionFormatVmt_->setEnabled(false);
    if(actionLiveValidateVmt_) actionLiveValidateVmt_->setEnabled(false);
    liveValidateVmt_ = false;
    if(actionLiveValidateVmt_) actionLiveValidateVmt_->setChecked(false);
    updateWindowTitle();
    updateVmtStatusText();
}

bool MainWindow::maybeSaveVmt() {
    if(!vmtEditor_) return true;
    if(!vmtEditor_->document()->isModified()) return true;

    const auto result = QMessageBox::question(this, "Unsaved VMT", "Save changes to the VMT?",
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(result == QMessageBox::Cancel) return false;
    if(result == QMessageBox::No) return true;
    saveVmt();
    return !vmtEditor_->document()->isModified();
}

void MainWindow::setVmtText(const QString &text, bool markModified) {
    if(!vmtEditor_) return;
    vmtEditor_->setPlainText(text);
    vmtEditor_->document()->setModified(markModified);
    actionSaveVmt_->setEnabled(true);
    actionSaveVmtAs_->setEnabled(true);
    actionValidateVmtStrict_->setEnabled(true);
    actionValidateVmtLoose_->setEnabled(true);
    actionCloseVmt_->setEnabled(true);
    actionOpenLinkedVtf_->setEnabled(true);
    if(actionFormatVmt_) actionFormatVmt_->setEnabled(true);
    if(actionLiveValidateVmt_) actionLiveValidateVmt_->setEnabled(true);
    updateWindowTitle();
    updateVmtStatusText();
}

QString MainWindow::vmtText() const {
    return vmtEditor_ ? vmtEditor_->toPlainText() : QString{};
}

void MainWindow::setStatusError(const QString &message) {
    statusBar()->showMessage(message, 8000);
}

void MainWindow::setStatusInfo(const QString &message) {
    statusBar()->showMessage(message, 3000);
}

void MainWindow::schedulePreviewRefresh() {
    if(imageId_ == kInvalidImageId) return;
    if(!imageLabel_) return;
    if(labelHasPixmap(imageLabel_)) return;

    const std::uint64_t token = ++previewRefreshToken_;
    // Try again after the event loop has a chance to process pending widget/layout updates.
    QTimer::singleShot(0, this, [this, token] { previewRefreshAttempt(token, 0); });
}

void MainWindow::previewRefreshAttempt(std::uint64_t token, int attempt) {
    if(token != previewRefreshToken_) return;
    if(imageId_ == kInvalidImageId) return;
    if(!imageLabel_ || !scrollArea_) return;
    if(labelHasPixmap(imageLabel_)) return;

    constexpr int kMaxAttempts = 4;
    const bool finalAttempt = (attempt >= kMaxAttempts - 1);

    // If we already have decoded pixels but the label isn't showing them, just re-apply the view.
    if(!rawRgbaImage_.isNull()) {
        setViewImage(rawRgbaImage_);
    } else {
        renderSelection(finalAttempt);
    }

    if(token != previewRefreshToken_) return;
    if(imageId_ == kInvalidImageId) return;

    if(imageLabel_) imageLabel_->update();
    if(scrollArea_->viewport()) scrollArea_->viewport()->update();

    if(labelHasPixmap(imageLabel_)) return;
    if(finalAttempt) return;

    int delayMs = 0;
    switch(attempt) {
        case 0: delayMs = 30; break;
        case 1: delayMs = 120; break;
        case 2: delayMs = 350; break;
        default: delayMs = 350; break;
    }

    QTimer::singleShot(delayMs, this, [this, token, attempt] { previewRefreshAttempt(token, attempt + 1); });
}

bool MainWindow::warningPopupsEnabled() const {
    return QSettings().value("options/warningPopups", true).toBool();
}

bool MainWindow::notificationSoundsEnabled() const {
    return QSettings().value("options/notificationSounds", false).toBool();
}

bool MainWindow::autoCreateVmtEnabled() const {
    return QSettings().value("options/autoCreateVmt", false).toBool();
}

void MainWindow::showInfoPopup(const QString &title, const QString &message) {
    setStatusInfo(message);
    if(!warningPopupsEnabled()) return;
    QMessageBox::information(this, title, message);
}

void MainWindow::showWarningPopup(const QString &title, const QString &message) {
    setStatusError(message);
    if(!warningPopupsEnabled()) return;
    if(notificationSoundsEnabled()) QApplication::beep();
    QMessageBox::warning(this, title, message);
}

void MainWindow::showErrorPopup(const QString &title, const QString &message) {
    setStatusError(message);
    if(!warningPopupsEnabled()) return;
    if(notificationSoundsEnabled()) QApplication::beep();
    QMessageBox::critical(this, title, message);
}

void MainWindow::addRecentVtf(const QString &path) {
    addRecentFile(RecentFileKind::Vtf, path);
}

void MainWindow::addRecentVmt(const QString &path) {
    addRecentFile(RecentFileKind::Vmt, path);
}

void MainWindow::rebuildRecentMenus() {
    if(!recentFilesMenu_) return;
    recentFilesMenu_->clear();

    if(recentFiles_.isEmpty()) {
        auto *a = recentFilesMenu_->addAction("(None)");
        a->setEnabled(false);
    } else {
        for(const auto &e : recentFiles_) {
            const QString name = QFileInfo(e.path).fileName();
            const QString tag = (e.kind == RecentFileKind::Vtf) ? "VTF" : "VMT";
            auto *a = recentFilesMenu_->addAction(QString("%1  [%2]").arg(name, tag));
            a->setToolTip(QDir::toNativeSeparators(e.path));
            a->setData(e.path);
            connect(a, &QAction::triggered, this, &MainWindow::openRecentFile);
        }
    }

    recentFilesMenu_->addSeparator();
    if(actionSetRecentLimit_) recentFilesMenu_->addAction(actionSetRecentLimit_);
    if(actionClearRecentFiles_) recentFilesMenu_->addAction(actionClearRecentFiles_);
}

void MainWindow::loadUiState() {
    QSettings s;
    setRecentFilesMax(s.value("recent/max", kDefaultRecentMax).toInt());

    const QStringList stored = s.value("recent/files").toStringList();
    QList<RecentFileEntry> loaded;
    loaded.reserve(stored.size());
    auto push = [&](RecentFileKind kind, const QString &path) {
        const QString p = QDir::fromNativeSeparators(path.trimmed());
        if(p.isEmpty()) return;
        for(const auto &ex : loaded) {
            if(ex.path.compare(p, Qt::CaseInsensitive) == 0) return;
        }
        loaded.push_back({kind, p});
    };

    if(!stored.isEmpty()) {
        for(const auto &line : stored) {
            const QString t = line.trimmed();
            if(t.startsWith("vtf:", Qt::CaseInsensitive)) {
                push(RecentFileKind::Vtf, t.mid(4));
            } else if(t.startsWith("vmt:", Qt::CaseInsensitive)) {
                push(RecentFileKind::Vmt, t.mid(4));
            } else if(t.endsWith(".vtf", Qt::CaseInsensitive)) {
                push(RecentFileKind::Vtf, t);
            } else if(t.endsWith(".vmt", Qt::CaseInsensitive)) {
                push(RecentFileKind::Vmt, t);
            }
        }
    } else {
        // Migration: older builds stored separate lists without shared ordering.
        const QStringList vtf = s.value("recent/vtf").toStringList();
        const QStringList vmt = s.value("recent/vmt").toStringList();
        for(const auto &p : vtf) push(RecentFileKind::Vtf, p);
        for(const auto &p : vmt) push(RecentFileKind::Vmt, p);
    }

    recentFiles_ = loaded;
    while(recentFiles_.size() > recentMax_) recentFiles_.removeLast();
    const QByteArray geom = s.value("ui/geometry").toByteArray();
    const bool restoredGeom = !geom.isEmpty() && restoreGeometry(geom);
    const QByteArray state = s.value("ui/state").toByteArray();
    (void)(!state.isEmpty() && restoreState(state));
    if(!restoredGeom) {
        resize(1024, 700);
    }

    // Clamp to the current screen's available geometry so stored layouts from larger displays
    // don't spawn off-screen or larger than a 1366x768-ish workspace.
    if(QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        if(!avail.isNull()) {
            const int maxW = std::max(640, avail.width() - 40);
            const int maxH = std::max(480, avail.height() - 40);
            if(width() > maxW || height() > maxH) {
                resize(std::min(width(), maxW), std::min(height(), maxH));
            }

            const QRect g = geometry();
            const int maxX = avail.x() + avail.width() - g.width();
            const int maxY = avail.y() + avail.height() - g.height();
            move(std::clamp(g.x(), avail.x(), maxX), std::clamp(g.y(), avail.y(), maxY));
        }
    }
    rebuildRecentMenus();
}

void MainWindow::saveUiState() const {
    QSettings s;
    s.setValue("recent/max", recentMax_);
    QStringList mixed;
    mixed.reserve(recentFiles_.size());
    QStringList vtf;
    QStringList vmt;
    for(const auto &e : recentFiles_) {
        const QString kind = (e.kind == RecentFileKind::Vtf) ? "vtf:" : "vmt:";
        mixed.push_back(kind + e.path);
        if(e.kind == RecentFileKind::Vtf) vtf.push_back(e.path);
        if(e.kind == RecentFileKind::Vmt) vmt.push_back(e.path);
    }
    s.setValue("recent/files", mixed);
    // Keep legacy keys for compatibility with older builds / external tooling.
    s.setValue("recent/vtf", vtf);
    s.setValue("recent/vmt", vmt);
    s.setValue("ui/geometry", saveGeometry());
    s.setValue("ui/state", saveState());
}

void MainWindow::setRecentFilesMax(int maxFiles) {
    maxFiles = std::clamp(maxFiles, 1, kMaxRecentMax);
    recentMax_ = maxFiles;
    while(recentFiles_.size() > recentMax_) recentFiles_.removeLast();
    rebuildRecentMenus();
}

void MainWindow::addRecentFile(RecentFileKind kind, const QString &path) {
    const QString p = QDir::fromNativeSeparators(path.trimmed());
    if(p.isEmpty()) return;

    for(int i = 0; i < recentFiles_.size(); ++i) {
        if(recentFiles_[i].path.compare(p, Qt::CaseInsensitive) == 0) {
            recentFiles_.removeAt(i);
            break;
        }
    }
    recentFiles_.prepend({kind, p});
    while(recentFiles_.size() > recentMax_) recentFiles_.removeLast();
    rebuildRecentMenus();
    saveUiState();
}

bool MainWindow::maybeSaveVtf() {
    if(imageId_ == kInvalidImageId) return true;
    if(!vtfDirty_) return true;

    const auto result = QMessageBox::question(this, "Unsaved VTF changes", "Save changes to this VTF?",
                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(result == QMessageBox::Cancel) return false;
    if(result == QMessageBox::No) {
        vtfDirty_ = false;
        actionSaveVtf_->setEnabled(false);
        return true;
    }
    saveVtf();
    return !vtfDirty_;
}

bool MainWindow::openVtf(const QString &path) {
    if(path.isEmpty()) return false;
    if(!maybeSaveVtf()) return false;

    vlUInt newImageId = 0;
    if(!vlCreateImage(&newImageId)) {
        showErrorPopup("VTFLib error", QString("Failed to create image handle: %1").arg(vlGetLastError()));
        return false;
    }
    if(!vlBindImage(newImageId)) {
        vlDeleteImage(newImageId);
        showErrorPopup("VTFLib error", QString("Failed to bind image handle: %1").arg(vlGetLastError()));
        return false;
    }

    const QByteArray encoded = QFile::encodeName(path);
    if(!vlImageLoad(encoded.constData(), vlFalse)) {
        const QString err = QString("Failed to load VTF: %1").arg(vlGetLastError());
        vlDeleteImage(newImageId);
        showErrorPopup("Open failed", err);
        return false;
    }

    if(imageId_ != kInvalidImageId) {
        vlDeleteImage(imageId_);
    }
    imageId_ = newImageId;
    currentPath_ = path;
    frame_ = face_ = slice_ = mip_ = 0;
    isPlaying_ = false;
    playbackTimer_->stop();
    actionPlayPause_->setText("&Play");
    vtfDirty_ = false;
    actionSaveVtf_->setEnabled(false);

    updateUiFromBoundVtf();
    addRecentVtf(path);
    {
        QSettings s;
        s.setValue("paths/open_vtf", QFileInfo(path).absolutePath());
    }
    setStatusInfo(QString("Opened %1").arg(QFileInfo(path).fileName()));
    return true;
}

bool MainWindow::openPath(const QString &path) {
    if(path.isEmpty()) return false;
    if(path.endsWith(".vtf", Qt::CaseInsensitive)) return openVtf(path);
    if(path.endsWith(".vmt", Qt::CaseInsensitive)) return openVmtFromPath(path);

    QImageReader reader(path);
    if(reader.canRead()) return importImageFromPath(path);

    QMessageBox::information(this, "Unsupported file", "This file type is not supported.");
    return false;
}

void MainWindow::updateUiFromBoundVtf() {
    if(imageId_ == kInvalidImageId) {
        closeVtf();
        return;
    }
    vlBindImage(imageId_);

    infoPath_->setText(currentPath_);

    const auto major = vlImageGetMajorVersion();
    const auto minor = vlImageGetMinorVersion();
    infoVersion_->setText(QString("%1.%2").arg(major).arg(minor));

    const auto width = vlImageGetWidth();
    const auto height = vlImageGetHeight();
    const auto depth = vlImageGetDepth();
    infoDimensions_->setText(QString("%1 × %2 × %3").arg(width).arg(height).arg(depth));

    const auto fmt = vlImageGetFormat();
    infoFormat_->setText(vtfImageFormatName(fmt));
    const bool hdrCapable = vtfFormatIsHdr(fmt);
    if(actionHdrTonemap_) actionHdrTonemap_->setEnabled(hdrCapable);
    if(hdrTonemapCombo_) hdrTonemapCombo_->setEnabled(hdrCapable && hdrTonemapEnabled_ && !showThumbnail_);

    infoMipmaps_->setText(QString::number(vlImageGetMipmapCount()));
    infoFrames_->setText(QString::number(vlImageGetFrameCount()));
    infoFaces_->setText(QString::number(vlImageGetFaceCount()));
    infoFlags_->setText(QString("0x%1").arg(static_cast<qulonglong>(vlImageGetFlags()), 8, 16, QChar('0')).toUpper());
    infoStartFrame_->setText(QString::number(vlImageGetStartFrame()));
    infoBumpScale_->setText(QString::number(static_cast<double>(vlImageGetBumpmapScale()), 'g', 6));
    {
        vlSingle x = 0, y = 0, z = 0;
        vlImageGetReflectivity(&x, &y, &z);
        infoReflectivity_->setText(QString("%1, %2, %3")
                                       .arg(static_cast<double>(x), 0, 'g', 6)
                                       .arg(static_cast<double>(y), 0, 'g', 6)
                                       .arg(static_cast<double>(z), 0, 'g', 6));
    }

    updateSelectionLimits();
    // Render preview, but delay warning dialogs until we've had a chance to retry on the next tick.
    renderSelection(false);
    rebuildResourcesTree();
    if(statusSelection_) {
        QString faceLabel;
        if(vlImageGetFaceCount() >= 6) {
            const QString name = cubeFaceName(face_);
            if(!name.isEmpty()) faceLabel = QString(" (%1)").arg(name);
        }
        statusSelection_->setText(QString("F%1  Face%2%3  S%4  M%5").arg(frame_).arg(face_).arg(faceLabel).arg(slice_).arg(mip_));
    }

    actionReloadVtf_->setEnabled(!currentPath_.isEmpty());
    actionOpenContainingFolder_->setEnabled(!currentPath_.isEmpty());
    actionCloseVtf_->setEnabled(true);
    actionOpenMatchingVmt_->setEnabled(!currentPath_.isEmpty());
    actionExportPng_->setEnabled(true);
    actionExportThumbnailPng_->setEnabled(vlImageGetHasThumbnail());
    actionExportAs_->setEnabled(true);
    actionExportAllFrames_->setEnabled(true);
    actionExportAllFaces_->setEnabled(vlImageGetFaceCount() > 1);
    actionExportAllSlices_->setEnabled(vlImageGetDepth() > 1);
    actionExportAllMipmaps_->setEnabled(vlImageGetMipmapCount() > 1);
    actionExportAll_->setEnabled(vlImageGetFrameCount() > 1 || vlImageGetFaceCount() > 1 || vlImageGetDepth() > 1);
    actionSaveAsVtf_->setEnabled(true);
    actionSaveVtf_->setEnabled(vtfDirty_);
    actionZoomIn_->setEnabled(true);
    actionZoomOut_->setEnabled(true);
    actionZoomReset_->setEnabled(true);
    actionZoomFit_->setEnabled(true);
    actionShowThumbnail_->setEnabled(vlImageGetHasThumbnail());
    actionPlayPause_->setEnabled(vlImageGetFrameCount() > 1);
    actionEditVtfFlags_->setEnabled(true);
    actionVtfProperties_->setEnabled(true);
    actionVtfResources_->setEnabled(vlImageGetSupportsResources());
    actionCreateVmtFromVtf_->setEnabled(!currentPath_.isEmpty());
    if(zoomCombo_) zoomCombo_->setEnabled(true);
    actionGenNormalFrame_->setEnabled(true);
    actionGenNormalAll_->setEnabled(true);
    {
        const auto faces = vlImageGetFaceCount();
        actionGenSphereMap_->setEnabled(faces == 6 || faces == 7);
        actionCubemapPreview_->setEnabled(faces >= 6);
        if(faces < 6) actionCubemapPreview_->setChecked(false);
    }
    channelCombo_->setEnabled(true);
    backgroundCombo_->setEnabled(true);
    frameMsSpin_->setEnabled(true);

    updateCubemapPreview();
    updateWindowTitle();
    schedulePreviewRefresh();
    if(imageLabel_) imageLabel_->update();
    if(scrollArea_ && scrollArea_->viewport()) scrollArea_->viewport()->update();
}

void MainWindow::rebuildResourcesTree() {
    if(!resourcesTree_) return;
    resourcesTree_->clear();

    if(imageId_ == kInvalidImageId) {
        if(resourcesDock_) resourcesDock_->hide();
        return;
    }
    vlBindImage(imageId_);

    if(!vlImageGetSupportsResources()) {
        auto *item = new QTreeWidgetItem(resourcesTree_, {"Resources", "Not supported (requires VTF v7.3+)"});
        item->setExpanded(true);
        if(resourcesDock_) resourcesDock_->show();
        return;
    }

    const vlUInt count = vlImageGetResourceCount();
    auto *root = new QTreeWidgetItem(resourcesTree_, {QString("Resources (%1)").arg(count), {}});
    root->setExpanded(true);

    for(vlUInt i = 0; i < count; ++i) {
        const vlUInt type = vlImageGetResourceType(i);
        vlUInt size = 0;
        void *data = vlImageGetResourceData(type, &size);

        auto *resItem = new QTreeWidgetItem(root, {resourceTypeNameForUi(type), {}});
        resItem->setData(0, Qt::UserRole, static_cast<uint>(type));
        resItem->setToolTip(0, QString("Type: 0x%1\nSize: %2 bytes")
                                   .arg(static_cast<qulonglong>(type), 8, 16, QChar('0'))
                                   .arg(size)
                                   .toUpper());

        switch(type) {
            case VTF_RSRC_CRC: {
                if(data && size == sizeof(vlUInt)) {
                    const vlUInt crc = *reinterpret_cast<const vlUInt *>(data);
                    new QTreeWidgetItem(resItem, {"Checksum", QString("0x%1").arg(static_cast<qulonglong>(crc), 8, 16, QChar('0')).toUpper()});
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
            case VTF_RSRC_TEXTURE_LOD_SETTINGS: {
                if(data && size == sizeof(vlUInt)) {
                    SVTFTextureLODControlResource lod{};
                    std::memcpy(&lod, data, sizeof(lod));
                    new QTreeWidgetItem(resItem, {"Clamp U", QString::number(lod.ResolutionClampU)});
                    new QTreeWidgetItem(resItem, {"Clamp V", QString::number(lod.ResolutionClampV)});
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
            case VTF_RSRC_TEXTURE_SETTINGS_EX: {
                if(data && size == sizeof(vlUInt)) {
                    SVTFTextureSettingsExResource tso{};
                    std::memcpy(&tso, data, sizeof(tso));
                    new QTreeWidgetItem(resItem, {"Byte 0", QString::number(tso.Flags0)});
                    new QTreeWidgetItem(resItem, {"Byte 1", QString::number(tso.Flags1)});
                    new QTreeWidgetItem(resItem, {"Byte 2", QString::number(tso.Flags2)});
                    new QTreeWidgetItem(resItem, {"Byte 3", QString::number(tso.Flags3)});
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
            case VTF_RSRC_KEY_VALUE_DATA: {
                if(data && size) {
                    VTFLib::CVMTFile vmt;
                    if(vmt.Load(data, size)) {
                        resItem->setText(0, QString::fromUtf8(vmt.GetRoot()->GetName()));

                        std::function<void(QTreeWidgetItem *, VTFLib::Nodes::CVMTNode *)> addNode;
                        addNode = [&](QTreeWidgetItem *parent, VTFLib::Nodes::CVMTNode *node) {
                            if(!node) return;
                            using namespace VTFLib::Nodes;
                            const QString name = QString::fromUtf8(node->GetName());
                            switch(node->GetType()) {
                                case NODE_TYPE_GROUP: {
                                    auto *g = static_cast<CVMTGroupNode *>(node);
                                    auto *item = new QTreeWidgetItem(parent, {name, {}});
                                    const vlUInt n = g->GetNodeCount();
                                    for(vlUInt j = 0; j < n; ++j) {
                                        addNode(item, g->GetNode(j));
                                    }
                                    break;
                                }
                                case NODE_TYPE_STRING: {
                                    auto *s = static_cast<CVMTStringNode *>(node);
                                    new QTreeWidgetItem(parent, {name, QString::fromUtf8(s->GetValue())});
                                    break;
                                }
                                case NODE_TYPE_INTEGER: {
                                    auto *n = static_cast<CVMTIntegerNode *>(node);
                                    new QTreeWidgetItem(parent, {name, QString::number(n->GetValue())});
                                    break;
                                }
                                case NODE_TYPE_SINGLE: {
                                    auto *f = static_cast<CVMTSingleNode *>(node);
                                    new QTreeWidgetItem(parent, {name, QString::number(static_cast<double>(f->GetValue()), 'g', 9)});
                                    break;
                                }
                                default:
                                    new QTreeWidgetItem(parent, {name, {}});
                                    break;
                            }
                        };

                        auto *rootGroup = static_cast<VTFLib::Nodes::CVMTGroupNode *>(vmt.GetRoot());
                        const vlUInt n = rootGroup->GetNodeCount();
                        for(vlUInt j = 0; j < n; ++j) {
                            addNode(resItem, rootGroup->GetNode(j));
                        }
                    } else {
                        new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                    }
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
            case VTF_RSRC_SHEET: {
                if(data && size) {
                    QByteArray bytes(reinterpret_cast<const char *>(data), static_cast<int>(size));
                    VtfSheet sheet;
                    QString err;
                    if(VtfSheet::parse(bytes, &sheet, &err)) {
                        new QTreeWidgetItem(resItem, {"Version", QString::number(static_cast<int>(sheet.version))});
                        new QTreeWidgetItem(resItem, {"Sequence count", QString::number(sheet.sequences.size())});
                        for(auto it = sheet.sequences.begin(); it != sheet.sequences.end(); ++it) {
                            const auto &seq = it.value();
                            auto *seqItem = new QTreeWidgetItem(resItem, {QString("Sequence %1").arg(seq.seqNum), {}});
                            seqItem->setData(0, Qt::UserRole, static_cast<uint>(type));
                            new QTreeWidgetItem(seqItem, {"Clamp", seq.clamp ? "true" : "false"});
                            new QTreeWidgetItem(seqItem, {"Total time", QString::number(seq.totalTime, 'g', 9)});
                            new QTreeWidgetItem(seqItem, {"Frame count", QString::number(seq.frames.size())});
                            for(int fi = 0; fi < seq.frames.size(); ++fi) {
                                const auto &fr = seq.frames[fi];
                                auto *frItem = new QTreeWidgetItem(seqItem, {QString("Frame %1").arg(fi), {}});
                                new QTreeWidgetItem(frItem, {"Duration", QString::number(fr.duration, 'g', 9)});
                                new QTreeWidgetItem(frItem, {"A", QString("%1, %2, %3, %4").arg(fr.a.left, 0, 'g', 6).arg(fr.a.top, 0, 'g', 6).arg(fr.a.right, 0, 'g', 6).arg(fr.a.bottom, 0, 'g', 6)});
                                if(sheet.version == 1) {
                                    new QTreeWidgetItem(frItem, {"B", QString("%1, %2, %3, %4").arg(fr.b.left, 0, 'g', 6).arg(fr.b.top, 0, 'g', 6).arg(fr.b.right, 0, 'g', 6).arg(fr.b.bottom, 0, 'g', 6)});
                                    new QTreeWidgetItem(frItem, {"C", QString("%1, %2, %3, %4").arg(fr.c.left, 0, 'g', 6).arg(fr.c.top, 0, 'g', 6).arg(fr.c.right, 0, 'g', 6).arg(fr.c.bottom, 0, 'g', 6)});
                                    new QTreeWidgetItem(frItem, {"D", QString("%1, %2, %3, %4").arg(fr.d.left, 0, 'g', 6).arg(fr.d.top, 0, 'g', 6).arg(fr.d.right, 0, 'g', 6).arg(fr.d.bottom, 0, 'g', 6)});
                                }
                            }
                            seqItem->setExpanded(false);
                        }
                    } else {
                        new QTreeWidgetItem(resItem, {"Decode error", err});
                        new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                    }
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
            default: {
                if(data && size == sizeof(vlUInt)) {
                    const vlUInt v = *reinterpret_cast<const vlUInt *>(data);
                    new QTreeWidgetItem(resItem, {"Data", QString("0x%1").arg(static_cast<qulonglong>(v), 8, 16, QChar('0')).toUpper()});
                } else {
                    new QTreeWidgetItem(resItem, {"Size", QString("%1 B").arg(size)});
                }
                break;
            }
        }

        resItem->setExpanded(type == VTF_RSRC_KEY_VALUE_DATA);
    }

    resourcesTree_->resizeColumnToContents(0);
    if(resourcesDock_) resourcesDock_->show();
}

QSize MainWindow::displayedImageSize() const {
    if(!viewImage_.isNull()) return viewImage_.size();
    return rawRgbaImage_.size();
}

void MainWindow::setPreviewPlaceholderText(const QString &text) {
    if(!imageLabel_ || !scrollArea_) return;

    scrollArea_->setWidgetResizable(true);
    imageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel_->setScaledContents(false);
    imageLabel_->setWordWrap(true);
    imageLabel_->setAlignment(Qt::AlignCenter);

    QFont f = imageLabel_->font();
    const qreal basePt = (f.pointSizeF() > 0.0) ? f.pointSizeF() : 11.0;
    f.setPointSizeF(std::max<qreal>(basePt, 14.0));
    imageLabel_->setFont(f);
    imageLabel_->setStyleSheet({});
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    imageLabel_->setForegroundRole(QPalette::PlaceholderText);
#else
    imageLabel_->setForegroundRole(QPalette::WindowText);
#endif

    imageLabel_->setPixmap(QPixmap());
    imageLabel_->setText(text);
}

void MainWindow::setViewImage(const QImage &rawRgba) {
    rawRgbaImage_ = rawRgba;
    viewImage_ = {};

    if(rawRgbaImage_.isNull()) {
        setPreviewPlaceholderText("No preview");
        actionCopyImage_->setEnabled(false);
        updateZoomUi();
        return;
    }

    if(scrollArea_) scrollArea_->setWidgetResizable(false);
    imageLabel_->setStyleSheet({});
    imageLabel_->setForegroundRole(QPalette::WindowText);
    imageLabel_->setWordWrap(false);
    imageLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel_->setScaledContents(true);
    imageLabel_->clear(); // ensure placeholder text doesn't persist behind the pixmap

    const auto channelMode = static_cast<ChannelMode>(channelCombo_->currentData().toInt());
    const auto bgMode = static_cast<BackgroundMode>(backgroundCombo_->currentData().toInt());

    QImage src = rawRgbaImage_;

    auto withAlphaFromSource = [](const QImage &rgb, const QImage &alphaSource) -> QImage {
        if(rgb.isNull()) return {};
        QImage out = rgb;
        if(out.format() != QImage::Format_ARGB32) out = out.convertToFormat(QImage::Format_ARGB32);
        QImage a = alphaSource;
        if(a.format() != QImage::Format_ARGB32) a = a.convertToFormat(QImage::Format_ARGB32);
        if(out.size() != a.size()) return out;

        for(int y = 0; y < out.height(); ++y) {
            const auto *aRow = reinterpret_cast<const QRgb *>(a.constScanLine(y));
            auto *row = reinterpret_cast<QRgb *>(out.scanLine(y));
            for(int x = 0; x < out.width(); ++x) {
                row[x] = qRgba(qRed(row[x]), qGreen(row[x]), qBlue(row[x]), qAlpha(aRow[x]));
            }
        }
        return out;
    };

    QImage view = channelView(src, channelMode);

    // Windows-style "Mask": toggle alpha compositing over the selected background.
    if(maskMode_ && channelMode != ChannelMode::A) {
        view = withAlphaFromSource(view, src);
        view = compositeOverBackground(view, bgMode, customBackgroundColor_);
    } else {
        // When not compositing, ignore alpha for RGBA/RGB views (matches Windows behavior).
        if(channelMode == ChannelMode::RGBA || channelMode == ChannelMode::RGB) {
            if(view.format() != QImage::Format_ARGB32) view = view.convertToFormat(QImage::Format_ARGB32);
            for(int y = 0; y < view.height(); ++y) {
                auto *row = reinterpret_cast<QRgb *>(view.scanLine(y));
                for(int x = 0; x < view.width(); ++x) {
                    row[x] = qRgba(qRed(row[x]), qGreen(row[x]), qBlue(row[x]), 255);
                }
            }
        }
    }

    // Handle Tile mode
    if(tileMode_ && !view.isNull()) {
        QImage tiled(view.width() * 2, view.height() * 2, view.format());
        QPainter painter(&tiled);
        painter.drawImage(0, 0, view);
        painter.drawImage(view.width(), 0, view);
        painter.drawImage(0, view.height(), view);
        painter.drawImage(view.width(), view.height(), view);
        view = std::move(tiled);
    }

    imageLabel_->setText({});
    viewImage_ = view;
    imageLabel_->setPixmap(QPixmap::fromImage(viewImage_));
    actionCopyImage_->setEnabled(true);

    if(fitToWindow_) {
        updateFitZoom();
    } else {
        imageLabel_->resize((QSizeF(displayedImageSize()) * zoom_).toSize());
        updateZoomUi();
    }
}

void MainWindow::setZoom(double zoom, const QPoint &viewportAnchor) {
    if(rawRgbaImage_.isNull()) return;
    if(!scrollArea_ || !imageLabel_) return;

    zoom = std::max(kMinZoom, std::min(zoom, kMaxZoom));
    if(std::abs(zoom - zoom_) < 1e-12) {
        updateZoomUi();
        return;
    }

    const QPoint anchor = viewportAnchor.isNull()
                              ? QPoint(scrollArea_->viewport()->width() / 2, scrollArea_->viewport()->height() / 2)
                              : viewportAnchor;

    QScrollBar *h = scrollArea_->horizontalScrollBar();
    QScrollBar *v = scrollArea_->verticalScrollBar();

    const double scale = zoom / zoom_;
    const double oldX = static_cast<double>(h->value() + anchor.x());
    const double oldY = static_cast<double>(v->value() + anchor.y());

    zoom_ = zoom;
    imageLabel_->resize((QSizeF(displayedImageSize()) * zoom_).toSize());

    h->setValue(static_cast<int>(oldX * scale - anchor.x()));
    v->setValue(static_cast<int>(oldY * scale - anchor.y()));

    updateZoomUi();
}

void MainWindow::setZoom(double zoom) {
    if(!scrollArea_) return;
    setZoom(zoom, QPoint(scrollArea_->viewport()->width() / 2, scrollArea_->viewport()->height() / 2));
}

void MainWindow::updateFitZoom() {
    if(!fitToWindow_) return;
    if(rawRgbaImage_.isNull()) return;
    if(!scrollArea_ || !imageLabel_) return;

    const QSize vp = scrollArea_->viewport()->size();
    const QSize img = displayedImageSize();
    if(vp.width() <= 0 || vp.height() <= 0 || img.width() <= 0 || img.height() <= 0) return;

    const double fit = std::min(static_cast<double>(vp.width()) / static_cast<double>(img.width()),
                                static_cast<double>(vp.height()) / static_cast<double>(img.height()));

    setZoom(std::max(kMinZoom, std::min(fit, kMaxZoom)));
}

void MainWindow::updateZoomUi() {
    if(!statusZoom_ || !zoomCombo_) return;
    if(rawRgbaImage_.isNull()) {
        statusZoom_->setText("Zoom: -");
        zoomCombo_->blockSignals(true);
        zoomCombo_->setCurrentText("100%");
        zoomCombo_->blockSignals(false);
        return;
    }

    const int pct = static_cast<int>(std::lround(zoom_ * 100.0));
    statusZoom_->setText(QString("Zoom: %1%2").arg(pct).arg(fitToWindow_ ? " (Fit)" : ""));
    zoomCombo_->blockSignals(true);
    zoomCombo_->setCurrentText(QString("%1%").arg(pct));
    zoomCombo_->blockSignals(false);
}

void MainWindow::updateWindowTitle() {
    QString title = "VTFEdit (Qt)";

    if(!currentPath_.isEmpty()) {
        title += " — ";
        title += QFileInfo(currentPath_).fileName();
        if(vtfDirty_) title += '*';
    }

    if(vmtEditor_ && (!currentVmtPath_.isEmpty() || !vmtEditor_->toPlainText().trimmed().isEmpty())) {
        title += "  |  VMT: ";
        title += currentVmtPath_.isEmpty() ? "(unsaved)" : QFileInfo(currentVmtPath_).fileName();
        if(vmtEditor_->document()->isModified()) title += '*';
    }

    setWindowTitle(title);
}

void MainWindow::updateVmtStatusText() {
    if(!statusVmt_) return;
    if(!vmtEditor_) {
        statusVmt_->setText("VMT: -");
        return;
    }
    if(!vmtEditor_->document()->isModified() && vmtEditor_->toPlainText().trimmed().isEmpty()) {
        statusVmt_->setText("VMT: -");
        return;
    }
    if(!liveValidateVmt_) {
        statusVmt_->setText("VMT: (no live check)");
        return;
    }
    // liveValidateVmtTick updates with OK/Invalid; this is just a fallback.
    statusVmt_->setText("VMT: checking…");
}

void MainWindow::cubemapPreviewToggled(bool enabled) {
    if(!cubemapDock_) return;
    if(!enabled) {
        cubemapDock_->hide();
        return;
    }
    if(imageId_ == kInvalidImageId) {
        actionCubemapPreview_->setChecked(false);
        return;
    }
    vlBindImage(imageId_);
    if(vlImageGetFaceCount() < 6) {
        QMessageBox::information(this, "Not a cubemap", "Cubemap preview requires a VTF with 6 (or more) faces.");
        actionCubemapPreview_->setChecked(false);
        return;
    }
    cubemapDock_->show();
    updateCubemapPreview();
}

void MainWindow::updateCubemapPreview() {
    if(!cubemapDock_) return;
    // Only refresh the dock contents here; visibility is handled by cubemapPreviewToggled() and closeVtf().
    if(!cubemapDock_->isVisible()) return;
    if(imageId_ == kInvalidImageId) return;
    if(!vlBindImage(imageId_)) return;

    const vlUInt faces = vlImageGetFaceCount();
    if(faces < 6) return;

    const auto channelMode = static_cast<ChannelMode>(channelCombo_->currentData().toInt());
    const auto bgMode = static_cast<BackgroundMode>(backgroundCombo_->currentData().toInt());
    const bool hdr = hdrTonemapEnabled_ && vtfFormatIsHdr(vlImageGetFormat());
    if(cubemapExposure_) cubemapExposure_->setEnabled(hdr && hdrTonemap_ != HdrTonemap::VTFLib);

    auto makeFacePreview = [&](unsigned int faceIdx, const QSize &target) -> QImage {
        QString error;
        QImage rgba;
        const auto fmt = vlImageGetFormat();
        if(hdrTonemapEnabled_ && vtfFormatIsHdr(fmt)) {
            if(hdrTonemap_ == HdrTonemap::VTFLib) {
                vlSetFloat(VTFLIB_FP16_HDR_EXPOSURE, static_cast<vlSingle>(hdrExposure_));
                rgba = vtfSelectionToRgba(frame_, faceIdx, slice_, mip_, &error);
            } else {
                rgba = vtfSelectionToHdrTonemappedRgba(frame_, faceIdx, slice_, mip_, cubemapHdrExposure_, hdrTonemap_, &error);
            }
        } else {
            rgba = vtfSelectionToRgba(frame_, faceIdx, slice_, mip_, &error);
        }
        if(rgba.isNull()) return {};
        if(!target.isEmpty()) {
            rgba = rgba.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        auto withAlphaFromSource = [](const QImage &rgb, const QImage &alphaSource) -> QImage {
            if(rgb.isNull()) return {};
            QImage out = rgb;
            if(out.format() != QImage::Format_ARGB32) out = out.convertToFormat(QImage::Format_ARGB32);
            QImage a = alphaSource;
            if(a.format() != QImage::Format_ARGB32) a = a.convertToFormat(QImage::Format_ARGB32);
            if(out.size() != a.size()) return out;

            for(int y = 0; y < out.height(); ++y) {
                const auto *aRow = reinterpret_cast<const QRgb *>(a.constScanLine(y));
                auto *row = reinterpret_cast<QRgb *>(out.scanLine(y));
                for(int x = 0; x < out.width(); ++x) {
                    row[x] = qRgba(qRed(row[x]), qGreen(row[x]), qBlue(row[x]), qAlpha(aRow[x]));
                }
            }
            return out;
        };

        QImage view = channelView(rgba, channelMode);
        if(maskMode_ && channelMode != ChannelMode::A) {
            view = withAlphaFromSource(view, rgba);
            view = compositeOverBackground(view, bgMode, customBackgroundColor_);
        } else {
            if(channelMode == ChannelMode::RGBA || channelMode == ChannelMode::RGB) {
                if(view.format() != QImage::Format_ARGB32) view = view.convertToFormat(QImage::Format_ARGB32);
                for(int y = 0; y < view.height(); ++y) {
                    auto *row = reinterpret_cast<QRgb *>(view.scanLine(y));
                    for(int x = 0; x < view.width(); ++x) {
                        row[x] = qRgba(qRed(row[x]), qGreen(row[x]), qBlue(row[x]), 255);
                    }
                }
            }
        }
        return view;
    };

    auto setButtonImage = [&](QToolButton *btn, int faceIdx) {
        if(!btn) return;
        if(faceIdx >= static_cast<int>(faces)) {
            btn->setEnabled(false);
            btn->setIcon(QIcon());
            btn->setToolTip({});
            btn->setChecked(false);
            return;
        }

        const QSize target = btn->iconSize();
        QImage view = makeFacePreview(static_cast<unsigned int>(faceIdx), target);
        if(view.isNull()) {
            btn->setEnabled(false);
            btn->setIcon(QIcon());
            btn->setToolTip("Preview unavailable.");
            btn->setChecked(false);
            return;
        }

        btn->setEnabled(true);
        btn->setIcon(QIcon(QPixmap::fromImage(view)));
        btn->setChecked(static_cast<unsigned int>(faceIdx) == face_);
        btn->setToolTip(QString("Face %1  Frame %2  Slice %3  Mip %4").arg(faceIdx).arg(frame_).arg(slice_).arg(mip_));
    };

    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_RIGHT], CUBEMAP_FACE_RIGHT);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_LEFT], CUBEMAP_FACE_LEFT);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_BACK], CUBEMAP_FACE_BACK);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_FRONT], CUBEMAP_FACE_FRONT);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_UP], CUBEMAP_FACE_UP);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_DOWN], CUBEMAP_FACE_DOWN);
    setButtonImage(cubeFaceButtons_[CUBEMAP_FACE_SPHERE_MAP], CUBEMAP_FACE_SPHERE_MAP);

    if(cubeFaceButtons_[CUBEMAP_FACE_SPHERE_MAP]) {
        cubeFaceButtons_[CUBEMAP_FACE_SPHERE_MAP]->setVisible(faces >= 7);
    }
}

void MainWindow::findInVmt() {
    if(!vmtEditor_) return;

    FindReplaceParams p;
    p.findText = vmtFindText_;
    p.caseSensitive = (vmtFindFlags_ & QTextDocument::FindCaseSensitively) != 0;
    p.wholeWord = (vmtFindFlags_ & QTextDocument::FindWholeWords) != 0;
    if(!showFindDialog(this, &p, false)) return;
    if(p.findText.isEmpty()) return;

    vmtFindText_ = p.findText;
    vmtFindFlags_ = {};
    if(p.caseSensitive) vmtFindFlags_ |= QTextDocument::FindCaseSensitively;
    if(p.wholeWord) vmtFindFlags_ |= QTextDocument::FindWholeWords;
    findNextInVmt();
}

void MainWindow::findNextInVmt() {
    if(!vmtEditor_) return;
    if(vmtFindText_.isEmpty()) {
        findInVmt();
        return;
    }

    const bool found = vmtEditor_->find(vmtFindText_, vmtFindFlags_);
    if(found) return;

    // Wrap-around.
    QTextCursor c = vmtEditor_->textCursor();
    c.movePosition(QTextCursor::Start);
    vmtEditor_->setTextCursor(c);
    if(vmtEditor_->find(vmtFindText_, vmtFindFlags_)) return;

    QMessageBox::information(this, "Find", "No matches found.");
}

void MainWindow::replaceInVmt() {
    if(!vmtEditor_) return;

    FindReplaceParams p;
    p.findText = vmtFindText_;
    p.caseSensitive = (vmtFindFlags_ & QTextDocument::FindCaseSensitively) != 0;
    p.wholeWord = (vmtFindFlags_ & QTextDocument::FindWholeWords) != 0;
    if(!showFindDialog(this, &p, true)) return;
    if(p.findText.isEmpty()) return;

    vmtFindText_ = p.findText;
    vmtFindFlags_ = {};
    if(p.caseSensitive) vmtFindFlags_ |= QTextDocument::FindCaseSensitively;
    if(p.wholeWord) vmtFindFlags_ |= QTextDocument::FindWholeWords;

    const QMessageBox::StandardButton mode = QMessageBox::question(
        this,
        "Replace",
        "Replace current match only?\n\nChoose No to replace all matches.",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    if(mode == QMessageBox::Cancel) return;

    if(mode == QMessageBox::No) {
        int count = 0;
        QTextCursor start = vmtEditor_->textCursor();
        vmtEditor_->moveCursor(QTextCursor::Start);
        while(vmtEditor_->find(vmtFindText_, vmtFindFlags_)) {
            QTextCursor tc = vmtEditor_->textCursor();
            if(!tc.hasSelection()) break;
            tc.insertText(p.replaceText);
            ++count;
        }
        vmtEditor_->setTextCursor(start);
        setStatusInfo(QString("Replaced %1 matches").arg(count));
        return;
    }

    QTextCursor tc = vmtEditor_->textCursor();
    if(tc.hasSelection()) {
        const QString sel = tc.selectedText();
        const QString needle = vmtFindText_;
        const bool matches = (vmtFindFlags_ & QTextDocument::FindCaseSensitively) ? (sel == needle)
                                                                                 : (sel.compare(needle, Qt::CaseInsensitive) == 0);
        if(matches) tc.insertText(p.replaceText);
    }
    findNextInVmt();
}

void MainWindow::goToLineInVmt() {
    if(!vmtEditor_) return;
    const int maxLine = vmtEditor_->document()->blockCount();
    if(maxLine <= 0) return;

    bool ok = false;
    const int line = QInputDialog::getInt(this, "Go to Line", "Line:", 1, 1, maxLine, 1, &ok);
    if(!ok) return;

    QTextBlock block = vmtEditor_->document()->findBlockByLineNumber(line - 1);
    if(!block.isValid()) return;

    QTextCursor c(block);
    vmtEditor_->setTextCursor(c);
    vmtEditor_->setFocus();
}

QString MainWindow::formatVmtText(const QString &text) const {
    // Conservative formatter: only adjusts indentation based on braces, preserving line content.
    QStringList lines = text.split('\n');
    QStringList out;
    out.reserve(lines.size());

    int indent = 0;
    for(QString line : lines) {
        // Preserve blank lines.
        if(line.trimmed().isEmpty()) {
            out.push_back({});
            continue;
        }

        // Keep leading tabs/spaces out, reapply indentation.
        QString trimmed = line.trimmed();

        // De-indent if line starts with '}' (outside strings/comments heuristic).
        if(trimmed.startsWith('}')) indent = std::max(0, indent - 1);

        out.push_back(QString(indent * 4, ' ') + trimmed);

        // Update indent by counting braces outside quotes and before // comments.
        bool inString = false;
        bool escape = false;
        for(int i = 0; i < trimmed.size(); ++i) {
            const QChar ch = trimmed.at(i);
            if(!inString && i + 1 < trimmed.size() && ch == '/' && trimmed.at(i + 1) == '/') break;
            if(escape) {
                escape = false;
                continue;
            }
            if(inString && ch == '\\') {
                escape = true;
                continue;
            }
            if(ch == '"') {
                inString = !inString;
                continue;
            }
            if(inString) continue;
            if(ch == '{') indent++;
            if(ch == '}') indent = std::max(0, indent - 1);
        }
    }
    return out.join('\n');
}

void MainWindow::formatVmt() {
    if(!vmtEditor_) return;
    const QString before = vmtText();
    const QString after = formatVmtText(before);
    if(after == before) return;

    QTextCursor c = vmtEditor_->textCursor();
    c.beginEditBlock();
    c.select(QTextCursor::Document);
    c.insertText(after);
    c.endEditBlock();
    setStatusInfo("Formatted VMT");
}

void MainWindow::liveValidateVmtToggled(bool enabled) {
    liveValidateVmt_ = enabled;
    updateVmtStatusText();
    if(!enabled) return;
    if(vmtValidateTimer_) vmtValidateTimer_->start();
}

void MainWindow::liveValidateVmtTick() {
    if(!liveValidateVmt_) return;
    if(!statusVmt_) return;
    if(!vmtEditor_) {
        statusVmt_->setText("VMT: -");
        return;
    }
    if(vmtEditor_->toPlainText().trimmed().isEmpty()) {
        statusVmt_->setText("VMT: -");
        return;
    }
    QString error;
    if(validateVmtText(vmtText(), PARSE_MODE_LOOSE, &error)) {
        statusVmt_->setText("VMT: OK");
        return;
    }
    // Keep it short in the status bar.
    statusVmt_->setText("VMT: invalid");
}

void MainWindow::openFileDialog() {
    QSettings s;
    const QString startDir = s.value("paths/open_vtf").toString();
    const QString path = QFileDialog::getOpenFileName(this, "Open VTF", startDir, "Valve Texture Format (*.vtf);;All files (*)");
    if(path.isEmpty()) return;
    s.setValue("paths/open_vtf", QFileInfo(path).absolutePath());
    // Defer until after the native file dialog fully closes.
    QTimer::singleShot(0, this, [this, path] { openPath(path); });
}

void MainWindow::openRecentFile() {
    auto *a = qobject_cast<QAction *>(sender());
    if(!a) return;
    const QString path = a->data().toString();
    if(path.isEmpty()) return;
    openPath(path);
}

bool MainWindow::openVmtFromPath(const QString &path) {
    if(path.isEmpty()) return false;
    if(!maybeSaveVmt()) return false;

    QString error;
    const QString text = loadTextFileUtf8(path, &error);
    if(text.isNull()) {
        showErrorPopup("Open failed", QString("Failed to open VMT: %1").arg(error));
        return false;
    }

    currentVmtPath_ = path;
    setVmtText(text, false);
    addRecentVmt(path);
    {
        QSettings s;
        s.setValue("paths/open_vmt", QFileInfo(path).absolutePath());
    }
    setStatusInfo(QString("Opened %1").arg(QFileInfo(path).fileName()));
    return true;
}

void MainWindow::reloadVtf() {
    if(currentPath_.isEmpty()) return;
    openVtf(currentPath_);
}

void MainWindow::openContainingFolder() {
    if(currentPath_.isEmpty()) return;
    const QString dir = QFileInfo(currentPath_).absolutePath();
    if(dir.isEmpty()) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void MainWindow::closeCurrentVtf() {
    if(!maybeSaveVtf()) return;
    closeVtf();
    setStatusInfo("Closed VTF");
}

void MainWindow::closeCurrentVmt() {
    if(!maybeSaveVmt()) return;
    closeVmt();
    setStatusInfo("Closed VMT");
}

void MainWindow::openVmtDialog() {
    if(!maybeSaveVmt()) return;
    QSettings s;
    const QString startDir = s.value("paths/open_vmt").toString();
    const QString path = QFileDialog::getOpenFileName(this, "Open VMT", startDir, "Valve Material Type (*.vmt);;All files (*)");
    if(path.isEmpty()) return;
    s.setValue("paths/open_vmt", QFileInfo(path).absolutePath());
    openVmtFromPath(path);
}

void MainWindow::newVmtDialog() {
    if(!maybeSaveVmt()) return;
    CreateVmtDialog dlg({}, this);
    if(dlg.exec() != QDialog::Accepted) return;
    currentVmtPath_.clear();
    setVmtText(dlg.vmtText(), true);
    setStatusInfo("Created new VMT (unsaved)");
}

void MainWindow::createVmtFromCurrentVtf() {
    if(!maybeSaveVmt()) return;
    bool suggestAlphaTest = false;
    bool suggestTranslucent = false;
    if(imageId_ != kInvalidImageId) {
        vlBindImage(imageId_);
        const vlUInt flags = vlImageGetFlags();
        suggestAlphaTest = (flags & TEXTUREFLAGS_ONEBITALPHA) != 0;
        suggestTranslucent = (flags & TEXTUREFLAGS_EIGHTBITALPHA) != 0;
    }

    CreateVmtDialog dlg(baseTextureFromVtfPathHeuristic(currentPath_), this, suggestAlphaTest, suggestTranslucent);
    if(dlg.exec() != QDialog::Accepted) return;
    currentVmtPath_.clear();
    setVmtText(dlg.vmtText(), true);
    setStatusInfo("Created VMT from current VTF (unsaved)");
}

void MainWindow::openMatchingVmtFromVtf() {
    if(currentPath_.isEmpty()) return;

    QString candidate = currentPath_;
    if(candidate.endsWith(".vtf", Qt::CaseInsensitive)) {
        candidate.chop(4);
        candidate += ".vmt";
    }
    if(QFileInfo::exists(candidate)) {
        openVmtFromPath(candidate);
        return;
    }

    const auto result = QMessageBox::question(
        this,
        "VMT not found",
        QString("Couldn't find a matching VMT next to this VTF.\n\nTried:\n%1\n\nLocate it manually?")
            .arg(QDir::toNativeSeparators(candidate)),
        QMessageBox::Yes | QMessageBox::No);
    if(result != QMessageBox::Yes) return;

    QSettings s;
    const QString startDir = QFileInfo(currentPath_).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, "Open VMT", startDir, "Valve Material Type (*.vmt);;All files (*)");
    if(path.isEmpty()) return;
    s.setValue("paths/open_vmt", QFileInfo(path).absolutePath());
    openVmtFromPath(path);
}

void MainWindow::openLinkedVtfFromVmt() {
    if(currentVmtPath_.isEmpty() && (!vmtEditor_ || vmtEditor_->toPlainText().trimmed().isEmpty())) return;

    const QString base = baseTextureFromVmtTextHeuristic(vmtText());
    if(base.isEmpty()) {
        QMessageBox::information(this, "No $basetexture", "Couldn't find a $basetexture entry in the current VMT.");
        return;
    }

    QString vmtPath = currentVmtPath_;
    if(vmtPath.isEmpty()) vmtPath = "material.vmt";
    const QString nativeVmt = QDir::fromNativeSeparators(vmtPath);

    QString materialsRoot;
    const QString needle = "/materials/";
    const int idx = nativeVmt.lastIndexOf(needle, -1, Qt::CaseInsensitive);
    if(idx >= 0) {
        materialsRoot = nativeVmt.left(idx + needle.size());
    } else if(!currentVmtPath_.isEmpty()) {
        materialsRoot = QDir::fromNativeSeparators(QFileInfo(currentVmtPath_).absolutePath());
        if(!materialsRoot.endsWith('/')) materialsRoot += '/';
    } else {
        materialsRoot = QDir::currentPath();
        if(!materialsRoot.endsWith('/')) materialsRoot += '/';
    }

    QString candidate = materialsRoot + base + ".vtf";
    candidate = QDir::cleanPath(candidate);
    if(QFileInfo::exists(candidate)) {
        openVtf(candidate);
        return;
    }

    const auto result = QMessageBox::question(
        this,
        "Linked VTF not found",
        QString("Couldn't find the linked VTF.\n\n$basetexture: %1\nTried:\n%2\n\nLocate it manually?")
            .arg(base)
            .arg(QDir::toNativeSeparators(candidate)),
        QMessageBox::Yes | QMessageBox::No);
    if(result != QMessageBox::Yes) return;

    QSettings s;
    const QString startDir = idx >= 0 ? materialsRoot : (currentVmtPath_.isEmpty() ? s.value("paths/open_vtf").toString()
                                                                                   : QFileInfo(currentVmtPath_).absolutePath());
    const QString path = QFileDialog::getOpenFileName(this, "Open VTF", startDir, "Valve Texture Format (*.vtf)");
    if(path.isEmpty()) return;
    s.setValue("paths/open_vtf", QFileInfo(path).absolutePath());
    openVtf(path);
}

void MainWindow::vtfPropertiesDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    vlSingle rx = 0, ry = 0, rz = 0;
    vlImageGetReflectivity(&rx, &ry, &rz);

    VtfPropertiesDialog dlg(
        currentPath_,
        vlImageGetWidth(),
        vlImageGetHeight(),
        vlImageGetDepth(),
        vlImageGetFrameCount(),
        vlImageGetFaceCount(),
        vlImageGetMipmapCount(),
        vlImageGetMajorVersion(),
        vlImageGetMinorVersion(),
        vlImageGetFormat(),
        vlImageGetFlags(),
        vlImageGetStartFrame(),
        vlImageGetBumpmapScale(),
        rx,
        ry,
        rz,
        this);

    if(dlg.exec() != QDialog::Accepted) return;
    const auto r = dlg.result();

    bool changed = false;

    if(vlImageGetMinorVersion() != r.minorVersion) {
        const vlUInt before = vlImageGetMinorVersion();
        vlImageSetMinorVersion(r.minorVersion);
        if(vlImageGetMinorVersion() != r.minorVersion) {
            showWarningPopup("VTFLib error",
                             QString("Failed to change minor version from %1 to %2: %3")
                                 .arg(before)
                                 .arg(r.minorVersion)
                                 .arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    }

    if(vlImageGetFlags() != r.flags) {
        vlImageSetFlags(r.flags);
        if(vlImageGetFlags() != r.flags) {
            showWarningPopup("VTFLib error", QString("Failed to set flags: %1").arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    }

    if(vlImageGetStartFrame() != r.startFrame) {
        vlImageSetStartFrame(r.startFrame);
        if(vlImageGetStartFrame() != r.startFrame) {
            showWarningPopup("VTFLib error", QString("Failed to set start frame: %1").arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    }

    if(vlImageGetBumpmapScale() != r.bumpScale) {
        vlImageSetBumpmapScale(r.bumpScale);
        changed = true;
    }

    if(r.computeReflectivity) {
        if(!vlImageComputeReflectivity()) {
            showWarningPopup("VTFLib error", QString("Failed to compute reflectivity: %1").arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    } else {
        vlSingle cx = 0, cy = 0, cz = 0;
        vlImageGetReflectivity(&cx, &cy, &cz);
        if(cx != r.reflectivityX || cy != r.reflectivityY || cz != r.reflectivityZ) {
            vlImageSetReflectivity(r.reflectivityX, r.reflectivityY, r.reflectivityZ);
            changed = true;
        }
    }

    if(r.generateMipmaps) {
        const vlBool srgb = (vlImageGetFlags() & TEXTUREFLAGS_SRGB) ? vlTrue : vlFalse;
        if(!vlImageGenerateAllMipmaps(r.mipmapFilter, srgb)) {
            showWarningPopup("VTFLib error", QString("Failed to generate mipmaps: %1").arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    }

    if(r.generateThumbnail) {
        if(!vlImageGenerateThumbnail(r.thumbnailSrgb ? vlTrue : vlFalse)) {
            showWarningPopup("VTFLib error", QString("Failed to generate thumbnail: %1").arg(QString::fromUtf8(vlGetLastError())));
        } else {
            changed = true;
        }
    }

    if(changed) {
        vtfDirty_ = true;
        actionSaveVtf_->setEnabled(true);
    }

    updateUiFromBoundVtf();
}

void MainWindow::vtfResourcesDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);
    if(!vlImageGetSupportsResources()) {
        QMessageBox::information(this, "Resources unavailable", "This VTF version does not support resources (requires v7.3+).");
        return;
    }

    VtfResourcesDialog dlg(this);
    if(dlg.exec() != QDialog::Accepted) return;

    QString error;
    if(!dlg.applyToBoundVtf(&error)) {
        showWarningPopup("Failed to apply resources", error.isEmpty() ? QString::fromUtf8(vlGetLastError()) : error);
        return;
    }

    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    updateUiFromBoundVtf();
}

void MainWindow::vtfSheetDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);
    if(!vlImageGetSupportsResources()) {
        QMessageBox::information(this, "Resources unavailable", "This VTF version does not support resources (requires v7.3+).");
        return;
    }

    vlUInt size = 0;
    void *data = vlImageGetResourceData(VTF_RSRC_SHEET, &size);
    QByteArray bytes;
    if(data && size) bytes = QByteArray(reinterpret_cast<const char *>(data), static_cast<int>(size));

    VtfSheetDialog dlg(bytes, this);
    if(dlg.exec() != QDialog::Accepted) return;

    QByteArray outBytes = dlg.resultSheetBytes();
    if(outBytes.isEmpty()) {
        showWarningPopup("Sheet update failed", "Sheet data is empty.");
        return;
    }

    if(!vlImageSetResourceData(VTF_RSRC_SHEET, static_cast<vlUInt>(outBytes.size()), outBytes.data())) {
        showWarningPopup("Sheet update failed", QString::fromUtf8(vlGetLastError()));
        return;
    }

    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    updateWindowTitle();
    rebuildResourcesTree();
    setStatusInfo("Updated sheet resource");
}

void MainWindow::editVtfFlagsDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);
    const vlUInt current = vlImageGetFlags();

    VtfFlagsDialog dlg(current, this);
    if(dlg.exec() != QDialog::Accepted) return;
    const vlUInt next = dlg.flags();
    if(next == current) return;

    vlImageSetFlags(next);
    if(vlImageGetFlags() != next) {
        showWarningPopup("VTFLib error", QString("Failed to set flags: %1").arg(QString::fromUtf8(vlGetLastError())));
        return;
    }
    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    infoFlags_->setText(QString("0x%1").arg(static_cast<qulonglong>(next), 8, 16, QChar('0')).toUpper());
    setStatusInfo("Updated VTF flags");
    updateWindowTitle();
}

void MainWindow::saveVtf() {
    if(imageId_ == kInvalidImageId) return;
    if(currentPath_.isEmpty()) {
        saveAsVtfDialog();
        return;
    }
    vlBindImage(imageId_);
    const QByteArray encoded = QFile::encodeName(currentPath_);
    if(!vlImageSave(encoded.constData())) {
        const QString err = QString("Failed to save VTF: %1").arg(QString::fromUtf8(vlGetLastError()));
        showErrorPopup("Save failed", err);
        return;
    }
    vtfDirty_ = false;
    actionSaveVtf_->setEnabled(false);
    addRecentVtf(currentPath_);
    maybeAutoCreateVmt(currentPath_);
    setStatusInfo(QString("Saved %1").arg(QFileInfo(currentPath_).fileName()));
    updateWindowTitle();
}

void MainWindow::saveVmt() {
    if(currentVmtPath_.isEmpty()) {
        saveVmtAs();
        return;
    }
    QString error;
    if(!writeTextFileUtf8(currentVmtPath_, vmtText(), &error)) {
        showErrorPopup("Save failed", QString("Failed to save VMT: %1").arg(error));
        return;
    }
    vmtEditor_->document()->setModified(false);
    addRecentVmt(currentVmtPath_);
    setStatusInfo(QString("Saved %1").arg(QFileInfo(currentVmtPath_).fileName()));
    updateWindowTitle();
}

void MainWindow::saveVmtAs() {
    const QString suggested = currentVmtPath_.isEmpty() ? "material.vmt" : QFileInfo(currentVmtPath_).fileName();
    QSettings s;
    const QString startDir = s.value("paths/save_vmt").toString();
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(suggested);
    const QString path = QFileDialog::getSaveFileName(this, "Save VMT As", start, "Valve Material Type (*.vmt);;All files (*)");
    if(path.isEmpty()) return;
    s.setValue("paths/save_vmt", QFileInfo(path).absolutePath());
    currentVmtPath_ = path;
    saveVmt();
}

void MainWindow::validateVmtStrict() {
    QString error;
    if(!validateVmtText(vmtText(), PARSE_MODE_STRICT, &error)) {
        showWarningPopup("VMT invalid", error);
        return;
    }
    QMessageBox::information(this, "VMT valid", "VMT is valid (strict).");
    setStatusInfo("VMT valid (strict)");
}

void MainWindow::validateVmtLoose() {
    QString error;
    if(!validateVmtText(vmtText(), PARSE_MODE_LOOSE, &error)) {
        showWarningPopup("VMT invalid", error);
        return;
    }
    QMessageBox::information(this, "VMT valid", "VMT is valid (loose).");
    setStatusInfo("VMT valid (loose)");
}

void MainWindow::batchConvertDialog() {
    BatchConvertDialog dlg(this);
    if(dlg.exec() != QDialog::Accepted) return;

    const QString input = dlg.inputDir();
    const QString output = dlg.outputDir();
    if(input.isEmpty() || output.isEmpty()) {
        showWarningPopup("Missing paths", "Please choose both input and output folders.");
        return;
    }

    QDir inDir(input);
    QDir outDir(output);
    if(!inDir.exists() || !outDir.exists()) {
        showWarningPopup("Invalid folder", "Input or output folder does not exist.");
        return;
    }

    if(dlg.outputUnderMaterials()) {
        const QString materialsOut = outDir.absoluteFilePath("materials");
        QDir().mkpath(materialsOut);
        outDir = QDir(materialsOut);
    }

    const QString materialsRoot = dlg.materialsRoot().isEmpty() ? outDir.absolutePath() : dlg.materialsRoot();

    QStringList patterns;
    const auto formats = QImageReader::supportedImageFormats();
    for(const auto &fmt : formats) {
        const QString ext = QString::fromLatin1(fmt).toLower();
        if(ext.isEmpty()) continue;
        patterns.push_back("*." + ext);
    }
    patterns.removeDuplicates();
    const auto flags = dlg.recursive() ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(input, patterns, QDir::Files, flags);

    QList<QString> files;
    while(it.hasNext()) {
        files.push_back(it.next());
    }
    if(files.isEmpty()) {
        QMessageBox::information(this, "No files", "No supported images found in the input folder.");
        return;
    }

    BatchConvertRunDialog::Job job;
    job.inputRoot = input;
    job.outputRoot = outDir.absolutePath();
    job.materialsRoot = materialsRoot;
    job.overwrite = dlg.overwrite();
    job.createVmts = dlg.createVmts();
    job.vmtShader = dlg.vmtShader();
    job.vmtPrefix = dlg.vmtBaseTexturePrefix();
    job.createOptions = dlg.createOptions();
    job.useAlphaFormat = dlg.useAlphaFormat();
    job.alphaFormat = dlg.alphaFormat();
    job.createLodResource = dlg.createLodResource();
    job.lodClampU = dlg.lodClampU();
    job.lodClampV = dlg.lodClampV();
    job.createInformationResource = dlg.createInformationResource();
    job.infoAuthor = dlg.informationAuthor();
    job.infoContact = dlg.informationContact();
    job.infoVersion = dlg.informationVersion();
    job.infoModification = dlg.informationModification();
    job.infoDescription = dlg.informationDescription();
    job.infoComments = dlg.informationComments();
    job.inputFiles = files;

    BatchConvertRunDialog run(job, this);
    run.exec();
}

void MainWindow::wadConvertDialog() {
    WadConvertDialog dlg(this);
    dlg.exec();
}

void MainWindow::generateNormalMapCurrentFrame() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const auto result = QMessageBox::question(
        this,
        "Generate normal map",
        "Generate a normal map for the current frame?\n\nThis modifies the VTF image data.",
        QMessageBox::Yes | QMessageBox::No);
    if(result != QMessageBox::Yes) return;

    if(!vlImageGenerateNormalMap(frame_, KERNEL_FILTER_3X3, HEIGHT_CONVERSION_METHOD_AVERAGE_RGB, NORMAL_ALPHA_RESULT_WHITE)) {
        showWarningPopup("VTFLib error", QString("Failed to generate normal map: %1").arg(QString::fromUtf8(vlGetLastError())));
        return;
    }

    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    updateUiFromBoundVtf();
    setStatusInfo("Generated normal map (current frame)");
}

void MainWindow::generateNormalMapsAllFrames() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const auto result = QMessageBox::question(
        this,
        "Generate normal maps",
        "Generate normal maps for all frames?\n\nThis modifies the VTF image data.",
        QMessageBox::Yes | QMessageBox::No);
    if(result != QMessageBox::Yes) return;

    if(!vlImageGenerateAllNormalMaps(KERNEL_FILTER_3X3, HEIGHT_CONVERSION_METHOD_AVERAGE_RGB, NORMAL_ALPHA_RESULT_WHITE)) {
        showWarningPopup("VTFLib error", QString("Failed to generate normal maps: %1").arg(QString::fromUtf8(vlGetLastError())));
        return;
    }

    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    updateUiFromBoundVtf();
    setStatusInfo("Generated normal maps (all frames)");
}

void MainWindow::generateSphereMap() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    if(vlImageGetFaceCount() < 6) {
        QMessageBox::information(this, "Not a cubemap", "Sphere map generation requires a 6-face cubemap VTF.");
        return;
    }

    const auto result = QMessageBox::question(
        this,
        "Generate sphere map",
        "Generate a sphere map from the 6 cubemap faces?\n\nThis modifies the VTF image data (adds/updates the spheremap face).",
        QMessageBox::Yes | QMessageBox::No);
    if(result != QMessageBox::Yes) return;

    if(!vlImageGenerateSphereMap()) {
        showWarningPopup("VTFLib error", QString("Failed to generate sphere map: %1").arg(QString::fromUtf8(vlGetLastError())));
        return;
    }

    vtfDirty_ = true;
    actionSaveVtf_->setEnabled(true);
    updateUiFromBoundVtf();
    setStatusInfo("Generated sphere map");
}

void MainWindow::exportPngDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    QString error;
    const QImage rgba = vtfSelectionToRgba(frame_, face_, slice_, mip_, &error);
    if(rgba.isNull()) {
        showWarningPopup("Export failed", error);
        return;
    }

    QSettings s;
    QString suggested = currentPath_;
    if(!suggested.isEmpty()) {
        suggested = QFileInfo(suggested).completeBaseName() + ".png";
    }

    const QString startDir = s.value("paths/export_png").toString();
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(QFileInfo(suggested).fileName());
    const QString path = QFileDialog::getSaveFileName(this, "Export PNG", start, "PNG Image (*.png)");
    if(path.isEmpty()) return;
    s.setValue("paths/export_png", QFileInfo(path).absolutePath());

    QImageWriter writer(path, "png");
    if(!writer.write(rgba)) {
        const QString err = QString("Failed to save PNG: %1").arg(writer.errorString());
        showErrorPopup("Export failed", err);
        return;
    }

    setStatusInfo(QString("Exported %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::exportThumbnailPngDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    QString error;
    const QImage rgba = vtfThumbnailToRgba(&error);
    if(rgba.isNull()) {
        QMessageBox::information(this, "No thumbnail", error.isEmpty() ? "VTF contains no thumbnail." : error);
        return;
    }

    QSettings s;
    QString suggested = currentPath_;
    if(!suggested.isEmpty()) {
        suggested = QFileInfo(suggested).completeBaseName() + "_thumbnail.png";
    } else {
        suggested = "thumbnail.png";
    }

    const QString startDir = s.value("paths/export_png").toString();
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(QFileInfo(suggested).fileName());
    const QString path = QFileDialog::getSaveFileName(this, "Export Thumbnail PNG", start, "PNG Image (*.png)");
    if(path.isEmpty()) return;
    s.setValue("paths/export_png", QFileInfo(path).absolutePath());

    QImageWriter writer(path, "png");
    if(!writer.write(rgba)) {
        const QString err = QString("Failed to save PNG: %1").arg(writer.errorString());
        showErrorPopup("Export failed", err);
        return;
    }

    setStatusInfo(QString("Exported %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::exportImageAsDialog() {
    if(viewImage_.isNull() && rawRgbaImage_.isNull()) return;

    QSettings s;
    QString suggested = currentPath_;
    if(!suggested.isEmpty()) {
        suggested = QFileInfo(suggested).completeBaseName() + ".png";
    }

    QStringList exts;
    for(const auto &fmt : QImageWriter::supportedImageFormats()) {
        const QString ext = QString::fromLatin1(fmt).toLower();
        if(ext.isEmpty()) continue;
        exts.push_back("*." + ext);
    }
    exts.removeDuplicates();
    exts.sort();

    const QString startDir = s.value("paths/export_image").toString();
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(QFileInfo(suggested).fileName());
    const QString filter = exts.isEmpty() ? "Images (*.*)" : QString("Images (%1)").arg(exts.join(' '));
    const QString path = QFileDialog::getSaveFileName(this, "Export Image", start, filter);
    if(path.isEmpty()) return;
    s.setValue("paths/export_image", QFileInfo(path).absolutePath());

    const QImage &img = viewImage_.isNull() ? rawRgbaImage_ : viewImage_;
    QImageWriter writer(path);
    if(!writer.write(img)) {
        const QString err = QString("Failed to export image: %1").arg(writer.errorString());
        showErrorPopup("Export failed", err);
        return;
    }

    setStatusInfo(QString("Exported %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::copyViewToClipboard() {
    if(viewImage_.isNull() && rawRgbaImage_.isNull()) return;
    const QImage &img = viewImage_.isNull() ? rawRgbaImage_ : viewImage_;
    QGuiApplication::clipboard()->setImage(img);
    setStatusInfo("Copied image to clipboard");
}

void MainWindow::pasteImageFromClipboard() {
    const QImage img = QGuiApplication::clipboard()->image();
    if(img.isNull()) {
        showInfoPopup("Clipboard empty", "Clipboard does not contain an image.");
        return;
    }
    if(!maybeSaveVtf()) return;
    createVtfFromRgbaImage(ensureRgba8888(img), "clipboard");
}

QString MainWindow::defaultVmtTextForVtfPath(const QString &vtfPath) const {
    auto quoted = [](QString s) {
        s.replace('"', "\\\"");
        return QString("\"%1\"").arg(s);
    };

    QString baseTexture = baseTextureFromVtfPathHeuristic(vtfPath);
    if(baseTexture.isEmpty()) baseTexture = QFileInfo(vtfPath).completeBaseName();

    QStringList lines;
    lines << quoted("VertexLitGeneric");
    lines << "{";
    lines << QString("\t%1 %2").arg(quoted("$basetexture"), quoted(baseTexture));
    lines << "}";
    lines << "";
    return lines.join("\n");
}

bool MainWindow::maybeAutoCreateVmt(const QString &vtfPath) {
    if(!autoCreateVmtEnabled()) return false;
    if(vtfPath.isEmpty()) return false;

    QString vmtPath = vtfPath;
    if(vmtPath.endsWith(".vtf", Qt::CaseInsensitive)) {
        vmtPath.chop(4);
        vmtPath += ".vmt";
    } else {
        return false;
    }
    if(QFileInfo::exists(vmtPath)) return false;

    QString error;
    if(!writeTextFileUtf8(vmtPath, defaultVmtTextForVtfPath(vtfPath), &error)) {
        showWarningPopup("Auto Create VMT", QString("Failed to create VMT: %1").arg(error));
        return false;
    }
    addRecentVmt(vmtPath);
    setStatusInfo(QString("Auto-created %1").arg(QFileInfo(vmtPath).fileName()));
    return true;
}

bool MainWindow::createVtfFromRgbaImage(const QImage &rgba8888, const QString &suggestedBaseName) {
    if(rgba8888.isNull()) return false;
    return createVtfFromRgbaImages({rgba8888}, suggestedBaseName);
}

bool MainWindow::createVtfFromRgbaImages(const QList<QImage> &rgba8888Images, const QString &suggestedBaseName) {
    if(rgba8888Images.isEmpty()) return false;
    for(const auto &img : rgba8888Images) {
        if(img.isNull()) return false;
    }

    CreateVtfDialog dlg(this);
    if(dlg.exec() != QDialog::Accepted) return false;
    SVTFCreateOptions opts = dlg.options();

    vlUInt frames = 1, faces = 1, slices = 1;
    switch(dlg.textureType()) {
        case CreateVtfDialog::TextureType::Animated:
            frames = static_cast<vlUInt>(rgba8888Images.size());
            break;
        case CreateVtfDialog::TextureType::EnvironmentMap:
            faces = static_cast<vlUInt>(rgba8888Images.size());
            break;
        case CreateVtfDialog::TextureType::VolumeTexture:
            slices = static_cast<vlUInt>(rgba8888Images.size());
            break;
    }

    const int w = rgba8888Images[0].width();
    const int h = rgba8888Images[0].height();
    for(const auto &img : rgba8888Images) {
        if(img.width() != w || img.height() != h) {
            showErrorPopup("Create failed", "All imported images must have the same dimensions.");
            return false;
        }
    }

    QSettings s;
    const QString startDir = s.value("paths/save_vtf").toString();
    const QString suggested = (suggestedBaseName.isEmpty() ? "texture" : suggestedBaseName) + ".vtf";
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(suggested);
    const QString savePath = QFileDialog::getSaveFileName(this, "Save VTF As", start, "Valve Texture Format (*.vtf)");
    if(savePath.isEmpty()) return false;
    s.setValue("paths/save_vtf", QFileInfo(savePath).absolutePath());

    vlUInt newImageId = 0;
    if(!vlCreateImage(&newImageId)) {
        showErrorPopup("VTFLib error", QString("Failed to create image handle: %1").arg(vlGetLastError()));
        return false;
    }
    if(!vlBindImage(newImageId)) {
        vlDeleteImage(newImageId);
        showErrorPopup("VTFLib error", QString("Failed to bind image handle: %1").arg(vlGetLastError()));
        return false;
    }

    bool hasAlpha = false;
    for(const auto &img : rgba8888Images) {
        if(imageHasTransparencyRgba8888(img)) {
            hasAlpha = true;
            break;
        }
    }

    SVTFCreateOptions createOpts = opts;
    if(dlg.useAlphaFormat() && hasAlpha) {
        createOpts.ImageFormat = dlg.alphaFormat();
    }

    if(!vtflibCanEncode(createOpts.ImageFormat)) {
        showWarningPopup("Format unavailable",
                         "This build of VTFLib cannot encode to the selected image format.\n\n"
                         "Pick a supported format, or install Compressonator and rebuild for DXT formats.");
        vlDeleteImage(newImageId);
        return false;
    }
    if(createOpts.bThumbnail && !vtflibCanEncode(IMAGE_FORMAT_DXT1)) {
        // CVTFFile::Create() forces low-res image format to DXT1, so disable thumbnail if we can't encode DXT1.
        createOpts.bThumbnail = vlFalse;
        showInfoPopup("Thumbnail disabled",
                      "This build of VTFLib can't encode DXT1, so the VTF thumbnail will be skipped.\n\n"
                      "Install Compressonator and rebuild to enable thumbnails.");
    }

    std::vector<std::vector<vlByte>> rgbaBytes;
    rgbaBytes.reserve(static_cast<size_t>(rgba8888Images.size()));
    std::vector<vlByte *> rgbaPtrs;
    rgbaPtrs.reserve(static_cast<size_t>(rgba8888Images.size()));
    for(const auto &img : rgba8888Images) {
        rgbaBytes.push_back(qimageToContiguousRgba(img));
        rgbaPtrs.push_back(rgbaBytes.back().data());
    }

    const bool created = (rgba8888Images.size() == 1)
                             ? (vlImageCreateSingle(static_cast<vlUInt>(w), static_cast<vlUInt>(h), rgbaPtrs[0], &createOpts) != 0)
                             : (vlImageCreateMultiple(static_cast<vlUInt>(w),
                                                      static_cast<vlUInt>(h),
                                                      frames,
                                                      faces,
                                                      slices,
                                                      rgbaPtrs.data(),
                                                      &createOpts) != 0);
    if(!created) {
        const QString err = QString("Failed to create VTF: %1").arg(vlGetLastError());
        vlDeleteImage(newImageId);
        showErrorPopup("Create failed", err);
        return false;
    }

    {
        QSettings settings;
        const bool createLod = settings.value("options/resources/createLod", false).toBool();
        const bool createInfo = settings.value("options/resources/createInformation", false).toBool();
        if(createLod || createInfo) {
            if(createOpts.uiVersion[0] != VTF_MAJOR_VERSION || createOpts.uiVersion[1] < 3) {
                showWarningPopup("Resources not supported",
                                 "This VTF version does not support resources (requires VTF v7.3+).\n\n"
                                 "Disable resource templates in Global Options or use a newer VTF version.");
            } else {
                if(createLod) {
                    SVTFTextureLODControlResource lod{};
                    lod.ResolutionClampU = static_cast<vlByte>(std::clamp(settings.value("options/resources/lodClampU", 0).toInt(), 0, 255));
                    lod.ResolutionClampV = static_cast<vlByte>(std::clamp(settings.value("options/resources/lodClampV", 0).toInt(), 0, 255));
                    vlUInt packed = 0;
                    std::memcpy(&packed, &lod, sizeof(lod));
                    if(!vlImageSetResourceData(VTF_RSRC_TEXTURE_LOD_SETTINGS, sizeof(vlUInt), &packed)) {
                        showWarningPopup("Resource warning", QString("Failed to create LOD resource: %1").arg(QString::fromUtf8(vlGetLastError())));
                    }
                }

                if(createInfo) {
                    auto add = [](QStringList *lines, const QString &k, const QString &v) {
                        const QString t = v.trimmed();
                        if(t.isEmpty()) return;
                        lines->push_back(QString("\t%1 %2").arg(quotedKvdValue(k), quotedKvdValue(t)));
                    };

                    QStringList lines;
                    lines << quotedKvdValue("Information");
                    lines << "{";
                    add(&lines, "Author", settings.value("options/resources/infoAuthor").toString());
                    add(&lines, "Contact", settings.value("options/resources/infoContact").toString());
                    add(&lines, "Version", settings.value("options/resources/infoVersion").toString());
                    add(&lines, "Modification", settings.value("options/resources/infoModification").toString());
                    add(&lines, "Description", settings.value("options/resources/infoDescription").toString());
                    add(&lines, "Comments", settings.value("options/resources/infoComments").toString());
                    lines << "}";
                    lines << "";

                    QByteArray bytes = lines.join('\n').toUtf8();
                    bytes.push_back('\0');
                    if(!vlImageSetResourceData(VTF_RSRC_KEY_VALUE_DATA, static_cast<vlUInt>(bytes.size()), bytes.data())) {
                        showWarningPopup("Resource warning", QString("Failed to create Information resource: %1").arg(QString::fromUtf8(vlGetLastError())));
                    }
                }
            }
        }
    }

    const QByteArray encoded = QFile::encodeName(savePath);
    if(!vlImageSave(encoded.constData())) {
        const QString err = QString("Failed to save VTF: %1").arg(vlGetLastError());
        vlDeleteImage(newImageId);
        showErrorPopup("Save failed", err);
        return false;
    }

    if(imageId_ != kInvalidImageId) {
        vlDeleteImage(imageId_);
    }
    imageId_ = newImageId;
    currentPath_ = savePath;
    frame_ = face_ = slice_ = mip_ = 0;
    isPlaying_ = false;
    playbackTimer_->stop();
    actionPlayPause_->setText("&Play");
    vtfDirty_ = false;
    actionSaveVtf_->setEnabled(false);
    updateUiFromBoundVtf();
    addRecentVtf(savePath);
    maybeAutoCreateVmt(savePath);
    setStatusInfo(QString("Created %1").arg(QFileInfo(savePath).fileName()));
    return true;
}

void MainWindow::exportAllFramesDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const auto frames = vlImageGetFrameCount();
    if(frames <= 1) {
        QMessageBox::information(this, "Nothing to export", "This VTF has only one frame.");
        return;
    }

    QSettings s;
    const QString startDir = s.value("paths/export_frames").toString();
    const QString dirPath = QFileDialog::getExistingDirectory(this, "Export frames to folder", startDir);
    if(dirPath.isEmpty()) return;
    s.setValue("paths/export_frames", dirPath);

    const QString baseName = QFileInfo(currentPath_).completeBaseName();
    const QDir dir(dirPath);

    for(unsigned int f = 0; f < frames; ++f) {
        QString error;
        const QImage rgba = vtfSelectionToRgba(f, face_, slice_, mip_, &error);
        if(rgba.isNull()) {
            showWarningPopup("Export failed", error);
            return;
        }
        const QString outPath = dir.filePath(QString("%1_frame%2.png").arg(baseName).arg(f, 3, 10, QChar('0')));
        QImageWriter writer(outPath, "png");
        if(!writer.write(rgba)) {
            showWarningPopup("Export failed", QString("Failed to export %1: %2").arg(outPath, writer.errorString()));
            return;
        }
    }

    setStatusInfo(QString("Exported %1 frames").arg(frames));
}

void MainWindow::exportAllFacesDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const auto faces = vlImageGetFaceCount();
    if(faces <= 1) {
        QMessageBox::information(this, "Nothing to export", "This VTF has only one face.");
        return;
    }

    QSettings s;
    const QString startDir = s.value("paths/export_faces").toString();
    const QString dirPath = QFileDialog::getExistingDirectory(this, "Export faces to folder", startDir);
    if(dirPath.isEmpty()) return;
    s.setValue("paths/export_faces", dirPath);

    const QString baseName = QFileInfo(currentPath_).completeBaseName();
    const QDir dir(dirPath);

    for(unsigned int f = 0; f < faces; ++f) {
        QString error;
        const QImage rgba = vtfSelectionToRgba(frame_, f, slice_, mip_, &error);
        if(rgba.isNull()) {
            showWarningPopup("Export failed", error);
            return;
        }

        const QString tag = faces >= 6 ? cubeFaceFileTag(f) : QString("face%1").arg(f, 2, 10, QChar('0'));
        const QString outPath = dir.filePath(QString("%1_%2_frame%3_mip%4.png")
                                                 .arg(baseName)
                                                 .arg(tag)
                                                 .arg(frame_, 3, 10, QChar('0'))
                                                 .arg(mip_, 2, 10, QChar('0')));
        QImageWriter writer(outPath, "png");
        if(!writer.write(rgba)) {
            showWarningPopup("Export failed", QString("Failed to export %1: %2").arg(outPath, writer.errorString()));
            return;
        }
    }

    setStatusInfo(QString("Exported %1 faces (frame %2, mip %3)").arg(faces).arg(frame_).arg(mip_));
}

void MainWindow::exportAllSlicesDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    vlUInt depth = 0;
    {
        const vlUInt baseW = vlImageGetWidth();
        const vlUInt baseH = vlImageGetHeight();
        const vlUInt baseD = vlImageGetDepth();
        vlUInt w = 0, h = 0, d = 0;
        vlImageComputeMipmapDimensions(baseW, baseH, baseD, mip_, &w, &h, &d);
        depth = d;
    }

    if(depth <= 1) {
        QMessageBox::information(this, "Nothing to export", "This VTF has only one slice at the current mip.");
        return;
    }

    QSettings s;
    const QString startDir = s.value("paths/export_slices").toString();
    const QString dirPath = QFileDialog::getExistingDirectory(this, "Export slices to folder", startDir);
    if(dirPath.isEmpty()) return;
    s.setValue("paths/export_slices", dirPath);

    const QString baseName = QFileInfo(currentPath_).completeBaseName();
    const QDir dir(dirPath);
    const unsigned int faces = vlImageGetFaceCount();
    const QString faceTag = faces >= 6 ? cubeFaceFileTag(face_) : QString("face%1").arg(face_, 2, 10, QChar('0'));

    for(unsigned int sidx = 0; sidx < depth; ++sidx) {
        QString error;
        const QImage rgba = vtfSelectionToRgba(frame_, face_, sidx, mip_, &error);
        if(rgba.isNull()) {
            showWarningPopup("Export failed", error);
            return;
        }

        const QString outPath = dir.filePath(QString("%1_%2_frame%3_mip%4_slice%5.png")
                                                 .arg(baseName)
                                                 .arg(faceTag)
                                                 .arg(frame_, 3, 10, QChar('0'))
                                                 .arg(mip_, 2, 10, QChar('0'))
                                                 .arg(sidx, 3, 10, QChar('0')));
        QImageWriter writer(outPath, "png");
        if(!writer.write(rgba)) {
            showWarningPopup("Export failed", QString("Failed to export %1: %2").arg(outPath, writer.errorString()));
            return;
        }
    }

    setStatusInfo(QString("Exported %1 slices (frame %2, face %3, mip %4)").arg(depth).arg(frame_).arg(face_).arg(mip_));
}

void MainWindow::exportAllMipmapsDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const auto mips = vlImageGetMipmapCount();
    if(mips <= 1) {
        QMessageBox::information(this, "Nothing to export", "This VTF has only one mipmap.");
        return;
    }

    QSettings s;
    const QString startDir = s.value("paths/export_mips").toString();
    const QString dirPath = QFileDialog::getExistingDirectory(this, "Export mipmaps to folder", startDir);
    if(dirPath.isEmpty()) return;
    s.setValue("paths/export_mips", dirPath);

    const QString baseName = QFileInfo(currentPath_).completeBaseName();
    const QDir dir(dirPath);

    for(unsigned int m = 0; m < mips; ++m) {
        QString error;
        const QImage rgba = vtfSelectionToRgba(frame_, face_, slice_, m, &error);
        if(rgba.isNull()) {
            showWarningPopup("Export failed", error);
            return;
        }
        const QString outPath = dir.filePath(QString("%1_face%2_frame%3_mip%4.png")
                                                 .arg(baseName)
                                                 .arg(face_, 2, 10, QChar('0'))
                                                 .arg(frame_, 3, 10, QChar('0'))
                                                 .arg(m, 2, 10, QChar('0')));
        QImageWriter writer(outPath, "png");
        if(!writer.write(rgba)) {
            showWarningPopup("Export failed", QString("Failed to export %1: %2").arg(outPath, writer.errorString()));
            return;
        }
    }

    setStatusInfo(QString("Exported %1 mipmaps (frame %2, face %3)").arg(mips).arg(frame_).arg(face_));
}

void MainWindow::exportAllDialog() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const unsigned int frames = vlImageGetFrameCount();
    const unsigned int faces = vlImageGetFaceCount();
    const unsigned int mips = vlImageGetMipmapCount();

    if(frames <= 1 && faces <= 1 && vlImageGetDepth() <= 1) {
        QMessageBox::information(this, "Nothing to export", "This VTF has only one frame, face and slice.");
        return;
    }

    int selectedMip = static_cast<int>(mip_);
    if(mips > 1) {
        bool ok = false;
        selectedMip = QInputDialog::getInt(this, "Export All", "Mipmap level:", selectedMip, 0, static_cast<int>(mips - 1), 1, &ok);
        if(!ok) return;
    } else {
        selectedMip = 0;
    }

    vlUInt depth = 0;
    {
        const vlUInt baseW = vlImageGetWidth();
        const vlUInt baseH = vlImageGetHeight();
        const vlUInt baseD = vlImageGetDepth();
        vlUInt w = 0, h = 0, d = 0;
        vlImageComputeMipmapDimensions(baseW, baseH, baseD, static_cast<vlUInt>(selectedMip), &w, &h, &d);
        depth = d;
    }

    QSettings s;
    const QString startDir = s.value("paths/export_all").toString();
    const QString dirPath = QFileDialog::getExistingDirectory(this, "Export all images to folder", startDir);
    if(dirPath.isEmpty()) return;
    s.setValue("paths/export_all", dirPath);

    const QString baseName = QFileInfo(currentPath_).completeBaseName();
    const QDir dir(dirPath);

    for(unsigned int fr = 0; fr < frames; ++fr) {
        for(unsigned int fa = 0; fa < faces; ++fa) {
            const QString tag = faces >= 6 ? cubeFaceFileTag(fa) : QString("face%1").arg(fa, 2, 10, QChar('0'));
            for(unsigned int sl = 0; sl < std::max<vlUInt>(1, depth); ++sl) {
                QString error;
                const QImage rgba = vtfSelectionToRgba(fr, fa, sl, static_cast<unsigned int>(selectedMip), &error);
                if(rgba.isNull()) {
                    showWarningPopup("Export failed", error);
                    return;
                }

                const QString outPath = dir.filePath(QString("%1_%2_frame%3_mip%4_slice%5.png")
                                                         .arg(baseName)
                                                         .arg(tag)
                                                         .arg(fr, 3, 10, QChar('0'))
                                                         .arg(selectedMip, 2, 10, QChar('0'))
                                                         .arg(sl, 3, 10, QChar('0')));
                QImageWriter writer(outPath, "png");
                if(!writer.write(rgba)) {
                    showWarningPopup("Export failed", QString("Failed to export %1: %2").arg(outPath, writer.errorString()));
                    return;
                }
            }
        }
    }

    const unsigned int total = frames * faces * std::max<vlUInt>(1, depth);
    setStatusInfo(QString("Exported %1 images (mip %2)").arg(total).arg(selectedMip));
}

void MainWindow::importImageDialog() {
    QSettings s;
    const QString startDir = s.value("paths/import_image").toString();
    const QStringList paths = QFileDialog::getOpenFileNames(this, "Import Image(s)", startDir, "Images (*.png *.jpg *.jpeg *.bmp);;All files (*.*)");
    if(paths.isEmpty()) return;
    s.setValue("paths/import_image", QFileInfo(paths[0]).absolutePath());
    // Defer until after the native file dialog fully closes.
    QTimer::singleShot(0, this, [this, paths] { importImagesFromPaths(paths); });
}

bool MainWindow::importImageFromPath(const QString &path) {
    if(path.isEmpty()) return false;
    return importImagesFromPaths({path});
}

bool MainWindow::importImagesFromPaths(const QStringList &paths) {
    if(paths.isEmpty()) return false;
    if(!maybeSaveVtf()) return false;

    QList<QImage> images;
    images.reserve(paths.size());
    for(const auto &path : paths) {
        QImageReader reader(path);
        reader.setAutoTransform(true);
        QImage image = reader.read();
        if(image.isNull()) {
            showErrorPopup("Import failed", QString("Failed to read image: %1").arg(reader.errorString()));
            return false;
        }
        images.push_back(ensureRgba8888(image));
    }
    return createVtfFromRgbaImages(images, QFileInfo(paths[0]).completeBaseName());
}

void MainWindow::saveAsVtfDialog() {
    if(imageId_ == kInvalidImageId) return;
    QSettings s;
    const QString startDir = s.value("paths/save_vtf").toString();
    const QString suggested = QFileInfo(currentPath_).fileName();
    const QString start = startDir.isEmpty() ? suggested : QDir(startDir).filePath(suggested);
    const QString path = QFileDialog::getSaveFileName(this, "Save VTF As", start, "Valve Texture Format (*.vtf)");
    if(path.isEmpty()) return;
    s.setValue("paths/save_vtf", QFileInfo(path).absolutePath());
    vlBindImage(imageId_);

    const QByteArray encoded = QFile::encodeName(path);
    if(!vlImageSave(encoded.constData())) {
        const QString err = QString("Failed to save VTF: %1").arg(vlGetLastError());
        showErrorPopup("Save failed", err);
        return;
    }

    currentPath_ = path;
    infoPath_->setText(currentPath_);
    vtfDirty_ = false;
    actionSaveVtf_->setEnabled(false);
    addRecentVtf(path);
    maybeAutoCreateVmt(path);
    setStatusInfo(QString("Saved %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::zoomIn() {
    if(rawRgbaImage_.isNull()) return;
    if(fitToWindow_) {
        fitToWindow_ = false;
        actionZoomFit_->setChecked(false);
    }
    setZoom(zoom_ * 1.25);
    prevManualZoom_ = zoom_;
}

void MainWindow::zoomOut() {
    if(rawRgbaImage_.isNull()) return;
    if(fitToWindow_) {
        fitToWindow_ = false;
        actionZoomFit_->setChecked(false);
    }
    setZoom(zoom_ / 1.25);
    prevManualZoom_ = zoom_;
}

void MainWindow::zoomReset() {
    if(rawRgbaImage_.isNull()) return;
    if(fitToWindow_) {
        fitToWindow_ = false;
        actionZoomFit_->setChecked(false);
    }
    setZoom(1.0);
    prevManualZoom_ = zoom_;
}

void MainWindow::zoomFit() {
    if(rawRgbaImage_.isNull()) return;

    fitToWindow_ = actionZoomFit_->isChecked();
    if(fitToWindow_) {
        prevManualZoom_ = zoom_;
        updateFitZoom();
        return;
    }
    setZoom(prevManualZoom_);
}

void MainWindow::zoomComboChanged() {
    if(rawRgbaImage_.isNull()) return;
    if(!zoomCombo_) return;

    QString t = zoomCombo_->currentText().trimmed();
    if(t.endsWith('%')) t.chop(1);
    t = t.trimmed();

    bool ok = false;
    const double pct = t.toDouble(&ok);
    if(!ok) {
        updateZoomUi();
        return;
    }

    if(fitToWindow_) {
        fitToWindow_ = false;
        actionZoomFit_->setChecked(false);
    }
    setZoom(pct / 100.0);
    prevManualZoom_ = zoom_;
}

void MainWindow::updateSelectionLimits() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);
    const unsigned int frames = vlImageGetFrameCount();
    const unsigned int faces = vlImageGetFaceCount();
    const unsigned int mips = vlImageGetMipmapCount();

    if(frames > 0 && frame_ >= frames) frame_ = frames - 1;
    if(faces > 0 && face_ >= faces) face_ = faces - 1;
    if(mips > 0 && mip_ >= mips) mip_ = mips - 1;

    unsigned int slices = 0;
    {
        const vlUInt baseW = vlImageGetWidth();
        const vlUInt baseH = vlImageGetHeight();
        const vlUInt baseD = vlImageGetDepth();
        vlUInt w = 0, h = 0, d = 0;
        vlImageComputeMipmapDimensions(baseW, baseH, baseD, mip_, &w, &h, &d);
        slices = static_cast<unsigned int>(d);
    }
    if(slices == 0) slices = 1;
    if(slice_ >= slices) slice_ = slices - 1;

    frameSpin_->blockSignals(true);
    faceSpin_->blockSignals(true);
    sliceSpin_->blockSignals(true);
    mipSpin_->blockSignals(true);

    frameSpin_->setRange(0, frames > 0 ? static_cast<int>(frames - 1) : 0);
    faceSpin_->setRange(0, faces > 0 ? static_cast<int>(faces - 1) : 0);
    sliceSpin_->setRange(0, slices > 0 ? static_cast<int>(slices - 1) : 0);
    mipSpin_->setRange(0, mips > 0 ? static_cast<int>(mips - 1) : 0);

    frameSpin_->setValue(static_cast<int>(frame_));
    faceSpin_->setValue(static_cast<int>(face_));
    sliceSpin_->setValue(static_cast<int>(slice_));
    mipSpin_->setValue(static_cast<int>(mip_));

    frameSpin_->blockSignals(false);
    faceSpin_->blockSignals(false);
    sliceSpin_->blockSignals(false);
    mipSpin_->blockSignals(false);

    frameSpin_->setEnabled(frames > 1);
    faceSpin_->setEnabled(faces > 1);
    sliceSpin_->setEnabled(slices > 1);
    mipSpin_->setEnabled(mips > 1);

    if(faces >= 6) {
        faceSpin_->setToolTip("Cubemap faces:\n0 +X Right\n1 -X Left\n2 +Y Back\n3 -Y Front\n4 +Z Up\n5 -Z Down\n6 Sphere Map");
    } else {
        faceSpin_->setToolTip({});
    }
}

void MainWindow::selectionChanged() {
    if(imageId_ == kInvalidImageId) return;
    frame_ = static_cast<unsigned int>(frameSpin_->value());
    face_ = static_cast<unsigned int>(faceSpin_->value());
    slice_ = static_cast<unsigned int>(sliceSpin_->value());
    mip_ = static_cast<unsigned int>(mipSpin_->value());
    channel_ = static_cast<ChannelMode>(channelCombo_->currentData().toInt());
    background_ = static_cast<BackgroundMode>(backgroundCombo_->currentData().toInt());

    // Slice count depends on mip level (depth shrinks with mipmaps).
    updateSelectionLimits();
    renderSelection();
    updateCubemapPreview();
    if(statusSelection_) {
        QString faceLabel;
        if(imageId_ != kInvalidImageId) {
            vlBindImage(imageId_);
            if(vlImageGetFaceCount() >= 6) {
                const QString name = cubeFaceName(face_);
                if(!name.isEmpty()) faceLabel = QString(" (%1)").arg(name);
            }
        }
        statusSelection_->setText(QString("F%1  Face%2%3  S%4  M%5").arg(frame_).arg(face_).arg(faceLabel).arg(slice_).arg(mip_));
    }
}

void MainWindow::updateBackgroundComboCustomIcon() {
    if(!backgroundCombo_) return;
    for(int i = 0; i < backgroundCombo_->count(); ++i) {
        const auto mode = static_cast<BackgroundMode>(backgroundCombo_->itemData(i).toInt());
        if(mode != BackgroundMode::Custom) continue;
        backgroundCombo_->setItemIcon(i, solidColorIcon(customBackgroundColor_));
        return;
    }
}

void MainWindow::selectCustomBackgroundColor() {
    const QColor next = QColorDialog::getColor(customBackgroundColor_, this, "Select Background Color");
    if(!next.isValid()) return;
    customBackgroundColor_ = next;
    QSettings().setValue("view/customBackgroundColor", customBackgroundColor_.name(QColor::HexRgb));
    updateBackgroundComboCustomIcon();
    renderSelection();
    updateCubemapPreview();
}

void MainWindow::renderSelection(bool showErrorPopup) {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    QString error;
    QImage rgba;
    previewIsHdrTonemapped_ = false;
    if(showThumbnail_) {
        rgba = vtfThumbnailToRgba(&error);
        if(rgba.isNull()) {
            showThumbnail_ = false;
            if(actionShowThumbnail_) actionShowThumbnail_->setChecked(false);
            error = error.isEmpty() ? "VTF contains no thumbnail." : error;
        }
    }
    if(rgba.isNull()) {
        const auto fmt = vlImageGetFormat();
        if(hdrTonemapEnabled_ && vtfFormatIsHdr(fmt)) {
            if(hdrTonemap_ == HdrTonemap::VTFLib) {
                vlSetFloat(VTFLIB_FP16_HDR_EXPOSURE, static_cast<vlSingle>(hdrExposure_));
                rgba = vtfSelectionToRgba(frame_, face_, slice_, mip_, &error);
                if(!rgba.isNull()) previewIsHdrTonemapped_ = true;
            } else {
                rgba = vtfSelectionToHdrTonemappedRgba(frame_, face_, slice_, mip_, hdrExposure_, hdrTonemap_, &error);
                if(!rgba.isNull()) previewIsHdrTonemapped_ = true;
            }
        } else {
            rgba = vtfSelectionToRgba(frame_, face_, slice_, mip_, &error);
        }
    }
    if(rgba.isNull()) {
        if(showErrorPopup) {
            showWarningPopup("Preview unavailable", error);
            setViewImage({});
        }
        return;
    }
    setViewImage(rgba);
}

void MainWindow::togglePlayback() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    if(vlImageGetFrameCount() <= 1) return;

    isPlaying_ = !isPlaying_;
    if(isPlaying_) {
        playbackTimer_->setInterval(frameMsSpin_->value());
        playbackTimer_->start();
        actionPlayPause_->setText("&Pause");
    } else {
        playbackTimer_->stop();
        actionPlayPause_->setText("&Play");
    }
}

void MainWindow::playbackTick() {
    if(imageId_ == kInvalidImageId) return;
    vlBindImage(imageId_);

    const unsigned int frames = vlImageGetFrameCount();
    if(frames <= 1) return;

    frame_ = (frame_ + 1) % frames;
    frameSpin_->setValue(static_cast<int>(frame_));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if(!event->mimeData()->hasUrls()) return;
    const auto urls = event->mimeData()->urls();
    if(urls.isEmpty()) return;

    const QString local = urls.first().toLocalFile();
    if(local.endsWith(".vtf", Qt::CaseInsensitive) || local.endsWith(".vmt", Qt::CaseInsensitive)) {
        event->acceptProposedAction();
        return;
    }

    QImageReader reader(local);
    if(reader.canRead()) {
        event->acceptProposedAction();
        return;
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    if(!event->mimeData()->hasUrls()) return;
    const auto urls = event->mimeData()->urls();
    if(urls.isEmpty()) return;
    const QString local = urls.first().toLocalFile();
    if(local.isEmpty()) return;
    if(local.endsWith(".vtf", Qt::CaseInsensitive)) {
        openVtf(local);
        return;
    }
    if(local.endsWith(".vmt", Qt::CaseInsensitive)) {
        openVmtFromPath(local);
        return;
    }

    QImageReader reader(local);
    if(reader.canRead()) {
        importImageFromPath(local);
        return;
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if(!maybeSaveVtf() || !maybeSaveVmt()) {
        event->ignore();
        return;
    }
    saveUiState();
    QMainWindow::closeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    const bool isViewport = scrollArea_ && watched == scrollArea_->viewport();
    const bool isImage = watched == imageLabel_;
    if(!isViewport && !isImage) return QMainWindow::eventFilter(watched, event);

    auto toViewportPos = [&](const QPointF &pos) -> QPoint {
        if(isViewport) return pos.toPoint();
        if(scrollArea_) return scrollArea_->viewport()->mapFrom(imageLabel_, pos.toPoint());
        return pos.toPoint();
    };

    auto updatePixelStatus = [&](const QPoint &viewportPos) {
        if(!statusPixel_) return;
        if(rawRgbaImage_.isNull() || !scrollArea_ || !imageLabel_) {
            statusPixel_->setText("-");
            return;
        }
        const QPoint labelPos = imageLabel_->mapFrom(scrollArea_->viewport(), viewportPos);
        if(labelPos.x() < 0 || labelPos.y() < 0 || labelPos.x() >= imageLabel_->width() || labelPos.y() >= imageLabel_->height()) {
            statusPixel_->setText("-");
            return;
        }
        const int x = static_cast<int>(labelPos.x() / zoom_);
        const int y = static_cast<int>(labelPos.y() / zoom_);
        if(tileMode_) {
            const int rw = rawRgbaImage_.width();
            const int rh = rawRgbaImage_.height();
            if(x < 0 || y < 0 || rw <= 0 || rh <= 0) {
                statusPixel_->setText("-");
                return;
            }
            const int tx = x % rw;
            const int ty = y % rh;
            const QColor c = rawRgbaImage_.pixelColor(tx, ty);
            statusPixel_->setText(QString("%1,%2  (tile %3,%4)  RGBA %5,%6,%7,%8")
                                      .arg(x)
                                      .arg(y)
                                      .arg(tx)
                                      .arg(ty)
                                      .arg(c.red())
                                      .arg(c.green())
                                      .arg(c.blue())
                                      .arg(c.alpha()));
            return;
        }

        if(x < 0 || y < 0 || x >= rawRgbaImage_.width() || y >= rawRgbaImage_.height()) {
            statusPixel_->setText("-");
            return;
        }
        const QColor c = rawRgbaImage_.pixelColor(x, y);
        statusPixel_->setText(QString("%1,%2  RGBA %3,%4,%5,%6").arg(x).arg(y).arg(c.red()).arg(c.green()).arg(c.blue()).arg(c.alpha()));
    };

    switch(event->type()) {
        case QEvent::Wheel: {
            if(rawRgbaImage_.isNull() || !scrollArea_) break;
            auto *e = static_cast<QWheelEvent *>(event);
            if((e->modifiers() & Qt::ControlModifier) == 0) break;

            if(fitToWindow_) {
                fitToWindow_ = false;
                actionZoomFit_->setChecked(false);
                prevManualZoom_ = zoom_;
            }

            const QPoint vpPos = toViewportPos(localPosF(e));
            const int steps = e->angleDelta().y() / 120;
            if(steps == 0) {
                e->accept();
                return true;
            }
            const double factor = std::pow(1.25, steps);
            setZoom(zoom_ * factor, vpPos);
            prevManualZoom_ = zoom_;
            e->accept();
            return true;
        }
        case QEvent::KeyPress: {
            auto *e = static_cast<QKeyEvent *>(event);
            if(e->matches(QKeySequence::Paste)) {
                const QImage img = QGuiApplication::clipboard()->image();
                if(!img.isNull()) {
                    pasteImageFromClipboard();
                    e->accept();
                    return true;
                }
            }
            break;
        }
        case QEvent::MouseButtonPress: {
            if(!scrollArea_) break;
            auto *e = static_cast<QMouseEvent *>(event);
            const bool altHeld = (e->modifiers() & Qt::AltModifier) != 0;
            if(e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && altHeld)) {
                isPanning_ = true;
                panStartViewportPos_ = toViewportPos(localPosF(e));
                panStartScroll_ = QPoint(scrollArea_->horizontalScrollBar()->value(), scrollArea_->verticalScrollBar()->value());
                scrollArea_->viewport()->setCursor(Qt::ClosedHandCursor);
                e->accept();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            if(!scrollArea_) break;
            auto *e = static_cast<QMouseEvent *>(event);
            const QPoint vpPos = toViewportPos(localPosF(e));
            if(isPanning_) {
                const QPoint delta = vpPos - panStartViewportPos_;
                scrollArea_->horizontalScrollBar()->setValue(panStartScroll_.x() - delta.x());
                scrollArea_->verticalScrollBar()->setValue(panStartScroll_.y() - delta.y());
                e->accept();
                return true;
            }
            updatePixelStatus(vpPos);
            break;
        }
        case QEvent::MouseButtonRelease: {
            if(!scrollArea_) break;
            auto *e = static_cast<QMouseEvent *>(event);
            if(isPanning_ && (e->button() == Qt::MiddleButton || e->button() == Qt::LeftButton)) {
                isPanning_ = false;
                scrollArea_->viewport()->unsetCursor();
                e->accept();
                return true;
            }
            break;
        }
        case QEvent::Leave: {
            if(statusPixel_) statusPixel_->setText("-");
            break;
        }
        case QEvent::Resize: {
            if(fitToWindow_ && watched == scrollArea_->viewport()) {
                updateFitZoom();
            }
            break;
        }
        case QEvent::ContextMenu: {
            auto *e = static_cast<QContextMenuEvent *>(event);
            QMenu menu(this);
            menu.addAction(actionOpen_);
            if(imageId_ != kInvalidImageId) {
                menu.addSeparator();
                menu.addAction(actionReloadVtf_);
                menu.addAction(actionOpenContainingFolder_);
                menu.addAction(actionOpenMatchingVmt_);
                menu.addAction(actionCloseVtf_);
                menu.addSeparator();
                menu.addAction(actionExportPng_);
                menu.addAction(actionExportThumbnailPng_);
                menu.addAction(actionExportAs_);
                menu.addAction(actionCopyImage_);
                menu.addSeparator();
                menu.addAction(actionExportAllFrames_);
                menu.addAction(actionExportAllFaces_);
                menu.addAction(actionExportAllSlices_);
                menu.addAction(actionExportAllMipmaps_);
                menu.addAction(actionExportAll_);
                menu.addSeparator();
                menu.addAction(actionZoomIn_);
                menu.addAction(actionZoomOut_);
                menu.addAction(actionZoomReset_);
                menu.addAction(actionZoomFit_);
                menu.addAction(actionShowThumbnail_);
                menu.addAction(actionCubemapPreview_);
                menu.addSeparator();
                menu.addAction(actionVtfProperties_);
                menu.addAction(actionVtfResources_);
                menu.addAction(actionEditVtfFlags_);
            }
            menu.exec(e->globalPos());
            e->accept();
            return true;
        }
        default:
            break;
    }

    return QMainWindow::eventFilter(watched, event);
}
