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
- **Invariante E2 (ENTE_INDEPENDENCE)**: Consome `ente-kernel` (`ente::kernel`) estritamente por seu contrato constitutivo. Eventos e transições de sessão pertencem ao ELO e não alteram a geração do kernel; `transform()` fica reservado a mudanças constitutivas identificadas da própria realização.
- **Invariantes E3 & E4 (JEV_INDEPENDENCE & JUDGMENT_IS_NOT_ACTION)**: Consome JEV para julgamentos probabilísticos tipados (`Score`, `Choice`, `Noul`). JEV julga, mas quem toma decisão operacional e age é o ELO.
- **Invariantes E5 & E6 (OFFLINE_PRIMARY_OPERATION & LOCAL_GUI_AVAILABILITY)**: estabelecem como requisito que `NETWORK = OFF` seja o estado padrão e que boot, interface, biometria, experiência, sorteios, questionários e persistência funcionem localmente.
- **Invariante E7 (PRESENTATION_DOMAIN_SEPARATION)**: QML restringe-se à apresentação através de `KioskPresentationModel`. Lógica de domínio, biometria e SQLite nunca entram no QML.
- **Invariantes E8, E9 & E10 (PERSON_AUTONOMY, VISIBLE_LOCAL_ENROLLMENT & RAW_IMAGE_EPHEMERALITY)**: reconhecimento local automático e visível, possibilidade imediata de esquecimento e imagens destruídas após extração do embedding, sem retenção de fotos brutas.
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
│   ├── ente-kernel/            # Snapshot vendorizado temporário de ente::kernel
│   └── jev/                    # Stub local do contrato JEV (Score, Choice, Noul)
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
└── tests/                      # Checks constitucionais e experimentos já implementados
```

### Estado demonstrado nesta versão

O código atual demonstra separação de identidades, captura contínua local, detecção
facial YuNet, embeddings SFace locais, estados `UNKNOWN`/`UNCERTAIN`, cadastro
automático e visível, persistência SQLite após
reinício, esquecimento local, separação entre julgamento JEV e ação, e independência
entre transições de sessão e geração do kernel.

Ainda não estão demonstrados: execução física no Raspberry Pi 5, calibração dos
limiares biométricos no ambiente final, consumo versionado dos repositórios externos,
nem a totalidade de E1–E22 e EXP-ELO-001–014. `external/ente-kernel` é um snapshot
vendorizado e `external/jev` é um stub de contrato até que as dependências independentes
sejam conectadas.

---

## 3. Compilação e Execução

### Pré-requisitos
- Compilador C++ com suporte a **C++26** (ex: GCC 16+)
- CMake 3.25+ e Ninja
- Qt 6 (Core, Gui, Quick, Qml)
- OpenCV 4.8+ (Core, ImgProc, ObjDetect, VideoIO, DNN)
- SQLite 3
- `libcamera` e backend GStreamer correspondente para câmeras CSI no Raspberry Pi 5

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

### Operação como totem

Não há escolha interativa de câmera nem botão de aproximação. O dispositivo físico do
totem é descoberto no boot, a captura inicia imediatamente e o detector permanece pronto
para observar presença. Quando um único rosto é sustentado por frames consecutivos, a
interface inicia automaticamente o reconhecimento local.

```bash
./build/apps/elo-kiosk/elo-kiosk
```

O ELO compara o embedding com os templates locais. Se não houver correspondência
compatível, cria automaticamente `person-local://Pxx` e apresenta “Bem-vindo ao ELO”.
Se houver correspondência, apresenta “Que bom ver você novamente” e retoma a
continuidade. Frames brutos não são gravados: somente o vetor facial derivado, o
histórico e os vínculos estritamente necessários são persistidos no SQLite local.
Cada decisão combina três embeddings transitórios consecutivos. Em um retorno
reconhecido, o vetor agregado atual refina o único template do mesmo `PersonLocalId`,
tornando a representação local mais robusta a variações futuras sem acumular templates
nem armazenar fotografias.

Configurações de implantação, não opções apresentadas à pessoa:

```bash
# Fixar uma câmera na imagem/configuração da unidade do totem
ELO_CAMERA_ID=/dev/v4l/by-id/... ./build/apps/elo-kiosk/elo-kiosk

# Fixar o diretório local persistente
ELO_DATA_DIR=/var/lib/elo ./build/apps/elo-kiosk/elo-kiosk
```

No Linux, o build prefere `libcamera` quando seus headers estão disponíveis; caso
contrário usa V4L2 para webcams USB. Os modelos YuNet e SFace são baixados durante a
configuração do CMake com versões e hashes fixados. Em uma imagem de produção offline,
use `ELO_FACE_DETECTOR_MODEL` e `ELO_FACE_RECOGNIZER_MODEL` para apontar aos artefatos
pré-instalados e configure `ELO_DOWNLOAD_VISION_MODELS=OFF`.

### Teste da interface e da continuidade

1. Inicie `./build/apps/elo-kiosk/elo-kiosk` com uma câmera conectada.
2. Confirme que o vídeo aparece sem clicar em qualquer botão e aproxime um único rosto.
3. Confirme que a prévia discreta no canto superior direito muda de “reconhecendo” para
   “identidade reconhecida”, sem solicitar confirmação.
4. Na primeira ocorrência, confirme a mensagem **Bem-vindo ao ELO** e a criação de um
   `person-local://Pxx`; nenhuma fotografia deve ser criada no diretório de dados.
5. Encerre e abra o aplicativo novamente usando o mesmo `ELO_DATA_DIR`. O mesmo rosto
   deve receber **Que bom ver você novamente**, recuperar o identificador e avançar
   para o próximo conteúdo.
6. Acione **Direito ao Esquecimento**, reinicie e confirme que a identidade anterior
   não é recuperada.

Para um ensaio descartável, aponte `ELO_DATA_DIR` para um diretório temporário dedicado.
Se mais de uma pessoa estiver no enquadramento, o sistema deve pedir que apenas uma
permaneça e não deve cadastrar um template até o enquadramento ficar inequívoco.
