#pragma once

#include <QImage>
#include <QList>
#include <QString>

QImage loadImageFileToRgba8888(const QString &path, QString *errorOut = nullptr);
bool isExtendedImageExtension(const QString &path);

// Returns a list of frames for animated containers (GIF/APNG); for static images, returns a single-frame list.
// Empty on error; errorOut is populated with a diagnostic if provided.
QList<QImage> loadAllFramesToRgba8888(const QString &path, QString *errorOut = nullptr);
bool isAnimatedImageExtension(const QString &path);
