import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: drawer
    width: 280
    height: parent.height
    edge: Qt.LeftEdge
    background: Rectangle {
        color: themeManager.color("window")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            id: convers
            text: "Conversations"
            color: themeManager.color("text")
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            id: chatBtn
            text: "+ New Chat"
            palette {
                buttonText: themeManager.color("buttonText")
                button: themeManager.color("button")
            }            
            Layout.fillWidth: true
            onClicked: {
                if (chatController)
                    chatController.createChat()
            }
        }

        ListView {
            id: chatListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5

            model: chatController ? chatController.chatList : []

            delegate: ItemDelegate {
                width: ListView.view.width
                height: 60

                property var chatData: modelData
                property bool isCurrent: chatController && chatController.currentChatIndex === chatData.index
                // Get the actual Chat object directly from model data
                property var chatObject: chatData.chatObject

                background: Rectangle {
                    color: isCurrent ? themeManager.color("windowDarker") : (parent.hovered ? themeManager.color("windowDarker2") : "transparent")
                    radius: 5
                }

                contentItem: RowLayout {
                    spacing: 8
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            id: chatNameLabel
                            text: chatData.name
                            color: themeManager.color("buttonText")
                            font.bold: isCurrent
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            spacing: 5
                            Layout.fillWidth: true

                            Label {
                                id: chatModelLabel
                                text: chatData.model
                                color: themeManager.color("buttonText")
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                id: tokensLabel
                                text: chatObject ? " • " + chatObject.contextSizeUsed + "/" + chatObject.contextSize : ""
                                color: themeManager.color("buttonText")
                                font.pixelSize: 10
                                Layout.alignment: Qt.AlignRight
                            }
                        }
                    }

                    ToolButton {
                        id: optionsButton
                        Layout.alignment: Qt.AlignVCenter

                        contentItem: Text {
                            id: optionsButtonText
                            text: "⋮"
                            font.pixelSize: 24
                            color: themeManager.color("buttonText")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 30
                            implicitHeight: 30
                            color: optionsButton.hovered ? themeManager.color("windowDarker2") : "transparent"
                            radius: 4
                        }

                        onClicked: contextMenu.popup()
                    }
                }

                onClicked: {
                    if (chatController)
                        chatController.switchToChat(chatData.index)
                    drawer.close()
                }

                // Support for mobile long press
                onPressAndHold: contextMenu.popup()

                Menu {
                    id: contextMenu
                    MenuItem {
                        text: "Delete"
                        enabled: chatController && chatController.chatList.length > 1
                        onTriggered: {
                            if (chatController)
                                chatController.deleteChat(chatData.index)
                        }
                    }
                    Menu {
                        title: "Set Context Size"
                        MenuItem { text: "2048"; onTriggered: chatObject.contextSize = 2048 }
                        MenuItem { text: "8192"; onTriggered: chatObject.contextSize = 8192 }
                        MenuItem { text: "16384"; onTriggered: chatObject.contextSize = 16384 }
                        MenuItem { text: "65536"; onTriggered: chatObject.contextSize = 65536 }
                        MenuItem { text: "128000"; onTriggered: chatObject.contextSize = 128000 }
                    }
                    MenuSeparator {}                    
                    Menu {
                        title: "Copy Chat to Clipboard"
                        MenuItem {
                            text: "Full Conversation"
                            enabled: chatController && chatController.currentChat && chatController.currentChat.history.length > 0
                            onTriggered: {
                                if (chatController && chatController.currentChat) {
                                    var text = chatController.currentChat.getFullConversation()
                                    clipboard.setText(text)
                                }
                            }
                        }
                        MenuItem {
                            text: "User Prompts Only"
                            enabled: chatController && chatController.currentChat && chatController.currentChat.history.length > 0
                            onTriggered: {
                                if (chatController && chatController.currentChat) {
                                    var text = chatController.currentChat.getUserPrompts()
                                    clipboard.setText(text)
                                }
                            }
                        }
                        MenuItem {
                            text: "Bot Responses Only"
                            enabled: chatController && chatController.currentChat && chatController.currentChat.history.length > 0
                            onTriggered: {
                                if (chatController && chatController.currentChat) {
                                    var text = chatController.currentChat.getBotResponses()
                                    clipboard.setText(text)
                                }
                            }
                        }
                    }
                }

                Connections {
                    target: themeManager
                    function onDarkModeChanged() {
                        // Update colors for static elements
                        chatNameLabel.color = themeManager.color("buttonText")
                        chatModelLabel.color = themeManager.color("buttonText")
                        tokensLabel.color = themeManager.color("buttonText")
                        optionsButtonText.color = themeManager.color("buttonText")
                    }
                }
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: themeManager
        function onDarkModeChanged() {
            // Update colors for static elements
            drawer.background.color = themeManager.color("window")
            convers.color = themeManager.color("text")
            chatBtn.palette.buttonText = themeManager.color("buttonText")
            chatBtn.palette.button = themeManager.color("button")
        }
    }
}
