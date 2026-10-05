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

QString KioskPresentationModel::contentTitle() const {
    if (!engine_) return QString();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        return QString::fromStdString(actions.front().title);
    }
    if (engine_->active_content_atom()) {
        return QString::fromStdString(engine_->active_content_atom()->title);
    }
    return QStringLiteral("Bioma Pampa");
}

QString KioskPresentationModel::contentText() const {
    if (!engine_) return QString();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        return QString::fromStdString(actions.front().text);
    }
    if (engine_->active_content_variant()) {
        return QString::fromStdString(engine_->active_content_variant()->presentation.text);
    }
    return QStringLiteral("Sempre pronto. Sempre incompleto.");
}

QString KioskPresentationModel::contentMedia() const {
    if (!engine_) return QString();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        return QString::fromStdString(actions.front().asset_path);
    }
    return QString();
}

QStringList KioskPresentationModel::contentOptions() const {
    if (!engine_) return QStringList();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        QStringList list;
        for (const auto& opt : actions.front().options) {
            list.append(QString::fromStdString(opt));
        }
        return list;
    }
    return QStringList();
}

bool KioskPresentationModel::isRecipeActive() const {
    return engine_ && engine_->is_recipe_active();
}

QString KioskPresentationModel::selectionReason() const {
    if (!engine_) return QString();
    return QString::fromStdString(engine_->active_selection_reason().explanation);
}

void KioskPresentationModel::selectPampaContent(const QString& roleStr) {
    if (engine_) {
        auto role = content::parse_content_role(roleStr.toStdString());
        if (role == content::ContentRole::Unknown) role = content::ContentRole::Attract;
        (void)engine_->select_contextual_content(role, "pampa");
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::startRecipe(const QString& recipeId) {
    if (engine_) {
        (void)engine_->start_recipe(recipeId.toStdString());
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::chooseOption(const QString& option) {
    if (engine_) {
        if (engine_->is_recipe_active()) {
            engine_->advance_recipe(option.toStdString());
        }
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::deepenExperience() {
    if (engine_) {
        (void)engine_->select_contextual_content(content::ContentRole::Deepen, "pampa");
        emit contentChanged();
        emit stateChanged();
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
        emit biometricAuthorizationChanged(false);
        emit recognitionVisualStateChanged(false);
        engine_->finish_session();
        current_content_.clear();
        recognition_resolved_ = false;
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
        greeting_title_ = QStringLiteral("Identidade local esquecida");
        greeting_message_ = QStringLiteral("A continuidade anterior foi removida deste totem.");
        emit contentChanged();
        emit recognitionChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::finishSession() {
    if (engine_) {
        emit biometricAuthorizationChanged(false);
        emit recognitionVisualStateChanged(false);
        engine_->finish_session();
        current_content_.clear();
        recognition_resolved_ = false;
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
        greeting_title_ = QStringLiteral("Reconhecendo presença");
        greeting_message_ = QStringLiteral("Processamento local em andamento");
        emit contentChanged();
        emit recognitionChanged();
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
        recognition_resolved_ = false;
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
        greeting_title_ = QStringLiteral("Reconhecendo presença");
        greeting_message_ = QStringLiteral("Comparando somente a identidade facial local");
        engine_->on_presence_detected();
        engine_->begin_automatic_continuity();
        emit recognitionVisualStateChanged(false);
        emit biometricAuthorizationChanged(true);
        emit recognitionChanged();
        emit stateChanged();
    } else if (!present && engine_ &&
               (engine_->current_state() == experience::SessionState::PRESENCE_DETECTED ||
                engine_->current_state() == experience::SessionState::BIOMETRIC_SESSION ||
                engine_->current_state() == experience::SessionState::IDENTITY_CANDIDATE ||
                engine_->current_state() == experience::SessionState::IDENTITY_UNCERTAIN ||
                engine_->current_state() == experience::SessionState::IDENTITY_SUPPORTED)) {
        emit biometricAuthorizationChanged(false);
        emit recognitionVisualStateChanged(false);
        engine_->finish_session();
        recognition_resolved_ = false;
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
        emit recognitionChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::onFaceEmbeddingReady(
    const QVector<float>& embedding, double quality) {
    if (!engine_ || !isBiometricSession()) {
        return;
    }

    const std::vector<float> values(embedding.begin(), embedding.end());
    if (values.empty()) {
        return;
    }
    if (!pending_embeddings_.empty() && pending_embeddings_.front().size() != values.size()) {
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
    }
    pending_embeddings_.push_back(values);
    pending_quality_sum_ += quality;

    constexpr std::size_t samples_required = 3;
    if (pending_embeddings_.size() < samples_required) {
        greeting_message_ = QStringLiteral("Construindo identidade facial local • amostra %1 de %2")
            .arg(pending_embeddings_.size())
            .arg(samples_required);
        emit recognitionChanged();
        return;
    }

    std::vector<float> aggregate(values.size(), 0.0F);
    for (const auto& sample : pending_embeddings_) {
        for (std::size_t index = 0; index < sample.size(); ++index) {
            aggregate[index] += sample[index];
        }
    }
    for (auto& component : aggregate) {
        component /= static_cast<float>(pending_embeddings_.size());
    }
    const double aggregate_quality = pending_quality_sum_ /
        static_cast<double>(pending_embeddings_.size());
    pending_embeddings_.clear();
    pending_quality_sum_ = 0.0;

    auto resolution = engine_->identify_or_enroll_local_face(aggregate, aggregate_quality);
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
        emit recognitionVisualStateChanged(true);
        recognition_resolved_ = true;
        if (resolution->newly_enrolled) {
            greeting_title_ = QStringLiteral("Bem-vindo ao ELO");
            greeting_message_ = QStringLiteral(
                "Sua continuidade local foi criada. Guardamos somente uma representação facial vetorial, nunca a fotografia.");
        } else {
            greeting_title_ = QStringLiteral("Que bom ver você novamente");
            greeting_message_ = QStringLiteral(
                "Continuidade local reconhecida. Vamos seguir de onde você parou.");
        }
        emit recognitionChanged();
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
    emit recognitionVisualStateChanged(false);
    if (engine_ && engine_->current_state() != experience::SessionState::IDLE) {
        engine_->finish_session();
        current_content_.clear();
        recognition_resolved_ = false;
        pending_embeddings_.clear();
        pending_quality_sum_ = 0.0;
        emit contentChanged();
        emit recognitionChanged();
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
