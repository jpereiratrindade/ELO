import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: rootWindow
    visible: true
    visibility: Window.FullScreen
    width: 1024
    height: 768
    title: "ELO — Totem de Experiência Presencial"
    color: "#0f172a" // Deep slate background

    // Invariant E6 & E7: Offline Local GUI, Domain-Presentation Separation
    Rectangle {
        id: mainContainer
        anchors.fill: parent
        color: "transparent"

        // Top Status Header (Offline local-first indicator, Invariant E5)
        Rectangle {
            id: headerBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 64
            color: "#1e293b"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24

                Text {
                    text: "ELO"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#38bdf8"
                }

                Text {
                    text: "•  Sempre pronto. Sempre incompleto."
                    font.pixelSize: 14
                    color: "#94a3b8"
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 130
                    height: 28
                    radius: 14
                    color: "#065f46"

                    Text {
                        anchors.centerIn: parent
                        text: "● OFFLINE LOCAL"
                        font.pixelSize: 11
                        font.bold: true
                        color: "#34d399"
                    }
                }

                Text {
                    text: "Kernel Gen: " + kioskModel.kernelGeneration
                    font.pixelSize: 12
                    color: "#64748b"
                }

                Text {
                    text: cameraAvailable && kioskModel.visionOperational
                          ? (kioskModel.faceDetected ? "● ROSTO DETECTADO" : "● VISÃO ATIVA")
                          : "● CÂMERA INDISPONÍVEL"
                    font.pixelSize: 12
                    font.bold: true
                    color: cameraAvailable && kioskModel.visionOperational ? "#34d399" : "#f87171"
                    elide: Text.ElideRight
                    Layout.maximumWidth: 180
                }

                Text {
                    text: "ID: " + kioskModel.activePerson
                    font.pixelSize: 12
                    font.bold: true
                    color: kioskModel.activePerson === "UNKNOWN" ? "#94a3b8" : "#fbbf24"
                }
            }
        }

        // Center Content Screen Area based on kioskModel.currentState
        Item {
            anchors.top: headerBar.bottom
            anchors.bottom: footerBar.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 40

            // 1. IDLE: the fixed camera remains active and presence is automatic.
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 16
                visible: kioskModel.currentState === "IDLE"

                Text {
                    text: "ELO está pronto"
                    font.pixelSize: 32
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.preferredWidth: 560
                    Layout.preferredHeight: 360
                    Layout.alignment: Qt.AlignHCenter
                    color: "#020617"
                    radius: 16
                    clip: true
                    border.color: kioskModel.faceDetected ? "#34d399" : "#334155"
                    border.width: 2

                    Image {
                        anchors.fill: parent
                        source: "image://camera/live?" + kioskModel.cameraFrameRevision
                        cache: false
                        fillMode: Image.PreserveAspectCrop
                        visible: cameraAvailable && kioskModel.visionOperational &&
                                 kioskModel.cameraFrameRevision > 0
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: !cameraAvailable || !kioskModel.visionOperational ||
                                 kioskModel.cameraFrameRevision === 0
                        text: cameraAvailable ? kioskModel.visionStatus : "Câmera indisponível"
                        color: "#94a3b8"
                        font.pixelSize: 16
                    }
                }

                Text {
                    text: kioskModel.faceDetected
                          ? "Presença reconhecida — preparando experiência"
                          : "Aproxime-se. A detecção acontece automaticamente."
                    font.pixelSize: 18
                    color: kioskModel.faceDetected ? "#34d399" : "#94a3b8"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // 2. PRESENCE_DETECTED & CONSENT Screen
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                width: 700
                visible: kioskModel.currentState === "PRESENCE_DETECTED" || kioskModel.currentState === "CONSENT_PENDING"

                Text {
                    text: "Transparência e Consentimento"
                    font.pixelSize: 30
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    color: "#1e293b"
                    radius: 12
                    border.color: "#334155"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        Text {
                            text: "• Processamento 100% local (sem nuvem ou internet)."
                            font.pixelSize: 15
                            color: "#cbd5e1"
                        }
                        Text {
                            text: "• Não salvamos fotos ou gravações do seu rosto por padrão."
                            font.pixelSize: 15
                            color: "#cbd5e1"
                        }
                        Text {
                            text: "• Você não precisa fornecer nome, CPF ou documentos."
                            font.pixelSize: 15
                            color: "#cbd5e1"
                        }
                        Text {
                            text: "• O reconhecimento facial é usado apenas para continuidade local desta experiência."
                            font.pixelSize: 15
                            color: "#cbd5e1"
                        }
                        Text {
                            text: "• Você pode solicitar esquecimento dos seus dados a qualquer momento."
                            font.pixelSize: 15
                            color: "#cbd5e1"
                        }
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20

                    Button {
                        Layout.preferredWidth: 250
                        Layout.preferredHeight: 56
                        text: "Autorizar Continuidade Local"
                        onClicked: kioskModel.chooseBiometricConsent(true)
                    }

                    Button {
                        Layout.preferredWidth: 250
                        Layout.preferredHeight: 56
                        text: "Continuar Sem Ser Lembrado"
                        onClicked: kioskModel.chooseBiometricConsent(false)
                    }

                    Button {
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 56
                        text: "Não Participar"
                        onClicked: kioskModel.chooseParticipation(false)
                    }
                }
            }

            // 3. BIOMETRIC_SESSION / IDENTITY RECOGNITION Screen
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                visible: kioskModel.currentState === "BIOMETRIC_SESSION" ||
                         kioskModel.currentState === "IDENTITY_SUPPORTED" ||
                         kioskModel.currentState === "IDENTITY_CANDIDATE" ||
                         kioskModel.currentState === "IDENTITY_UNCERTAIN" ||
                         kioskModel.currentState === "IDENTITY_UNKNOWN"

                Text {
                    text: kioskModel.currentState === "IDENTITY_SUPPORTED" ? "Continuidade Reconhecida!" :
                          kioskModel.currentState === "IDENTITY_UNKNOWN" ? "Novo Participante Local" :
                          "Identificando localmente..."
                    font.pixelSize: 28
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.preferredWidth: 560
                    Layout.preferredHeight: 360
                    Layout.alignment: Qt.AlignHCenter
                    color: "#020617"
                    radius: 16
                    clip: true
                    border.color: "#34d399"
                    border.width: 2

                    Image {
                        anchors.fill: parent
                        source: "image://camera/live?" + kioskModel.cameraFrameRevision
                        cache: false
                        fillMode: Image.PreserveAspectCrop
                    }
                }

                Text {
                    text: kioskModel.visionStatus
                    font.pixelSize: 16
                    color: "#38bdf8"
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // 3b. Anonymous participation remains possible without enrollment.
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                visible: kioskModel.currentState === "NON_BIOMETRIC_SESSION"

                Text {
                    text: "Sessão sem memória biométrica"
                    font.pixelSize: 30
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Nenhuma identidade local será criada para esta sessão."
                    font.pixelSize: 17
                    color: "#94a3b8"
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 58
                    text: "Iniciar experiência"
                    onClicked: kioskModel.advanceContent()
                }
            }

            // 4. CONTENT_ACTIVE Screen
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                visible: kioskModel.currentState === "CONTENT_ACTIVE"

                Text {
                    text: "Experiência em Andamento"
                    font.pixelSize: 28
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Rectangle {
                    Layout.preferredWidth: 500
                    Layout.preferredHeight: 120
                    color: "#1e293b"
                    radius: 12

                    Text {
                        anchors.centerIn: parent
                        text: "Conteúdo Apresentado:\n" + kioskModel.currentContent
                        font.pixelSize: 18
                        font.bold: true
                        color: "#38bdf8"
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 20

                    Button {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 56
                        text: "Próximo Conteúdo"
                        onClicked: kioskModel.advanceContent()
                    }

                    Button {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 56
                        text: "Concluir Sessão"
                        onClicked: kioskModel.finishSession()
                    }
                }
            }

            // 5. SESSION_COMPLETE Screen
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                visible: kioskModel.currentState === "SESSION_COMPLETE"

                Text {
                    text: "Sessão Concluída com Sucesso!"
                    font.pixelSize: 30
                    font.bold: true
                    color: "#34d399"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Imagens transitórias foram descartadas da memória RAM."
                    font.pixelSize: 16
                    color: "#94a3b8"
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 56
                    text: "Retornar ao Início"
                    onClicked: kioskModel.finishSession()
                }
            }
        }

        // Bottom Footer Bar (Forgetting & Privacy Actions)
        Rectangle {
            id: footerBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 56
            color: "#0f172a"
            border.color: "#1e293b"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 24
                anchors.rightMargin: 24

                Text {
                    text: "Estado da Aplicação: " + kioskModel.currentState
                    font.pixelSize: 13
                    color: "#64748b"
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Direito ao Esquecimento (Esquecer Meus Dados)"
                    visible: kioskModel.activePerson !== "UNKNOWN"
                    onClicked: kioskModel.requestForgetMe()
                }

                Button {
                    text: "Encerrar Sessão"
                    visible: kioskModel.currentState !== "IDLE"
                    onClicked: kioskModel.finishSession()
                }
            }
        }
    }
}
