# ELO — Offline-First Personalized Interaction System

> **“Sempre pronto. Sempre incompleto.”**

Sistema autônomo e experimental concebido para oferecer experiências presenciais personalizadas em regime **offline-first**, preservando continuidade entre encontros sem exigir que a pessoa entregue ao sistema mais informação do que aquela estritamente necessária.

Consulte o documento normativo completo em: [`docs/constitution/ELO-CONSTITUTION-001.md`](docs/constitution/ELO-CONSTITUTION-001.md).

---

## 1. Princípios Constitucionais Fundamentais

```text
ELO ≠ ENTE
ELO ≠ JEV
ELO ≠ RECONHECIMENTO FACIAL
ELO ≠ RASPBERRY PI
ELO ≠ QT
```

- **Invariante E1 (SYSTEM_AUTONOMY)**: O ELO tem identidade, propósito, ciclo de vida e regras próprias.
- **Invariante E2 (ENTE_INDEPENDENCE)**: Consome `ente-kernel` (`ente::kernel`) estritamente por seu contrato constitutivo (`observe()`, `transform()`, `retire()`), sem vazar domínio para o kernel.
- **Invariantes E3 & E4 (JEV_INDEPENDENCE & JUDGMENT_IS_NOT_ACTION)**: Consome JEV para julgamentos probabilísticos tipados (`Score`, `Choice`, `Noul`). JEV julga, mas quem toma decisão operacional e age é o ELO.
- **Invariantes E5 & E6 (OFFLINE_PRIMARY_OPERATION & LOCAL_GUI_AVAILABILITY)**: `NETWORK = OFF` é o estado padrão. Boot, interface, biometria, experiência, sorteios, questionários e persistência funcionam 100% locais e desconectados.
- **Invariante E7 (PRESENTATION_DOMAIN_SEPARATION)**: QML restringe-se à apresentação através de `KioskPresentationModel`. Lógica de domínio, biometria e SQLite nunca entram no QML.
- **Invariantes E8, E9 & E10 (PERSON_AUTONOMY, NO_SILENT_ENROLLMENT & RAW_IMAGE_EPHEMERALITY)**: Participação consentida, imagens em RAM destruídas após extração de embedding (`EphemeralFrame`), sem retenção de fotos brutas.
- **Invariante E12 & E13 (BIOMETRIC_IS_EVIDENCE & UNKNOWN_IS_VALID)**: Correspondência facial é evidência probabilística. Estados: `UNKNOWN`, `CANDIDATE`, `SUPPORTED`, `UNCERTAIN`.
- **Invariante E14 (LOCAL_FORGETTING)**: Direito local ao esquecimento instantâneo (`FORGET(Pxx)`) sem necessidade de conexão externa.
- **Invariante E15 (IDENTITY_SPACE_SEPARATION)**: Separação forte de tipos:
  `EloId` (`elo://...`) $\neq$ `KernelId` $\neq$ `DeviceId` (`device://...`) $\neq$ `PersonLocalId` (`person-local://...`).
- **Invariante E17 (RANDOMIZATION_INDEPENDENCE)**: Sorteios locais são estatisticamente independentes da aparência ou qualidade facial.

---

## 2. Estrutura do Repositório

```text
elo/
├── CMakeLists.txt              # Configuração global (C++26, Ninja, Qt6)
├── docs/constitution/          # ELO-CONSTITUTION-001 (documento normativo)
├── external/
│   ├── ente-kernel/            # Integração limpa com ente::kernel
│   └── jev/                    # Contrato de julgamento probabilístico (Score, Choice, Noul)
├── include/elo/
│   ├── core/                   # Invariantes, tipos fundamentais, Result<T, Error>
│   ├── identity/               # Espaços de identidade estritamente separados
│   ├── perception/             # EphemeralFrame (RAII para descarte seguro de frames)
│   ├── biometric/              # FaceTemplate, IdentityHypothesis, BiometricMatcher
│   ├── judgment/               # JevAdapter tipado
│   ├── experience/             # ExperienceEngine e máquina de estados da sessão
│   ├── survey/                 # Questionários e políticas de desvinculação de resposta
│   ├── storage/                # IBiometricStore, IExperienceStore, ISurveyStore
│   └── ui/                     # KioskPresentationModel (fronteira C++/QML)
├── src/                        # Implementações em C++26
├── ui/                         # Qt Quick / QML (Interface Kiosk/Totem offline)
│   ├── Main.qml
│   └── qml.qrc
├── apps/elo-kiosk/             # Executável do totem presencial
└── tests/                      # Validação de invariantes (E1–E22) e experimentos (EXP-001–014)
```

---

## 3. Compilação e Execução

### Pré-requisitos
- Compilador C++ com suporte a **C++26** (ex: GCC 16+)
- CMake 3.25+ e Ninja
- Qt 6 (Core, Gui, Quick, Qml)

### Build
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
ninja -C build
```

### Executar Testes Constitucionais
```bash
ctest --test-dir build --output-on-failure
# ou diretamente:
./build/tests/test_constitutional_invariants
```

### Executar Totem Kiosk
```bash
./build/apps/elo-kiosk/elo-kiosk
```
