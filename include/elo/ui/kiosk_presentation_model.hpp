#pragma once

#include "elo/experience/experience_engine.hpp"

#ifdef ELO_HAS_QT
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#endif

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

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
    Q_PROPERTY(QString greetingTitle READ greetingTitle NOTIFY recognitionChanged)
    Q_PROPERTY(QString greetingMessage READ greetingMessage NOTIFY recognitionChanged)
    Q_PROPERTY(bool recognitionResolved READ recognitionResolved NOTIFY recognitionChanged)
    Q_PROPERTY(QString contentTitle READ contentTitle NOTIFY contentChanged)
    Q_PROPERTY(QString contentText READ contentText NOTIFY contentChanged)
    Q_PROPERTY(QString contentMedia READ contentMedia NOTIFY contentChanged)
    Q_PROPERTY(QString contentAudio READ contentAudio NOTIFY contentChanged)
    Q_PROPERTY(QStringList contentOptions READ contentOptions NOTIFY contentChanged)
    Q_PROPERTY(bool isRecipeActive READ isRecipeActive NOTIFY contentChanged)
    Q_PROPERTY(QString selectionReason READ selectionReason NOTIFY contentChanged)
    Q_PROPERTY(qreal behaviorProgress READ behaviorProgress NOTIFY behaviorProgressChanged)
    Q_PROPERTY(bool autoNavigationEnabled READ autoNavigationEnabled WRITE setAutoNavigationEnabled NOTIFY autoNavigationEnabledChanged)
    Q_PROPERTY(QString behaviorStatus READ behaviorStatus NOTIFY behaviorProgressChanged)
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
    [[nodiscard]] QString greetingTitle() const { return greeting_title_; }
    [[nodiscard]] QString greetingMessage() const { return greeting_message_; }
    [[nodiscard]] bool recognitionResolved() const noexcept { return recognition_resolved_; }
    [[nodiscard]] QString contentTitle() const;
    [[nodiscard]] QString contentText() const;
    [[nodiscard]] QString contentMedia() const;
    [[nodiscard]] QString contentAudio() const;
    [[nodiscard]] QStringList contentOptions() const;
    [[nodiscard]] bool isRecipeActive() const;
    [[nodiscard]] QString selectionReason() const;
    [[nodiscard]] qreal behaviorProgress() const noexcept { return behavior_progress_; }
    [[nodiscard]] bool autoNavigationEnabled() const noexcept { return auto_navigation_enabled_; }
    void setAutoNavigationEnabled(bool enabled);
    [[nodiscard]] QString behaviorStatus() const;

    Q_INVOKABLE void advanceContent();
    Q_INVOKABLE void selectPampaContent(const QString& role = QStringLiteral("attract"));
    Q_INVOKABLE void startRecipe(const QString& recipeId = QStringLiteral("discover_by_sound"));
    Q_INVOKABLE void chooseOption(const QString& option);
    Q_INVOKABLE void deepenExperience();
    Q_INVOKABLE void submitSurveyResponse(const QString& questionId, const QString& selectedOption, bool anonymous);
    Q_INVOKABLE void requestForgetMe();
    Q_INVOKABLE void finishSession();
    Q_INVOKABLE void resetBehaviorTimer();
    Q_INVOKABLE void playSound(const QString& soundPath);

    void tick(double delta_seconds);

public slots:
    void onCameraFrameReady();
    void onFacePresenceChanged(bool present);
    void onFaceEmbeddingReady(const QVector<float>& embedding, double quality);
    void onVisionStatusChanged(const QString& status);
    void onVisionFailure(const QString& message);
    void onBehaviorTick();

signals:
    void stateChanged();
    void contentChanged();
    void kernelChanged();
    void cameraFrameChanged();
    void visionChanged();
    void recognitionChanged();
    void biometricAuthorizationChanged(bool authorized);
    void recognitionVisualStateChanged(bool confirmed);
    void behaviorProgressChanged();
    void autoNavigationEnabledChanged();
#else
    [[nodiscard]] std::string currentState() const;
    [[nodiscard]] std::string activePerson() const;
    [[nodiscard]] std::string currentContent() const;
    [[nodiscard]] bool isBiometricSession() const;
    [[nodiscard]] uint32_t kernelGeneration() const;

    void advanceContent();
    void submitSurveyResponse(const std::string& questionId, const std::string& selectedOption, bool anonymous);
    void requestForgetMe();
    void finishSession();
    void tick(double delta_seconds);
#endif

private:
    std::shared_ptr<experience::ExperienceEngine> engine_;
    std::string current_content_{};
    double state_duration_{0.0};
    double absence_duration_{0.0};
    double behavior_progress_{0.0};
    bool auto_navigation_enabled_{true};
#ifdef ELO_HAS_QT
    quint64 camera_frame_revision_{0};
    bool face_detected_{false};
    bool vision_operational_{false};
    QString vision_status_{QStringLiteral("Inicializando visão")};
    QString greeting_title_{QStringLiteral("Reconhecendo presença")};
    QString greeting_message_{QStringLiteral("Processamento local em andamento")};
    bool recognition_resolved_{false};
    std::vector<std::vector<float>> pending_embeddings_{};
    double pending_quality_sum_{0.0};
    QTimer* behavior_timer_{nullptr};
#endif
};

} // namespace elo::ui
