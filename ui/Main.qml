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

    Timer {
        interval: 3200
        running: kioskModel.recognitionResolved &&
                 kioskModel.currentState === "IDENTITY_SUPPORTED"
        repeat: false
        onTriggered: kioskModel.advanceContent()
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

        // Discrete operational preview: it explains what the totem sees but
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
                source: "image://camera/live?" + kioskModel.cameraFrameRevision
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

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20
            visible: kioskModel.currentState === "IDLE"

            Text {
                text: "ELO está pronto"
                font.pixelSize: 40
                font.bold: true
                color: "#f8fafc"
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Aproxime-se. O reconhecimento acontece automaticamente."
                font.pixelSize: 19
                color: "#94a3b8"
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "Processamento local • nenhuma fotografia armazenada • somente representação facial vetorial"
                font.pixelSize: 13
                color: "#64748b"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width * 0.72, 760)
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
                font.pixelSize: 40
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

            Text {
                visible: kioskModel.recognitionResolved
                text: kioskModel.activePerson
                font.pixelSize: 12
                color: "#64748b"
                Layout.alignment: Qt.AlignHCenter
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 24
            visible: kioskModel.currentState === "CONTENT_ACTIVE"

            Text {
                text: "Experiência em andamento"
                font.pixelSize: 30
                font.bold: true
                color: "#f8fafc"
                Layout.alignment: Qt.AlignHCenter
            }

            Rectangle {
                Layout.preferredWidth: 520
                Layout.preferredHeight: 140
                color: "#1e293b"
                radius: 12

                Text {
                    anchors.centerIn: parent
                    text: "Conteúdo apresentado:\n" + kioskModel.currentContent
                    font.pixelSize: 19
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
                    text: "Próximo conteúdo"
                    onClicked: kioskModel.advanceContent()
                }

                Button {
                    Layout.preferredWidth: 220
                    Layout.preferredHeight: 56
                    text: "Concluir sessão"
                    onClicked: kioskModel.finishSession()
                }
            }
        }

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

            Button {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 240
                Layout.preferredHeight: 56
                text: "Retornar ao início"
                onClicked: kioskModel.finishSession()
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
                text: "Estado: " + kioskModel.currentState
                font.pixelSize: 12
                color: "#64748b"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Esquecer minha identidade local"
                visible: kioskModel.activePerson !== "UNKNOWN"
                onClicked: kioskModel.requestForgetMe()
            }

            Button {
                text: "Encerrar sessão"
                visible: kioskModel.currentState !== "IDLE"
                onClicked: kioskModel.finishSession()
            }
        }
    }
}
