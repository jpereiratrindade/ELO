#include "camera_image_provider.hpp"

#include <QMutexLocker>

CameraImageProvider::CameraImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage CameraImageProvider::requestImage(
    const QString&, QSize* size, const QSize& requested_size) {
    QMutexLocker lock(&mutex_);
    if (size != nullptr) {
        *size = frame_.size();
    }
    if (requested_size.isValid()) {
        return frame_.scaled(requested_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return frame_;
}

void CameraImageProvider::updateFrame(const QImage& frame) {
    QMutexLocker lock(&mutex_);
    if (!frame_.isNull()) {
        frame_.fill(Qt::black);
    }
    frame_ = frame.copy();
}
