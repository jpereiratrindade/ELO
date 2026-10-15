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

    // Atalhos de teclado para saída e controle operacional
    Shortcut {
        sequence: "Escape"
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (rootWindow.visibility === Window.FullScreen) {
                rootWindow.visibility = Window.Windowed
            } else {
                rootWindow.visibility = Window.FullScreen
            }
        }
    }

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
                text: "•  " + (kioskModel.bundleTitle.length > 0 ? kioskModel.bundleTitle : "Sempre pronto. Sempre incompleto.")
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
                      ? "● SENSOR INDISPONÍVEL"
                      : (kioskModel.faceDetected
                         ? "● PRESENÇA DETECTADA"
                         : "● AGUARDANDO VISITANTE")
                font.pixelSize: 12
                font.bold: true
                color: kioskModel.visionOperational ? (kioskModel.faceDetected ? "#38bdf8" : "#94a3b8") : "#f87171"
            }
        }
    }

    Item {
        id: contentArea
        anchors.top: headerBar.bottom
        anchors.bottom: footerBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 32
        anchors.rightMargin: 32
        anchors.topMargin: 20
        anchors.bottomMargin: 20

        // Discrete operational preview: explains what the totem sees but
        // is never the primary content of the experience.
        Rectangle {
            id: cameraPreview
            anchors.top: parent.top
            anchors.right: parent.right
            width: 140
            height: 92
            radius: 8
            color: "#020617"
            border.color: kioskModel.faceDetected ? "#38bdf8" : "#334155"
            border.width: 1
            clip: true
            opacity: 0.82
            z: 20

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
                height: 22
                color: "#cc020617"

                Text {
                    anchors.centerIn: parent
                    text: !cameraAvailable || !kioskModel.visionOperational
                          ? "sensor inativo"
                          : (kioskModel.faceDetected ? "presença ativa" : "aguardando")
                    font.pixelSize: 10
                    font.bold: true
                    color: kioskModel.faceDetected ? "#38bdf8" : "#94a3b8"
                }
            }
        }

        // Componente discreto de acolhimento e estado da interação
        Rectangle {
            id: interactionStatePill
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(contentArea.width * 0.55, 520)
            height: 42
            radius: 21
            color: "#1e293b"
            border.color: "#38bdf8"
            border.width: 1
            z: 25
            visible: kioskModel.currentState === "PRESENCE_DETECTED"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: "#38bdf8"
                }

                Text {
                    text: kioskModel.greetingTitle + " • " + kioskModel.greetingMessage
                    font.pixelSize: 12
                    font.bold: true
                    color: "#f8fafc"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.preferredWidth: 64
                    Layout.preferredHeight: 4
                    radius: 2
                    color: "#0f172a"

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width * kioskModel.behaviorProgress
                        radius: 2
                        color: "#38bdf8"
                    }
                }
            }
        }

        // Conteúdo Dinâmico (Modo IDLE Ambient Attract, PRESENCE_DETECTED & Modo CONTENT_ACTIVE Interativo)
        ColumnLayout {
            anchors.fill: parent
            spacing: 12
            visible: kioskModel.currentState === "CONTENT_ACTIVE" || kioskModel.currentState === "IDLE" || kioskModel.currentState === "PRESENCE_DETECTED"

            // Cabeçalho do Conteúdo Ativo
            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: kioskModel.contentTitle.length > 0 ? kioskModel.contentTitle : "Exploração do Acervo"
                    font.pixelSize: Math.max(28, Math.min(contentArea.width * 0.026, 44))
                    font.bold: true
                    color: "#f8fafc"
                    Layout.alignment: Qt.AlignHCenter
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12
                    visible: kioskModel.contentScientificName.length > 0 || kioskModel.contentTypeLabel.length > 0

                    Text {
                        visible: kioskModel.contentScientificName.length > 0
                        text: kioskModel.contentScientificName
                        font.pixelSize: Math.max(15, Math.min(contentArea.width * 0.012, 18))
                        font.italic: true
                        color: "#38bdf8"
                    }

                    Rectangle {
                        visible: kioskModel.contentTypeLabel.length > 0
                        height: 26
                        width: typeLabelText.implicitWidth + 20
                        radius: 13
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

                    Rectangle {
                        visible: kioskModel.currentState === "IDLE"
                        height: 26
                        width: idleBadgeText.implicitWidth + 20
                        radius: 13
                        color: "#064e3b"
                        border.color: "#059669"
                        border.width: 1

                        Text {
                            id: idleBadgeText
                            anchors.centerIn: parent
                            text: "✦ Modo Ambiente • Aproxime-se"
                            font.pixelSize: 11
                            font.bold: true
                            color: "#34d399"
                        }
                    }
                }
            }

            // Cartão de Mídia Visual Hero (Ocupa o máximo de espaço dinamicamente)
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 280
                radius: 18
                color: "#080e1e"
                border.color: "#334155"
                border.width: 1.5
                clip: true

                // Imagem carregada (PreserveAspectFit em tela cheia)
                Image {
                    id: heroImage
                    anchors.fill: parent
                    anchors.margins: 12
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
                        width: 76
                        height: 76
                        radius: 38
                        color: "#1e293b"
                        border.color: "#38bdf8"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: "◈"
                            font.pixelSize: 36
                            color: "#38bdf8"
                        }
                    }

                    Text {
                        text: kioskModel.contentTitle.length > 0 ? kioskModel.contentTitle : "Átomo de Conteúdo"
                        font.pixelSize: 24
                        font.bold: true
                        color: "#e2e8f0"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Text {
                        text: kioskModel.contentTypeLabel.length > 0 ? kioskModel.contentTypeLabel : (kioskModel.contentScientificName.length > 0 ? kioskModel.contentScientificName : "")
                        font.pixelSize: 15
                        color: "#64748b"
                        Layout.alignment: Qt.AlignHCenter
                        visible: text.length > 0
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
                Layout.fillWidth: true
                Layout.preferredHeight: contentDescText.implicitHeight + 36
                color: "#131d36"
                radius: 14
                border.color: "#1e293b"
                border.width: 1

                Text {
                    id: contentDescText
                    anchors.centerIn: parent
                    width: parent.width - 48
                    text: kioskModel.contentText
                    font.pixelSize: Math.max(16, Math.min(contentArea.width * 0.013, 22))
                    color: "#e2e8f0"
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    lineHeight: 1.35
                }
            }

            // Trilhas de Exploração e Relações Ecológicas (Chips Horizontais, NÃO formato quiz)
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 16
                visible: kioskModel.contentOptions.length > 0

                Repeater {
                    model: kioskModel.contentOptions

                    Rectangle {
                        Layout.preferredWidth: Math.max(chipText.implicitWidth + 44, 160)
                        Layout.preferredHeight: 50
                        radius: 25
                        color: chipArea.pressed ? "#0284c7" : (chipArea.containsMouse ? "#1e293b" : "#0f172a")
                        border.color: chipArea.containsMouse ? "#38bdf8" : "#334155"
                        border.width: 1

                        RowLayout {
                            anchors.centerIn: parent
                            spacing: 8

                            Text {
                                text: "✦"
                                font.pixelSize: 14
                                color: "#38bdf8"
                            }

                            Text {
                                id: chipText
                                text: modelData
                                font.pixelSize: 16
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
                Layout.fillWidth: true
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: kioskModel.behaviorStatus
                        font.pixelSize: 13
                        color: "#94a3b8"
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "Navegação contínua autônoma • Presença ativa"
                        font.pixelSize: 13
                        color: "#64748b"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
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
