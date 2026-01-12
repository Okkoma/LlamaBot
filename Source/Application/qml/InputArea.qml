import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: rootItem
    // Utiliser un Item au lieu d'un Rectangle pour éviter les effets de survol
    Rectangle {
        id: bgRect
        anchors.fill: parent
        color: "transparent"
    }

    signal accepted(string text)

    // Popup d'emoji (placé en dehors du RowLayout pour un meilleur positionnement)
    Popup {
        id: emojiPopup
        parent: rootItem
        modal: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        // Liste d'emoji accessible globalement dans le Popup
        property var emojiList: ["😄","🚀","❤️","👍","😂","🎉","💡","🌟","😎","🤖","🍕","🐱"]

        // Fonction pour calculer la position au-dessus du bouton
        function updatePosition() {
            var buttonPos = rootItem.mapFromItem(emojiButton, 0, 0)
            x = buttonPos.x
            y = buttonPos.y - height - 5
        }
        
        onAboutToShow: updatePosition()

        background: Rectangle {
            color: themeManager.color("windowDarker")
            border.color: themeManager.color("windowDarker2")
            border.width: 1
            radius: 8
            
            // Ombre portée simple
            layer.enabled: true
            layer.effect: DropShadow {
                horizontalOffset: 0
                verticalOffset: 2
                radius: 8
                samples: 17
                color: "#40000000"
            }
        }

        contentItem: GridLayout {
            Layout.alignment: Qt.AlignCenter
            columns: 6
            columnSpacing: 4
            rowSpacing: 4

            Repeater {
                model: emojiPopup.emojiList.length
                delegate: Button {
                    Layout.preferredWidth: 30
                    Layout.preferredHeight: 30
                    Layout.alignment: Qt.AlignCenter

                    contentItem: Text {
                        id: textid
                        anchors.alignWhenCentered: true
                        anchors.centerIn: parent     
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.family: themeManager.colorEmojiFont
                        font.pixelSize: 20
                        text: emojiPopup.emojiList[index]
                        color: themeManager.color("text")
                    }
                    
                    background: Rectangle {
                        anchors.centerIn: textid                        
                        width: parent.width
                        height: parent.height
                        color: parent.hovered ? themeManager.color("accent") : "transparent"
                        radius: 8
                    }

                    onClicked: {
                        inputField.insert(inputField.cursorPosition, emojiPopup.emojiList[index])
                        emojiPopup.close()
                        inputField.forceActiveFocus()
                    }
                }
            }
        }
    }

    implicitHeight: Math.max(emojiButton.height * 1.2, rowLayout.height + 20) // s'adapte au contenu + marges


    FontMetrics {
        id: fontMetrics
        font.family: themeManager.currentFont
        font.pixelSize: themeManager.currentFontSize * 1.2
    }

    readonly property real preferredInputHeight: 
        Math.max(emojiButton.height * 1.2,
            Math.min(150, Math.max((fontMetrics.height * 2) + inputField.topPadding + inputField.bottomPadding, 
                                   inputField.contentHeight + inputField.topPadding + inputField.bottomPadding)))

    RowLayout {
        id: rowLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        spacing: 6

        Button { // Bouton pour ouvrir le popup d'emoji
            id: emojiButton
            Layout.alignment: Qt.AlignBottom
            font.family: themeManager.currentFont
            font.pixelSize: 20
            text: "😌"
            palette {
                buttonText: themeManager.color("buttonText")
                button: themeManager.color("button")
            }                  
            onClicked: emojiPopup.open()
        }
        
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            clip: true
            
            // Hauteur dynamique : calculée pour au moins 2 lignes
            Layout.preferredHeight: rootItem.preferredInputHeight
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                x: parent.width - 16
                y: inputField.topPadding + 4
                contentItem: Rectangle {
                    implicitWidth: 8
                    implicitHeight: rootItem.preferredInputHeight - inputField.topPadding - inputField.bottomPadding - 8
                    radius: width / 2
                    color: themeManager.color("windowDarker2")
                    opacity: 0.3
                }
                
                background: Item {} // Pas de fond de rail
            }

            TextArea {
                id: inputField
                placeholderText: "Type a message..."
                color: themeManager.color("text")
                font.family: themeManager.currentFont
                font.pixelSize: themeManager.currentFontSize * 1.2
                verticalAlignment: Text.AlignTop // Alignement standard pour le texte multi-lignes
                wrapMode: Text.Wrap
                selectByMouse: true
                hoverEnabled: false
                
                // Marge interne pour le texte
                leftPadding: 6
                rightPadding: 6
                topPadding: themeManager.currentFontSize * 1.2
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
                    MenuItem { text: "Paste"; enabled: inputField.canPaste; onTriggered: inputField.paste() }
                    MenuSeparator {}
                    MenuItem { text: "Select All"; enabled: inputField.length > 0; onTriggered: inputField.selectAll() }
                }
            }
        }
        
        Button {
            id: sendBtn
            Layout.alignment: Qt.AlignBottom        
            palette {
                buttonText: themeManager.color("buttonText")
                button: themeManager.color("button")
            }             
            text: "Send"
            enabled: inputField.text.trim().length > 0
            onClicked: {
                parent.parent.accepted(inputField.text)
                inputField.clear()
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: themeManager
        function onDarkModeChanged() {
            inputField.color = themeManager.color("text")
            sendBtn.palette.buttonText = themeManager.color("buttonText")
            sendBtn.palette.button = themeManager.color("button")
            emojiButton.palette.buttonText = themeManager.color("buttonText")
            emojiButton.palette.button = themeManager.color("button")
            emojiPopup.background.color = themeManager.color("windowDarker")
            emojiPopup.background.border.color = themeManager.color("windowDarker2")
        }
        function onFontChanged() {
            inputField.font.family = themeManager.currentFont
            inputField.font.pixelSize = themeManager.currentFontSize * 1.2
            inputField.topPadding = themeManager.currentFontSize * 1.2
        }
    }
}
