#include "GuiImageLoader.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QIODevice>
#include <QMovie>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "qoi.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

extern "C" {
#include "tinyexr_wrapper.h"
}

namespace {

bool endsWithNoCase(const QString &path, const char *ext) {
    return path.endsWith(QLatin1String(ext), Qt::CaseInsensitive);
}

QImage loadExrToRgba8888(const QString &path, QString *errorOut) {
    float *rgba = nullptr;
    int w = 0, h = 0;
    const char *err = nullptr;
    const int rc = vtfcmd_LoadEXR(&rgba, &w, &h, path.toLocal8Bit().constData(), &err);
    if(rc != VTFCMD_TINYEXR_SUCCESS || !rgba || w <= 0 || h <= 0) {
        if(errorOut) {
            *errorOut = err ? QString::fromUtf8(err) : QStringLiteral("tinyexr: unknown error");
        }
        if(err) vtfcmd_FreeEXRErrorMessage(err);
        if(rgba) std::free(rgba);
        return {};
    }

    QImage img(w, h, QImage::Format_RGBA8888);
    const double invGamma = 1.0 / 2.2;
    for(int y = 0; y < h; ++y) {
        auto *dst = img.scanLine(y);
        const float *src = rgba + static_cast<qsizetype>(y) * w * 4;
        for(int x = 0; x < w; ++x) {
            for(int c = 0; c < 3; ++c) {
                double v = std::max(0.0, std::min(1.0, static_cast<double>(src[x * 4 + c])));
                v = std::pow(v, invGamma);
                dst[x * 4 + c] = static_cast<uchar>(std::lround(v * 255.0));
            }
            double a = std::max(0.0, std::min(1.0, static_cast<double>(src[x * 4 + 3])));
            dst[x * 4 + 3] = static_cast<uchar>(std::lround(a * 255.0));
        }
    }
    std::free(rgba);
    return img;
}

QImage loadQoiToRgba8888(const QString &path, QString *errorOut) {
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) {
        if(errorOut) *errorOut = QStringLiteral("qoi: failed to open file");
        return {};
    }
    const QByteArray bytes = f.readAll();
    qoi_desc desc{};
    void *pixels = qoi_decode(bytes.constData(), bytes.size(), &desc, 4);
    if(!pixels || desc.width == 0 || desc.height == 0) {
        if(errorOut) *errorOut = QStringLiteral("qoi: decode failed");
        if(pixels) std::free(pixels);
        return {};
    }

    QImage img(static_cast<int>(desc.width), static_cast<int>(desc.height), QImage::Format_RGBA8888);
    std::memcpy(img.bits(), pixels, static_cast<size_t>(desc.width) * desc.height * 4);
    std::free(pixels);
    return img;
}

QImage loadWithStbFromBytes(const QByteArray &bytes, bool hdr, QString *errorOut) {
    int w = 0, h = 0, channels = 0;
    if(hdr) {
        float *data = stbi_loadf_from_memory(
            reinterpret_cast<const stbi_uc *>(bytes.constData()),
            static_cast<int>(bytes.size()), &w, &h, &channels, 4);
        if(!data || w <= 0 || h <= 0) {
            if(errorOut) *errorOut = QString::fromUtf8(stbi_failure_reason() ? stbi_failure_reason() : "stb_image: decode failed");
            if(data) stbi_image_free(data);
            return {};
        }
        QImage img(w, h, QImage::Format_RGBA8888);
        const double invGamma = 1.0 / 2.2;
        for(int y = 0; y < h; ++y) {
            auto *dst = img.scanLine(y);
            const float *src = data + static_cast<qsizetype>(y) * w * 4;
            for(int x = 0; x < w; ++x) {
                for(int c = 0; c < 3; ++c) {
                    double v = std::max(0.0, std::min(1.0, static_cast<double>(src[x * 4 + c])));
                    v = std::pow(v, invGamma);
                    dst[x * 4 + c] = static_cast<uchar>(std::lround(v * 255.0));
                }
                double a = std::max(0.0, std::min(1.0, static_cast<double>(src[x * 4 + 3])));
                dst[x * 4 + 3] = static_cast<uchar>(std::lround(a * 255.0));
            }
        }
        stbi_image_free(data);
        return img;
    }

    stbi_uc *data = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc *>(bytes.constData()),
        static_cast<int>(bytes.size()), &w, &h, &channels, 4);
    if(!data || w <= 0 || h <= 0) {
        if(errorOut) *errorOut = QString::fromUtf8(stbi_failure_reason() ? stbi_failure_reason() : "stb_image: decode failed");
        if(data) stbi_image_free(data);
        return {};
    }

    QImage img(w, h, QImage::Format_RGBA8888);
    std::memcpy(img.bits(), data, static_cast<size_t>(w) * h * 4);
    stbi_image_free(data);
    return img;
}

QImage loadWithStb(const QString &path, bool hdr, QString *errorOut) {
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly)) {
        if(errorOut) *errorOut = QStringLiteral("stb_image: failed to open file");
        return {};
    }
    return loadWithStbFromBytes(f.readAll(), hdr, errorOut);
}

bool isStbHdrExtension(const QString &path) {
    return endsWithNoCase(path, ".hdr");
}

bool isStbPrimaryExtension(const QString &path) {
    return endsWithNoCase(path, ".tga")
        || endsWithNoCase(path, ".hdr")
        || endsWithNoCase(path, ".psd")
        || endsWithNoCase(path, ".pic");
}

} // namespace

bool isExtendedImageExtension(const QString &path) {
    return endsWithNoCase(path, ".exr")
        || endsWithNoCase(path, ".qoi")
        || isStbPrimaryExtension(path);
}

bool isAnimatedImageExtension(const QString &path) {
    return endsWithNoCase(path, ".gif") || endsWithNoCase(path, ".apng");
}

QList<QImage> loadAllFramesToRgba8888(const QString &path, QString *errorOut) {
    QList<QImage> frames;
    if(path.isEmpty()) {
        if(errorOut) *errorOut = QStringLiteral("empty path");
        return frames;
    }

    if(isAnimatedImageExtension(path)) {
        QMovie movie(path);
        if(!movie.isValid()) {
            if(errorOut) *errorOut = QStringLiteral("QMovie: not a valid animated image");
            return frames;
        }
        const int total = movie.frameCount();
        if(total <= 0) {
            if(errorOut) *errorOut = QStringLiteral("QMovie: frame count unknown");
            return frames;
        }
        for(int i = 0; i < total; ++i) {
            if(!movie.jumpToFrame(i)) break;
            QImage f = movie.currentImage();
            if(f.isNull()) continue;
            frames.push_back(f.convertToFormat(QImage::Format_RGBA8888));
        }
        if(frames.isEmpty() && errorOut) *errorOut = QStringLiteral("QMovie: no decodable frames");
        return frames;
    }

    QImage single = loadImageFileToRgba8888(path, errorOut);
    if(!single.isNull()) frames.push_back(single);
    return frames;
}

QImage loadImageFileToRgba8888(const QString &path, QString *errorOut) {
    if(path.isEmpty()) {
        if(errorOut) *errorOut = QStringLiteral("empty path");
        return {};
    }

    if(endsWithNoCase(path, ".exr")) return loadExrToRgba8888(path, errorOut);
    if(endsWithNoCase(path, ".qoi")) return loadQoiToRgba8888(path, errorOut);
    if(isStbPrimaryExtension(path)) return loadWithStb(path, isStbHdrExtension(path), errorOut);

    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage img = reader.read();
    if(!img.isNull()) return img;

    // Qt couldn't read it — try stb_image as a last-resort fallback (handles quirky TGAs,
    // older JPEGs, etc. that Qt plugins sometimes reject).
    QString stbError;
    QImage fallback = loadWithStb(path, false, &stbError);
    if(!fallback.isNull()) return fallback;

    if(errorOut) *errorOut = reader.errorString().isEmpty() ? stbError : reader.errorString();
    return {};
}
