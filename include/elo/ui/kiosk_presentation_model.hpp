#pragma once

#include "elo/experience/experience_engine.hpp"

#ifdef ELO_HAS_QT
#include <QObject>
#include <QString>
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

    Q_INVOKABLE void userApproached();
    Q_INVOKABLE void acknowledgeInfo();
    Q_INVOKABLE void chooseParticipation(bool participate);
    Q_INVOKABLE void chooseBiometricConsent(bool consent);
    Q_INVOKABLE void advanceContent();
    Q_INVOKABLE void submitSurveyResponse(const QString& questionId, const QString& selectedOption, bool anonymous);
    Q_INVOKABLE void requestForgetMe();
    Q_INVOKABLE void finishSession();

signals:
    void stateChanged();
    void contentChanged();
    void kernelChanged();
#else
    [[nodiscard]] std::string currentState() const;
    [[nodiscard]] std::string activePerson() const;
    [[nodiscard]] std::string currentContent() const;
    [[nodiscard]] bool isBiometricSession() const;
    [[nodiscard]] uint32_t kernelGeneration() const;

    void userApproached();
    void acknowledgeInfo();
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
};

} // namespace elo::ui
