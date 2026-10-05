#pragma once

#include "elo/perception/camera_catalog.hpp"

#include <QImage>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

namespace elo::perception {

/// @brief Continuously captures transient frames, detects faces and produces
/// embeddings only after biometric authorization.
class VisionService final : public QObject {
    Q_OBJECT

public:
    VisionService(
        CameraDevice camera,
        QString detector_model_path,
        QString recognizer_model_path,
        QObject* parent = nullptr);
    ~VisionService() override;

    VisionService(const VisionService&) = delete;
    VisionService& operator=(const VisionService&) = delete;

    void start();
    void stop();

public slots:
    void setBiometricAuthorized(bool authorized);

signals:
    void frameReady(const QImage& frame);
    void facePresenceChanged(bool present);
    void faceEmbeddingReady(const QVector<float>& embedding, double quality);
    void statusChanged(const QString& status);
    void failure(const QString& message);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace elo::perception
