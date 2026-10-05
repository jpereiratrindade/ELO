import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: rootWindow
    visible: true
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
                    text: cameraSelected
                          ? "Camera: " + selectedCameraName + " [" + selectedCameraBackend + "]"
                          : "Camera: não selecionada"
                    font.pixelSize: 12
                    color: cameraSelected ? "#34d399" : "#f59e0b"
                    elide: Text.ElideRight
                    Layout.maximumWidth: 260
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

            // 1. IDLE Screen
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 24
                visible: kioskModel.currentState === "IDLE"

                Text {
                    text: "Bem-vindo ao ELO"
                    font.pixelSize: 36
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Aproxime-se para iniciar uma experiência personalizada local."
                    font.pixelSize: 18
                    color: "#94a3b8"
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 64
                    text: "Aproximar / Iniciar"
                    onClicked: kioskModel.userApproached()
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
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 56
                        text: "Usar Biometria Local"
                        onClicked: kioskModel.chooseBiometricConsent(true)
                    }

                    Button {
                        Layout.preferredWidth: 220
                        Layout.preferredHeight: 56
                        text: "Sessão Anônima (Sem Rosto)"
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
                         kioskModel.currentState === "IDENTITY_UNKNOWN"

                Text {
                    text: kioskModel.currentState === "IDENTITY_SUPPORTED" ? "Continuidade Reconhecida!" :
                          kioskModel.currentState === "IDENTITY_UNKNOWN" ? "Novo Participante Local" :
                          "Avaliando Presença Biométrica..."
                    font.pixelSize: 28
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "Identificador Local: " + kioskModel.activePerson
                    font.pixelSize: 16
                    color: "#38bdf8"
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 60
                    text: "Ver Meu Conteúdo"
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
