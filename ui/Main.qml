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
    color: "#0f172a"

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
                text: !cameraAvailable || !kioskModel.visionOperational
                      ? "● CÂMERA INDISPONÍVEL"
                      : (kioskModel.recognitionResolved
                         ? "● IDENTIDADE RECONHECIDA"
                         : (kioskModel.faceDetected ? "● RECONHECENDO" : "● VISÃO ATIVA"))
                font.pixelSize: 12
                font.bold: true
                color: kioskModel.visionOperational ? "#34d399" : "#f87171"
            }
        }
    }

    Item {
        id: contentArea
        anchors.top: headerBar.bottom
        anchors.bottom: footerBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 40

        // Discrete operational preview: explains what the totem sees but
        // is never the primary content of the experience.
        Rectangle {
            id: cameraPreview
            anchors.top: parent.top
            anchors.right: parent.right
            width: 230
            height: 150
            radius: 8
            color: "#020617"
            border.color: "#334155"
            border.width: 1
            clip: true
            opacity: 0.78
            z: 10

            Image {
                anchors.fill: parent
                source: kioskModel.cameraFrameRevision > 0
                        ? ("image://camera/live?" + kioskModel.cameraFrameRevision)
                        : ""
                cache: false
                fillMode: Image.PreserveAspectCrop
                visible: cameraAvailable && kioskModel.visionOperational &&
                         kioskModel.cameraFrameRevision > 0
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 28
                color: "#b3020617"

                Text {
                    anchors.centerIn: parent
                    text: !cameraAvailable || !kioskModel.visionOperational
                          ? "câmera indisponível"
                          : (kioskModel.recognitionResolved
                             ? "identidade reconhecida"
                             : (kioskModel.faceDetected ? "reconhecendo localmente" : "aguardando presença"))
                    font.pixelSize: 11
                    color: kioskModel.recognitionResolved ? "#34d399" : "#cbd5e1"
                }
            }
        }

        // Estado IDLE: Modo contemplativo e atrativo do Bioma Pampa
        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.85, 840)
            spacing: 20
            visible: kioskModel.currentState === "IDLE"

            Text {
                text: kioskModel.contentTitle.length > 0 ? kioskModel.contentTitle : "Bioma Pampa"
                font.pixelSize: 42
                font.bold: true
                color: "#f8fafc"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: idleDescText.implicitHeight + 36
                color: "#1e293b"
                radius: 12
                border.color: "#334155"
                border.width: 1

                Text {
                    id: idleDescText
                    anchors.centerIn: parent
                    width: parent.width - 48
                    text: kioskModel.contentText.length > 0
                          ? kioskModel.contentText
                          : "Aproxime-se. O totem reconhece sua presença e navega automaticamente pelo ecossistema."
                    font.pixelSize: 20
                    color: "#38bdf8"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    lineHeight: 1.3
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12

                Rectangle {
                    width: 220
                    height: 36
                    radius: 18
                    color: "#0f172a"
                    border.color: "#38bdf8"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "● Aproxime-se do totem"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#38bdf8"
                    }
                }

                Rectangle {
                    width: 240
                    height: 36
                    radius: 18
                    color: "#0f172a"
                    border.color: "#334155"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Autônomo • Não requer mouse"
                        font.pixelSize: 12
                        color: "#94a3b8"
                    }
                }
            }

            Text {
                text: "Processamento local soberano • nenhuma fotografia armazenada • somente representação facial vetorial"
                font.pixelSize: 13
                color: "#64748b"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // Estado de Presença e Reconhecimento Facial
        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.75, 760)
            spacing: 22
            visible: kioskModel.currentState === "PRESENCE_DETECTED" ||
                     kioskModel.currentState === "BIOMETRIC_SESSION" ||
                     kioskModel.currentState === "IDENTITY_UNKNOWN" ||
                     kioskModel.currentState === "IDENTITY_CANDIDATE" ||
                     kioskModel.currentState === "IDENTITY_UNCERTAIN" ||
                     kioskModel.currentState === "IDENTITY_SUPPORTED"

            BusyIndicator {
                running: !kioskModel.recognitionResolved
                visible: running
                Layout.preferredWidth: 56
                Layout.preferredHeight: 56
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: kioskModel.greetingTitle
                font.pixelSize: 38
                font.bold: true
                color: kioskModel.recognitionResolved ? "#f8fafc" : "#cbd5e1"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: kioskModel.greetingMessage
                font.pixelSize: 19
                color: kioskModel.recognitionResolved ? "#34d399" : "#94a3b8"
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // Barra de ritmo comportamental de acolhimento
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 320
                Layout.preferredHeight: 4
                radius: 2
                color: "#1e293b"

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width * kioskModel.behaviorProgress
                    radius: 2
                    color: "#38bdf8"
                }
            }

            Text {
                visible: kioskModel.recognitionResolved
                text: kioskModel.activePerson
                font.pixelSize: 12
                color: "#64748b"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // Estado de Conteúdo Ativo: Modo Contemplativo, Fotografia Hero e Navegação Autônoma
        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.90, 880)
            spacing: 16
            visible: kioskModel.currentState === "CONTENT_ACTIVE"

            // Cabeçalho da Entidade / Espécie
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 6

                Text {
                    text: kioskModel.contentTitle.length > 0 ? kioskModel.contentTitle : "Bioma Pampa"
                    font.pixelSize: 36
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 10
                    visible: kioskModel.contentScientificName.length > 0 || kioskModel.contentTypeLabel.length > 0

                    Text {
                        visible: kioskModel.contentScientificName.length > 0
                        text: kioskModel.contentScientificName
                        font.pixelSize: 17
                        font.italic: true
                        color: "#38bdf8"
                    }

                    Rectangle {
                        visible: kioskModel.contentTypeLabel.length > 0
                        height: 24
                        width: typeLabelText.implicitWidth + 18
                        radius: 12
                        color: "#0f172a"
                        border.color: "#334155"
                        border.width: 1

                        Text {
                            id: typeLabelText
                            anchors.centerIn: parent
                            text: kioskModel.contentTypeLabel
                            font.pixelSize: 11
                            font.bold: true
                            color: "#94a3b8"
                        }
                    }
                }
            }

            // Cartão de Mídia Visual (Fotografia Científica Hero)
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width * 0.92, 760)
                Layout.preferredHeight: 320
                radius: 16
                color: "#0b1329"
                border.color: "#1e293b"
                border.width: 1
                clip: true

                // Imagem carregada
                Image {
                    id: heroImage
                    anchors.fill: parent
                    anchors.margins: 8
                    fillMode: Image.PreserveAspectFit
                    source: kioskModel.contentImage
                    visible: kioskModel.hasImage
                    smooth: true
                    asynchronous: true
                }

                // Placeholder estético e contemplativo quando a imagem ainda não está em disco
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: !kioskModel.hasImage

                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 72
                        height: 72
                        radius: 36
                        color: "#1e293b"
                        border.color: "#38bdf8"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "🌾"
                            font.pixelSize: 32
                        }
                    }

                    Text {
                        text: kioskModel.contentTitle
                        font.pixelSize: 20
                        font.bold: true
                        color: "#e2e8f0"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: "Acervo e Biodiversidade do Pampa"
                        font.pixelSize: 13
                        color: "#64748b"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                // Indicador sonoro sutil (caso haja áudio autêntico)
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.margins: 14
                    height: 28
                    width: audioLabelText.implicitWidth + 30
                    radius: 14
                    color: "#cc090d16"
                    border.color: "#059669"
                    border.width: 1
                    visible: kioskModel.contentAudio.length > 0

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6

                        Text {
                            text: "●"
                            font.pixelSize: 10
                            color: "#10b981"
                        }
                        Text {
                            id: audioLabelText
                            text: "Paisagem Sonora"
                            font.pixelSize: 11
                            font.bold: true
                            color: "#34d399"
                        }
                    }
                }
            }

            // Bloco de Narrativa Ecológica
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width * 0.92, 760)
                Layout.preferredHeight: contentDescText.implicitHeight + 36
                color: "#131d36"
                radius: 12
                border.color: "#1e293b"
                border.width: 1

                Text {
                    id: contentDescText
                    anchors.centerIn: parent
                    width: parent.width - 44
                    text: kioskModel.contentText
                    font.pixelSize: 18
                    color: "#e2e8f0"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    lineHeight: 1.35
                }
            }

            // Trilhas de Exploração e Relações Ecológicas (Chips Horizontais, NÃO formato quiz)
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 12
                visible: kioskModel.contentOptions.length > 0

                Repeater {
                    model: kioskModel.contentOptions

                    Rectangle {
                        Layout.preferredWidth: Math.max(chipText.implicitWidth + 36, 140)
                        Layout.preferredHeight: 44
                        radius: 22
                        color: chipArea.pressed ? "#0284c7" : (chipArea.containsMouse ? "#1e293b" : "#0f172a")
                        border.color: chipArea.containsMouse ? "#38bdf8" : "#334155"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 6

                            Text {
                                text: "✦"
                                font.pixelSize: 12
                                color: "#38bdf8"
                            }

                            Text {
                                id: chipText
                                text: modelData
                                font.pixelSize: 15
                                font.bold: true
                                color: "#f8fafc"
                            }
                        }

                        MouseArea {
                            id: chipArea
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: kioskModel.chooseOption(modelData)
                        }
                    }
                }
            }

            // Barra de Ritmo e Progresso Comportamental
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(parent.width * 0.92, 760)
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: kioskModel.behaviorStatus
                        font.pixelSize: 12
                        color: "#94a3b8"
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "Navegação contínua • Toque na tela para explorar"
                        font.pixelSize: 12
                        color: "#64748b"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 5
                    radius: 3
                    color: "#1e293b"
                    border.color: "#334155"
                    border.width: 1

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width * kioskModel.behaviorProgress
                        radius: 3
                        color: "#38bdf8"
                    }
                }
            }
        }

        // Conclusão da sessão (retorno automático por ausência ou tempo)
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20
            visible: kioskModel.currentState === "SESSION_COMPLETE"

            Text {
                text: "Sessão concluída"
                font.pixelSize: 32
                font.bold: true
                color: "#34d399"
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Imagens transitórias foram descartadas da memória."
                font.pixelSize: 16
                color: "#94a3b8"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 260
                Layout.preferredHeight: 4
                radius: 2
                color: "#1e293b"

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width * kioskModel.behaviorProgress
                    radius: 2
                    color: "#34d399"
                }
            }
        }
    }

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
                text: "Estado: " + kioskModel.currentState + " • " + kioskModel.behaviorStatus
                font.pixelSize: 12
                color: "#64748b"
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: kioskModel.activePerson !== "UNKNOWN"
                width: 210
                height: 32
                radius: 16
                color: forgetArea.pressed ? "#7f1d1d" : "#1e293b"
                border.color: forgetArea.containsMouse ? "#f87171" : "#334155"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "Esquecer minha identidade"
                    font.pixelSize: 11
                    color: forgetArea.containsMouse ? "#f87171" : "#94a3b8"
                }

                MouseArea {
                    id: forgetArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: kioskModel.requestForgetMe()
                }
            }
        }
    }
}
