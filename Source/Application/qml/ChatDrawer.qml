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

    // Property to force delegate refresh
    property int forceRefresh: 0

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
                // Property to force refresh when theme changes
                property int themeDependency: drawer.forceRefresh
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
                            color: themeManager.color("text")
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
                                color: themeManager.color("text")
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                id: tokensLabel
                                text: chatObject ? " • " + chatObject.tokensConsumed + "/" + chatObject.tokensTotal + " tokens" : ""
                                color: themeManager.color("textSecondary")
                                font.pixelSize: 10
                                Layout.alignment: Qt.AlignRight
                            }
                        }
                    }

                    ToolButton {
                        id: optionsButton
                        text: "⋮"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: contextMenu.popup()
                        
                        background: Rectangle {
                            color: optionsButton.hovered ? themeManager.color("windowDarker2") : "transparent"
                            radius: 4
                        }
                    }
                }
                
                // Force color update when theme changes
                onThemeDependencyChanged: {
                    chatNameLabel.color = themeManager.color("text")
                    chatModelLabel.color = themeManager.color("text")
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
            
            // Force delegates to refresh by incrementing the dependency property
            drawer.forceRefresh++
        }
    }
}
