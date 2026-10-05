#include "elo/ui/kiosk_presentation_model.hpp"

namespace elo::ui {

#ifdef ELO_HAS_QT

KioskPresentationModel::KioskPresentationModel(
    std::shared_ptr<experience::ExperienceEngine> engine, QObject* parent)
    : QObject(parent), engine_{std::move(engine)} {}

QString KioskPresentationModel::currentState() const {
    if (!engine_) return QStringLiteral("UNAVAILABLE");
    return QString::fromUtf8(experience::to_string(engine_->current_state()).data());
}

QString KioskPresentationModel::activePerson() const {
    if (!engine_ || !engine_->active_person()) return QStringLiteral("UNKNOWN");
    return QString::fromStdString(engine_->active_person()->str());
}

QString KioskPresentationModel::currentContent() const {
    return QString::fromStdString(current_content_);
}

bool KioskPresentationModel::isBiometricSession() const {
    if (!engine_) return false;
    return engine_->current_state() == experience::SessionState::BIOMETRIC_SESSION ||
           engine_->current_state() == experience::SessionState::IDENTITY_SUPPORTED ||
           engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE;
}

quint32 KioskPresentationModel::kernelGeneration() const {
    if (!engine_) return 0;
    return engine_->kernel_view().generation;
}

void KioskPresentationModel::userApproached() {
    if (engine_) {
        engine_->on_presence_detected();
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::acknowledgeInfo() {
    if (engine_) {
        engine_->on_information_acknowledged();
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::chooseParticipation(bool participate) {
    if (engine_) {
        engine_->decide_participation(participate);
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::chooseBiometricConsent(bool consent) {
    if (engine_) {
        engine_->decide_biometric_consent(consent);
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::advanceContent() {
    if (engine_) {
        current_content_ = engine_->select_next_content();
        emit contentChanged();
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::submitSurveyResponse(
    const QString& questionId, const QString& selectedOption, bool anonymous) {
    if (engine_) {
        auto policy = anonymous ? survey::LinkagePolicy::UnlinkedAnonymous
                                : survey::LinkagePolicy::LinkedToIdentity;
        (void)engine_->submit_survey_response(
            questionId.toStdString(), selectedOption.toStdString(), policy);
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::requestForgetMe() {
    if (engine_ && engine_->active_person()) {
        (void)engine_->request_forget(*engine_->active_person());
        emit stateChanged();
        emit kernelChanged();
    }
}

void KioskPresentationModel::finishSession() {
    if (engine_) {
        engine_->finish_session();
        current_content_.clear();
        emit contentChanged();
        emit stateChanged();
        emit kernelChanged();
    }
}

#else

KioskPresentationModel::KioskPresentationModel(
    std::shared_ptr<experience::ExperienceEngine> engine)
    : engine_{std::move(engine)} {}

std::string KioskPresentationModel::currentState() const {
    if (!engine_) return "UNAVAILABLE";
    return std::string(experience::to_string(engine_->current_state()));
}

std::string KioskPresentationModel::activePerson() const {
    if (!engine_ || !engine_->active_person()) return "UNKNOWN";
    return engine_->active_person()->str();
}

std::string KioskPresentationModel::currentContent() const {
    return current_content_;
}

bool KioskPresentationModel::isBiometricSession() const {
    if (!engine_) return false;
    return engine_->current_state() == experience::SessionState::BIOMETRIC_SESSION ||
           engine_->current_state() == experience::SessionState::IDENTITY_SUPPORTED ||
           engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE;
}

uint32_t KioskPresentationModel::kernelGeneration() const {
    if (!engine_) return 0;
    return engine_->kernel_view().generation;
}

void KioskPresentationModel::userApproached() {
    if (engine_) engine_->on_presence_detected();
}

void KioskPresentationModel::acknowledgeInfo() {
    if (engine_) engine_->on_information_acknowledged();
}

void KioskPresentationModel::chooseParticipation(bool participate) {
    if (engine_) engine_->decide_participation(participate);
}

void KioskPresentationModel::chooseBiometricConsent(bool consent) {
    if (engine_) engine_->decide_biometric_consent(consent);
}

void KioskPresentationModel::advanceContent() {
    if (engine_) current_content_ = engine_->select_next_content();
}

void KioskPresentationModel::submitSurveyResponse(
    const std::string& questionId, const std::string& selectedOption, bool anonymous) {
    if (engine_) {
        auto policy = anonymous ? survey::LinkagePolicy::UnlinkedAnonymous
                                : survey::LinkagePolicy::LinkedToIdentity;
        (void)engine_->submit_survey_response(questionId, selectedOption, policy);
    }
}

void KioskPresentationModel::requestForgetMe() {
    if (engine_ && engine_->active_person()) {
        (void)engine_->request_forget(*engine_->active_person());
    }
}

void KioskPresentationModel::finishSession() {
    if (engine_) {
        engine_->finish_session();
        current_content_.clear();
    }
}

#endif

} // namespace elo::ui
