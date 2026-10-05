#pragma once

#include "elo/experience/experience_engine.hpp"

#ifdef ELO_HAS_QT
#include <QObject>
#include <QString>
#include <QVector>
#endif

#include <memory>
#include <string>

namespace elo::ui {

#ifdef ELO_HAS_QT
class KioskPresentationModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentState READ currentState NOTIFY stateChanged)
    Q_PROPERTY(QString activePerson READ activePerson NOTIFY stateChanged)
    Q_PROPERTY(QString currentContent READ currentContent NOTIFY contentChanged)
    Q_PROPERTY(bool isBiometricSession READ isBiometricSession NOTIFY stateChanged)
    Q_PROPERTY(quint32 kernelGeneration READ kernelGeneration NOTIFY kernelChanged)
    Q_PROPERTY(quint64 cameraFrameRevision READ cameraFrameRevision NOTIFY cameraFrameChanged)
    Q_PROPERTY(bool faceDetected READ faceDetected NOTIFY visionChanged)
    Q_PROPERTY(bool visionOperational READ visionOperational NOTIFY visionChanged)
    Q_PROPERTY(QString visionStatus READ visionStatus NOTIFY visionChanged)
#else
class KioskPresentationModel {
#endif

public:
#ifdef ELO_HAS_QT
    explicit KioskPresentationModel(std::shared_ptr<experience::ExperienceEngine> engine, QObject* parent = nullptr);
#else
    explicit KioskPresentationModel(std::shared_ptr<experience::ExperienceEngine> engine);
#endif

#ifdef ELO_HAS_QT
    [[nodiscard]] QString currentState() const;
    [[nodiscard]] QString activePerson() const;
    [[nodiscard]] QString currentContent() const;
    [[nodiscard]] bool isBiometricSession() const;
    [[nodiscard]] quint32 kernelGeneration() const;
    [[nodiscard]] quint64 cameraFrameRevision() const noexcept { return camera_frame_revision_; }
    [[nodiscard]] bool faceDetected() const noexcept { return face_detected_; }
    [[nodiscard]] bool visionOperational() const noexcept { return vision_operational_; }
    [[nodiscard]] QString visionStatus() const { return vision_status_; }

    Q_INVOKABLE void chooseParticipation(bool participate);
    Q_INVOKABLE void chooseBiometricConsent(bool consent);
    Q_INVOKABLE void advanceContent();
    Q_INVOKABLE void submitSurveyResponse(const QString& questionId, const QString& selectedOption, bool anonymous);
    Q_INVOKABLE void requestForgetMe();
    Q_INVOKABLE void finishSession();

public slots:
    void onCameraFrameReady();
    void onFacePresenceChanged(bool present);
    void onFaceEmbeddingReady(const QVector<float>& embedding, double quality);
    void onVisionStatusChanged(const QString& status);
    void onVisionFailure(const QString& message);

signals:
    void stateChanged();
    void contentChanged();
    void kernelChanged();
    void cameraFrameChanged();
    void visionChanged();
    void biometricAuthorizationChanged(bool authorized);
#else
    [[nodiscard]] std::string currentState() const;
    [[nodiscard]] std::string activePerson() const;
    [[nodiscard]] std::string currentContent() const;
    [[nodiscard]] bool isBiometricSession() const;
    [[nodiscard]] uint32_t kernelGeneration() const;

    void chooseParticipation(bool participate);
    void chooseBiometricConsent(bool consent);
    void advanceContent();
    void submitSurveyResponse(const std::string& questionId, const std::string& selectedOption, bool anonymous);
    void requestForgetMe();
    void finishSession();
#endif

private:
    std::shared_ptr<experience::ExperienceEngine> engine_;
    std::string current_content_{};
#ifdef ELO_HAS_QT
    quint64 camera_frame_revision_{0};
    bool face_detected_{false};
    bool vision_operational_{false};
    QString vision_status_{QStringLiteral("Inicializando visão")};
#endif
};

} // namespace elo::ui
