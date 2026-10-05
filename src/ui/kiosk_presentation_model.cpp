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
           engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE ||
           engine_->current_state() == experience::SessionState::IDENTITY_UNCERTAIN;
}

quint32 KioskPresentationModel::kernelGeneration() const {
    if (!engine_) return 0;
    return engine_->kernel_view().generation;
}

void KioskPresentationModel::chooseParticipation(bool participate) {
    if (engine_) {
        engine_->decide_participation(participate);
        emit stateChanged();
    }
}

void KioskPresentationModel::chooseBiometricConsent(bool consent) {
    if (engine_) {
        if (engine_->current_state() == experience::SessionState::PRESENCE_DETECTED ||
            engine_->current_state() == experience::SessionState::CONSENT_PENDING) {
            engine_->decide_participation(true);
        }
        engine_->decide_biometric_consent(consent);
        emit stateChanged();
        emit biometricAuthorizationChanged(consent);
    }
}

void KioskPresentationModel::advanceContent() {
    if (engine_) {
        current_content_ = engine_->select_next_content();
        emit contentChanged();
        emit stateChanged();
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
    }
}

void KioskPresentationModel::requestForgetMe() {
    if (engine_ && engine_->active_person()) {
        (void)engine_->request_forget(*engine_->active_person());
        emit stateChanged();
    }
}

void KioskPresentationModel::finishSession() {
    if (engine_) {
        emit biometricAuthorizationChanged(false);
        engine_->finish_session();
        current_content_.clear();
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::onCameraFrameReady() {
    ++camera_frame_revision_;
    emit cameraFrameChanged();
}

void KioskPresentationModel::onFacePresenceChanged(bool present) {
    if (face_detected_ == present) {
        return;
    }

    face_detected_ = present;
    emit visionChanged();
    if (present && engine_ && engine_->current_state() == experience::SessionState::IDLE) {
        engine_->on_presence_detected();
        emit stateChanged();
    } else if (!present && engine_ &&
               (engine_->current_state() == experience::SessionState::PRESENCE_DETECTED ||
                engine_->current_state() == experience::SessionState::CONSENT_PENDING ||
                engine_->current_state() == experience::SessionState::BIOMETRIC_SESSION ||
                engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE ||
                engine_->current_state() == experience::SessionState::IDENTITY_UNCERTAIN)) {
        emit biometricAuthorizationChanged(false);
        engine_->finish_session();
        emit stateChanged();
    }
}

void KioskPresentationModel::onFaceEmbeddingReady(
    const QVector<float>& embedding, double quality) {
    if (!engine_ || !isBiometricSession()) {
        return;
    }

    const std::vector<float> values(embedding.begin(), embedding.end());
    auto resolution = engine_->identify_or_enroll_consented_face(values, quality);
    if (!resolution) {
        vision_status_ = QString::fromStdString(resolution.error().to_string());
        emit visionChanged();
        return;
    }

    vision_status_ = QString::fromUtf8(biometric::to_string(resolution->state).data());
    emit visionChanged();
    emit stateChanged();

    if (resolution->state == biometric::IdentityState::SUPPORTED) {
        emit biometricAuthorizationChanged(false);
        current_content_ = engine_->select_next_content();
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::onVisionStatusChanged(const QString& status) {
    vision_operational_ = status != QStringLiteral("Câmera parada");
    vision_status_ = status;
    emit visionChanged();
}

void KioskPresentationModel::onVisionFailure(const QString& message) {
    vision_operational_ = false;
    face_detected_ = false;
    vision_status_ = QStringLiteral("Falha de visão: ") + message;
    emit biometricAuthorizationChanged(false);
    if (engine_ && engine_->current_state() != experience::SessionState::IDLE) {
        engine_->finish_session();
        current_content_.clear();
        emit contentChanged();
        emit stateChanged();
    }
    emit visionChanged();
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
           engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE ||
           engine_->current_state() == experience::SessionState::IDENTITY_UNCERTAIN;
}

uint32_t KioskPresentationModel::kernelGeneration() const {
    if (!engine_) return 0;
    return engine_->kernel_view().generation;
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
