pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Item {
    id: root

    readonly property var currentChat: ChatController.currentChat

    // Utiliser un Item au lieu d'un Rectangle pour éviter les effets de survol
    Rectangle {
        id: bgRect
        anchors.fill: parent
        color: "transparent"
    }

    signal accepted(string text)

    implicitHeight: Math.max(25 * 1.2, rowLayout.height + 20) // s'adapte au contenu + marges

    FontMetrics {
        id: fontMetrics
        font.family: ThemeManager.currentFont
        font.pixelSize: ThemeManager.currentFontSize * 1.2
    }

    readonly property real preferredInputHeight: 
        Math.max(25 * 1.2,
            Math.min(150, Math.max((fontMetrics.height * 2) + inputField.topPadding + inputField.bottomPadding, 
                                   inputField.contentHeight + inputField.topPadding + inputField.bottomPadding)))

    RowLayout {
        id: rowLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        spacing: 6
        
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            clip: true
            
            // Hauteur dynamique : calculée pour au moins 2 lignes
            Layout.preferredHeight: root.preferredInputHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                x: parent.width - 16
                y: inputField.topPadding + 4
                contentItem: Rectangle {
                    implicitWidth: 8
                    implicitHeight: root.preferredInputHeight - inputField.topPadding - inputField.bottomPadding - 8
                    radius: width / 2
                    color: ThemeManager.color("windowDarker2")
                    opacity: 0.3
                }
                
                background: Item {} // Pas de fond de rail
            }

            TextArea {
                id: inputField
                placeholderText: "your message..."
                color: ThemeManager.color("text")
                font.family: ThemeManager.currentFont
                font.pixelSize: ThemeManager.currentFontSize * 1.2
                verticalAlignment: Text.AlignTop // Alignement standard pour le texte multi-lignes
                wrapMode: Text.Wrap
                selectByMouse: true
                hoverEnabled: false
                
                // Marge interne pour le texte
                leftPadding: 6
                rightPadding: 6
                topPadding: ThemeManager.currentFontSize * 1.2
                bottomPadding: 6

                // Handle Shift+Enter
                Keys.onReturnPressed: (event) => {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false
                    } else {
                        event.accepted = true
                        if (inputField.text.trim().length > 0)
                            sendBtn.clicked()
                    }
                }

                // Custom Context Menu
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    cursorShape: Qt.IBeamCursor
                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.popup()
                        }
                    }
                }

                Menu {
                    id: contextMenu
                    MenuItem { text: "Cut"; enabled: inputField.selectedText.length > 0; onTriggered: inputField.cut() }
                    MenuItem { text: "Copy"; enabled: inputField.selectedText.length > 0; onTriggered: inputField.copy() }
                    MenuItem { 
                        text: "Paste"; 
                        enabled: inputField.canPaste || Clipboard.hasUrls() || Clipboard.hasImage(); 
                        onTriggered: {
                            // Vérifier d'abord si le presse-papier contient des fichiers/images
                            if (Clipboard.hasUrls()) {
                                var urls = Clipboard.getUrls()
                                if (urls.length > 0) {
                                    // Vérifier si c'est une image
                                    var filePath = urls[0]
                                    var extension = filePath.split('.').pop().toLowerCase()
                                    if (extension === "png" || extension === "jpg" || extension === "jpeg" || 
                                        extension === "gif" || extension === "webp") {
                                        ChatController.addAsset(filePath)
                                    } else {
                                        // Pour les autres fichiers, on pourrait les traiter différemment
                                        // ou simplement coller le chemin comme texte
                                        inputField.paste()
                                    }
                                }
                            } else if (Clipboard.hasImage()) {
                                // Image collée directement depuis le presse-papier
                                var base64Image = Clipboard.getImageAsBase64()
                                if (base64Image) {
                                    ChatController.addAssetBase64(base64Image)
                                }
                            } else {
                                // Coller du texte normalement
                                inputField.paste()
                            }
                        }
                    }
                    MenuSeparator {}
                    MenuItem { text: "Select All"; enabled: inputField.length > 0; onTriggered: inputField.selectAll() }
                }
            }
        }
        
        Button {
            id: sendBtn
            Layout.alignment: Qt.AlignBottom        
            palette {
                buttonText: ThemeManager.color("buttonText")
                button: ThemeManager.color("button")
            }             
            text: "⤴️"
            enabled: text === "🟥" || (inputField.text.trim().length > 0 || ChatController.pendingAssets.length > 0)
            onClicked: {
                if (text === "⤴️") {
                    root.accepted(inputField.text)                    
                } else {
                    ChatController.stopGeneration()
                }
            }
            
            LoadingSpinner {
                anchors.fill: parent
                id: loadingSpinner
                size: parent.height

                Component.onCompleted: {
                    if (ChatController) {
                        loadingSpinner.connectToController(ChatController);
                    }
                }
            }
            
            Connections {
                target: root.currentChat
                function onProcessingStarted() {
                    sendBtn.text = "🟥"
                    inputField.clear()
                }                
                function onProcessingFinished() {
                    sendBtn.text = "⤴️"
                }
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: ThemeManager
        function onDarkModeChanged() {
            inputField.color = ThemeManager.color("text")
            sendBtn.palette.buttonText = ThemeManager.color("buttonText")
            sendBtn.palette.button = ThemeManager.color("button")
        }
        function onFontChanged() {
            inputField.font.family = ThemeManager.currentFont
            inputField.font.pixelSize = ThemeManager.currentFontSize * 1.2
            inputField.topPadding = ThemeManager.currentFontSize * 1.2
            sendBtn.font.pixelSize = ThemeManager.currentFontSize * 1.2
        }
    }
}
