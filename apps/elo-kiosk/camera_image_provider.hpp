#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class CameraImageProvider final : public QQuickImageProvider {
public:
    CameraImageProvider();

    [[nodiscard]] QImage requestImage(
        const QString& id,
        QSize* size,
        const QSize& requested_size) override;

    void updateFrame(const QImage& frame);

private:
    QMutex mutex_;
    QImage frame_;
};
