#include "elo/content/content_bundle.hpp"
#include "elo/ui/kiosk_presentation_model.hpp"

#ifdef ELO_HAS_QT
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#endif
#include <algorithm>

namespace elo::ui {

#ifdef ELO_HAS_QT

KioskPresentationModel::KioskPresentationModel(
    std::shared_ptr<experience::ExperienceEngine> engine, QObject* parent)
    : QObject(parent), engine_{std::move(engine)} {
    if (QCoreApplication::instance()) {
        behavior_timer_ = new QTimer(this);
        connect(behavior_timer_, &QTimer::timeout, this, &KioskPresentationModel::onBehaviorTick);
        behavior_timer_->start(100);
    }
}

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

QString KioskPresentationModel::bundleTitle() const {
    if (engine_ && engine_->content_catalog()) {
        const auto& manifest = engine_->content_catalog()->manifest();
        if (!manifest.title.empty()) {
            return QString::fromStdString(manifest.title);
        }
    }
    return QString();
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
    return bundleTitle();
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
    if (engine_->active_content_atom() && !engine_->active_content_atom()->canonical_facts.empty()) {
        return QString::fromStdString(engine_->active_content_atom()->canonical_facts.front().statement);
    }
    return QString();
}

QString KioskPresentationModel::contentMedia() const {
    if (!engine_) return QString();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        return QString::fromStdString(actions.front().asset_path);
    }
    return QString();
}

QString KioskPresentationModel::contentImage() const {
    if (!engine_) return QString();

    QString candidate;
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        const auto& path = actions.front().asset_path;
        if (path.ends_with(".png") || path.ends_with(".jpg") || path.ends_with(".jpeg") || path.ends_with(".webp")) {
            candidate = QString::fromStdString(path);
        }
    }
    if (candidate.isEmpty() && engine_->active_content_atom()) {
        const auto& imgs = engine_->active_content_atom()->assets.images;
        if (!imgs.empty()) {
            candidate = QString::fromStdString(imgs.front());
        }
    }
    if (candidate.isEmpty() && engine_->active_content_variant()) {
        for (const auto& media : engine_->active_content_variant()->presentation.media_refs) {
            if (media.ends_with(".png") || media.ends_with(".jpg") || media.ends_with(".jpeg") || media.ends_with(".webp")) {
                candidate = QString::fromStdString(media);
                break;
            }
        }
    }
    if (candidate.isEmpty()) return QString();

    // Check direct file or search in sovereign system content asset locations
    if (QFile::exists(candidate)) {
        return QUrl::fromLocalFile(QFileInfo(candidate).absoluteFilePath()).toString();
    }

    auto sys_content = content::resolve_system_content_dir(false);
    QString sysPath = QString::fromStdString(sys_content.string());

    const QStringList searchPrefixes = {
        sysPath + QStringLiteral("/assets/"),
        sysPath + QStringLiteral("/assets/images/"),
        sysPath + QStringLiteral("/current/assets/"),
        sysPath + QStringLiteral("/current/assets/images/"),
        sysPath + QStringLiteral("/"),
        sysPath + QStringLiteral("/current/")
    };

    for (const auto& prefix : searchPrefixes) {
        QString testPath = prefix + candidate;
        if (QFile::exists(testPath)) {
            return QUrl::fromLocalFile(QFileInfo(testPath).absoluteFilePath()).toString();
        }
        if (candidate.startsWith(QStringLiteral("assets/"))) {
            QString testSub = prefix + candidate.mid(7);
            if (QFile::exists(testSub)) {
                return QUrl::fromLocalFile(QFileInfo(testSub).absoluteFilePath()).toString();
            }
        }
    }

    return QString();
}

bool KioskPresentationModel::hasImage() const {
    QString imgUrl = contentImage();
    if (imgUrl.isEmpty()) return false;
    QUrl url(imgUrl);
    return QFile::exists(url.toLocalFile());
}

QString KioskPresentationModel::contentScientificName() const {
    if (!engine_ || !engine_->active_content_atom()) return QString();
    return QString::fromStdString(engine_->active_content_atom()->subject.scientific_name);
}

QString KioskPresentationModel::contentTypeLabel() const {
    if (!engine_ || !engine_->active_content_atom()) return QString();
    return QString::fromStdString(engine_->active_content_atom()->subject.type_label);
}

QString KioskPresentationModel::contentAudio() const {
    if (!engine_) return QString();
    auto actions = engine_->active_presentation_actions();
    if (!actions.empty()) {
        const auto& path = actions.front().asset_path;
        if (path.ends_with(".wav") || path.ends_with(".ogg") || path.ends_with(".mp3")) {
            return QString::fromStdString(path);
        }
    }
    if (engine_->active_content_variant()) {
        for (const auto& media : engine_->active_content_variant()->presentation.media_refs) {
            if (media.ends_with(".wav") || media.ends_with(".ogg") || media.ends_with(".mp3")) {
                return QString::fromStdString(media);
            }
        }
    }
    return QString();
}

void KioskPresentationModel::playSound(const QString& soundPath) {
    if (soundPath.isEmpty()) return;
    QString resolved = soundPath;
    if (!QFile::exists(resolved)) {
        for (const auto& prefix : {"", "content/", "../content/", "../../content/", "content/assets/audio/", "assets/audio/"}) {
            QString cand = QString::fromUtf8(prefix) + soundPath;
            if (QFile::exists(cand)) {
                resolved = cand;
                break;
            }
        }
    }
    if (!QFile::exists(resolved)) return;

    for (const auto& player : {"pw-play", "paplay", "aplay"}) {
        if (QProcess::startDetached(QString::fromUtf8(player), {resolved})) {
            break;
        }
    }
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

QStringList KioskPresentationModel::explorationPaths() const {
    return contentOptions();
}

bool KioskPresentationModel::isRecipeActive() const {
    return engine_ && engine_->is_recipe_active();
}

QString KioskPresentationModel::selectionReason() const {
    if (!engine_) return QString();
    return QString::fromStdString(engine_->active_selection_reason().explanation);
}

void KioskPresentationModel::setAutoNavigationEnabled(bool enabled) {
    if (auto_navigation_enabled_ != enabled) {
        auto_navigation_enabled_ = enabled;
        emit autoNavigationEnabledChanged();
    }
}

QString KioskPresentationModel::behaviorStatus() const {
    if (!auto_navigation_enabled_) {
        return QStringLiteral("Navegação manual");
    }
    if (!engine_) {
        return QString();
    }
    switch (engine_->current_state()) {
        case experience::SessionState::IDLE:
            return QStringLiteral("Modo ambiente • Rotação contemplativa");
        case experience::SessionState::PRESENCE_DETECTED:
        case experience::SessionState::BIOMETRIC_SESSION:
        case experience::SessionState::IDENTITY_CANDIDATE:
        case experience::SessionState::IDENTITY_UNCERTAIN:
        case experience::SessionState::IDENTITY_UNKNOWN:
            return QStringLiteral("Acolhendo visitante...");
        case experience::SessionState::IDENTITY_SUPPORTED:
            return QStringLiteral("Visitante reconhecido");
        case experience::SessionState::CONTENT_ACTIVE:
            return QStringLiteral("Modo contemplativo • Navegação autônoma");
        case experience::SessionState::SESSION_COMPLETE:
            return QStringLiteral("Concluindo sessão");
        default:
            return QString();
    }
}

void KioskPresentationModel::resetBehaviorTimer() {
    state_duration_ = 0.0;
    behavior_progress_ = 0.0;
    emit behaviorProgressChanged();
}

void KioskPresentationModel::selectContextualContent(const QString& roleStr) {
    if (engine_) {
        auto role = content::parse_content_role(roleStr.toStdString());
        if (role == content::ContentRole::Unknown) role = content::ContentRole::Attract;
        auto res = engine_->select_contextual_content(role);
        if (!res) {
            advanceContent();
            return;
        }
        resetBehaviorTimer();
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::startRecipe(const QString& recipeId) {
    if (engine_) {
        auto res = engine_->start_recipe(recipeId.toStdString());
        if (!res) {
            advanceContent();
            return;
        }
        resetBehaviorTimer();
        emit contentChanged();
        emit stateChanged();
        auto audio = contentAudio();
        if (!audio.isEmpty()) {
            playSound(audio);
        }
    }
}

QString KioskPresentationModel::activeAtomId() const {
    if (engine_ && engine_->active_content_atom()) {
        return QString::fromStdString(engine_->active_content_atom()->content_id);
    }
    return QString();
}

void KioskPresentationModel::selectAtomDirectly(const QString& contentId) {
    if (engine_) {
        auto res = engine_->select_atom(contentId.toStdString());
        if (res) {
            current_content_ = contentId.toStdString();
            resetBehaviorTimer();
            emit contentChanged();
            emit stateChanged();
            auto audio = contentAudio();
            if (!audio.isEmpty()) {
                playSound(audio);
            }
        }
    }
}

void KioskPresentationModel::chooseOption(const QString& option) {
    if (!engine_) return;

    QString optTrimmed = option.trimmed();
    if (optTrimmed == QStringLiteral("Próxima Descoberta") ||
        optTrimmed == QStringLiteral("Avançar") ||
        optTrimmed == QStringLiteral("Próximo") ||
        optTrimmed == QStringLiteral("Avançar Conteúdo")) {
        advanceContent();
        return;
    }

    if (optTrimmed == QStringLiteral("Aprofundar") ||
        optTrimmed == QStringLiteral("Ver Relações") ||
        optTrimmed == QStringLiteral("Explorar mais") ||
        optTrimmed == QStringLiteral("Saiba mais") ||
        optTrimmed == QStringLiteral("Detalhes")) {
        deepenExperience();
        return;
    }

    if (auto catalog = engine_->content_catalog()) {
        const auto* atom = catalog->find_atom_by_name(optTrimmed.toStdString());
        if (atom) {
            selectAtomDirectly(QString::fromStdString(atom->content_id));
            return;
        }
    }

    if (engine_->is_recipe_active()) {
        engine_->advance_recipe(option.toStdString());
    } else {
        advanceContent();
    }
    resetBehaviorTimer();
    emit contentChanged();
    emit stateChanged();
    auto audio = contentAudio();
    if (!audio.isEmpty()) {
        playSound(audio);
    }
}

void KioskPresentationModel::deepenExperience() {
    if (engine_) {
        (void)engine_->select_contextual_content(content::ContentRole::Deepen);
        resetBehaviorTimer();
        emit contentChanged();
        emit stateChanged();
    }
}

void KioskPresentationModel::advanceContent() {
    if (engine_) {
        current_content_ = engine_->select_next_content();
        resetBehaviorTimer();
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
        resetBehaviorTimer();
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
        resetBehaviorTimer();
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
        state_duration_ = 0.0;
        absence_duration_ = 0.0;
        behavior_progress_ = 0.0;
        emit contentChanged();
        emit recognitionChanged();
        emit stateChanged();
        emit behaviorProgressChanged();
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
    if (present) {
        absence_duration_ = 0.0;
        if (engine_ && engine_->current_state() == experience::SessionState::IDLE) {
            recognition_resolved_ = false;
            pending_embeddings_.clear();
            pending_quality_sum_ = 0.0;
            state_duration_ = 0.0;
            behavior_progress_ = 0.0;
            greeting_title_ = QStringLiteral("Reconhecendo presença");
            greeting_message_ = QStringLiteral("Comparando somente a identidade facial local");
            engine_->on_presence_detected();
            engine_->begin_automatic_continuity();
            emit recognitionVisualStateChanged(false);
            emit biometricAuthorizationChanged(true);
            emit recognitionChanged();
            emit stateChanged();
            emit behaviorProgressChanged();
        }
    } else {
        absence_duration_ = 0.0;
    }
}

void KioskPresentationModel::onBehaviorTick() {
    tick(0.1);
}

void KioskPresentationModel::tick(double delta_seconds) {
    if (!auto_navigation_enabled_ || !engine_) {
        return;
    }

    const auto state = engine_->current_state();

    // 1. Ausência do visitante diante da câmera
    if (!face_detected_) {
        if (state != experience::SessionState::IDLE) {
            absence_duration_ += delta_seconds;
            // Se o visitante se afastou por 1.5s, conclui a sessão soberanamente
            if (absence_duration_ >= 1.5) {
                finishSession();
                return;
            }
        }
    } else {
        absence_duration_ = 0.0;
    }

    // 2. Máquina de estados temporal autônoma (sem mouse)
    state_duration_ += delta_seconds;

    switch (state) {
        case experience::SessionState::IDLE: {
            constexpr double kAmbientRotationDuration = 10.0;
            behavior_progress_ = std::clamp(state_duration_ / kAmbientRotationDuration, 0.0, 1.0);
            emit behaviorProgressChanged();
            if (state_duration_ >= kAmbientRotationDuration) {
                state_duration_ = 0.0;
                behavior_progress_ = 0.0;
                selectContextualContent(QStringLiteral("attract"));
            }
            break;
        }

        case experience::SessionState::PRESENCE_DETECTED:
        case experience::SessionState::BIOMETRIC_SESSION:
        case experience::SessionState::IDENTITY_CANDIDATE:
        case experience::SessionState::IDENTITY_UNCERTAIN:
        case experience::SessionState::IDENTITY_UNKNOWN: {
            constexpr double kBiometricGracePeriod = 2.5;
            behavior_progress_ = std::clamp(state_duration_ / kBiometricGracePeriod, 0.0, 1.0);
            emit behaviorProgressChanged();
            if (state_duration_ >= kBiometricGracePeriod) {
                state_duration_ = 0.0;
                behavior_progress_ = 0.0;
                startRecipe(QStringLiteral("discover_by_sound"));
            }
            break;
        }

        case experience::SessionState::IDENTITY_SUPPORTED: {
            constexpr double kGreetingDuration = 2.0;
            behavior_progress_ = std::clamp(state_duration_ / kGreetingDuration, 0.0, 1.0);
            emit behaviorProgressChanged();
            if (state_duration_ >= kGreetingDuration) {
                state_duration_ = 0.0;
                behavior_progress_ = 0.0;
                selectContextualContent(QStringLiteral("attract"));
            }
            break;
        }

        case experience::SessionState::CONTENT_ACTIVE: {
            // Se a receita foi concluída (ex: tela "Concluído"):
            if (engine_->is_recipe_active() && engine_->recipe_state().completed) {
                constexpr double kCompletionReadingDuration = 3.5;
                behavior_progress_ = std::clamp(state_duration_ / kCompletionReadingDuration, 0.0, 1.0);
                emit behaviorProgressChanged();
                if (state_duration_ >= kCompletionReadingDuration) {
                    state_duration_ = 0.0;
                    behavior_progress_ = 0.0;
                    advanceContent();
                }
                break;
            }

            const auto opts = contentOptions();
            if (!opts.isEmpty()) {
                // Modo exploratório / opções táteis de navegação ecológica: 8.5s
                constexpr double kExplorationStepDuration = 8.5;
                behavior_progress_ = std::clamp(state_duration_ / kExplorationStepDuration, 0.0, 1.0);
                emit behaviorProgressChanged();
                if (state_duration_ >= kExplorationStepDuration) {
                    state_duration_ = 0.0;
                    behavior_progress_ = 0.0;
                    advanceContent();
                }
            } else {
                // Modo contemplativo narrativo: 7.0s
                constexpr double kReadingStepDuration = 7.0;
                behavior_progress_ = std::clamp(state_duration_ / kReadingStepDuration, 0.0, 1.0);
                emit behaviorProgressChanged();
                if (state_duration_ >= kReadingStepDuration) {
                    state_duration_ = 0.0;
                    behavior_progress_ = 0.0;
                    if (engine_->is_recipe_active()) {
                        engine_->advance_recipe("");
                        emit contentChanged();
                        emit stateChanged();
                    } else {
                        advanceContent();
                    }
                }
            }
            break;
        }

        case experience::SessionState::SESSION_COMPLETE: {
            constexpr double kCompleteDuration = 3.5;
            behavior_progress_ = std::clamp(state_duration_ / kCompleteDuration, 0.0, 1.0);
            emit behaviorProgressChanged();
            if (state_duration_ >= kCompleteDuration) {
                finishSession();
            }
            break;
        }

        default:
            break;
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
        resetBehaviorTimer();
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

void KioskPresentationModel::tick(double delta_seconds) {
    if (!auto_navigation_enabled_ || !engine_) return;
    state_duration_ += delta_seconds;
    if (engine_->current_state() == experience::SessionState::CONTENT_ACTIVE && state_duration_ >= 8.0) {
        state_duration_ = 0.0;
        advanceContent();
    }
}

#endif

} // namespace elo::ui
