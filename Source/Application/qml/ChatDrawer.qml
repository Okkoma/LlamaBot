pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Drawer {
    id: root

    width: 280
    height: parent.height
    edge: Qt.LeftEdge
    background: Rectangle {
        color: ThemeManager.color("window")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            id: convers
            text: "Conversations"
            color: ThemeManager.color("text")
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            id: chatBtn
            text: "+ New Chat"
            palette {
                buttonText: ThemeManager.color("buttonText")
                button: ThemeManager.color("button")
            }
            Layout.fillWidth: true
            onClicked: {
                if (ChatController)
                    ChatController.createChat();
                chatListView.currentIndex = ChatController.currentChatIndex;
                chatListView.positionViewAtIndex(chatListView.currentIndex, ListView.Visible);
            }
        }

        ListView {
            id: chatListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5

            model: ChatController ? ChatController.chatList : []

            delegate: ItemDelegate {
                id: chatDelegate
                width: ListView.view.width
                height: 60

                required property var modelData
                required property int index

                property bool isCurrent: (typeof ChatController !== "undefined" && ChatController) ? (ChatController.currentChatIndex === modelData.index) : false
                // Get the actual Chat object directly from model data
                property var chatObject: modelData.chatObject

                background: Rectangle {
                    color: chatDelegate.isCurrent ? ThemeManager.color("windowDarker") : (chatDelegate.hovered ? ThemeManager.color("windowDarker2") : "transparent")
                    radius: 5
                }

                contentItem: RowLayout {
                    spacing: 8

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            id: chatNameLabel
                            text: chatDelegate.modelData.name
                            color: ThemeManager.color("buttonText")
                            font.bold: chatDelegate.isCurrent
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }

                        RowLayout {
                            spacing: 5
                            Layout.fillWidth: true

                            Label {
                                id: chatModelLabel
                                text: chatDelegate.modelData.model
                                color: ThemeManager.color("buttonText")
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                id: tokensLabel
                                text: chatDelegate.chatObject ? " • " + chatDelegate.chatObject.contextSizeUsed + "/" + chatDelegate.chatObject.contextSize : ""
                                color: ThemeManager.color("buttonText")
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
                            color: ThemeManager.color("buttonText")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 30
                            implicitHeight: 30
                            color: optionsButton.hovered ? ThemeManager.color("windowDarker2") : "transparent"
                            radius: 4
                        }

                        onClicked: contextMenu.popup()
                    }
                }

                onClicked: {
                    if (ChatController)
                        ChatController.switchToChat(modelData.index);
                    root.close();
                }

                // Support for mobile long press
                onPressAndHold: contextMenu.popup()

                Menu {
                    id: contextMenu
                    MenuItem {
                        text: "Delete"
                        enabled: (typeof ChatController !== "undefined" && ChatController) ? (ChatController.chatList.length > 1) : false
                        onTriggered: {
                            if (ChatController)
                                ChatController.deleteChat(chatDelegate.modelData.index);
                        }
                    }
                    Menu {
                        title: "Set Context Size"
                        MenuItem {
                            text: "2048"
                            onTriggered: chatDelegate.chatObject.contextSize = 2048
                        }
                        MenuItem {
                            text: "8192"
                            onTriggered: chatDelegate.chatObject.contextSize = 8192
                        }
                        MenuItem {
                            text: "16384"
                            onTriggered: chatDelegate.chatObject.contextSize = 16384
                        }
                        MenuItem {
                            text: "65536"
                            onTriggered: chatDelegate.chatObject.contextSize = 65536
                        }
                        MenuItem {
                            text: "128000"
                            onTriggered: chatDelegate.chatObject.contextSize = 128000
                        }
                    }
                    MenuSeparator {}
                    Menu {
                        title: "Copy Chat to Clipboard"
                        MenuItem {
                            text: "Full Conversation"
                            enabled: (typeof ChatController !== "undefined" && ChatController && ChatController.currentChat) ? (ChatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (ChatController && ChatController.currentChat) {
                                    var text = ChatController.currentChat.getFullConversation();
                                    Clipboard.setText(text);
                                }
                            }
                        }
                        MenuItem {
                            text: "User Prompts Only"
                            enabled: (typeof ChatController !== "undefined" && ChatController && ChatController.currentChat) ? (ChatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (ChatController && ChatController.currentChat) {
                                    var text = ChatController.currentChat.getUserPrompts();
                                    Clipboard.setText(text);
                                }
                            }
                        }
                        MenuItem {
                            text: "Bot Responses Only"
                            enabled: (typeof ChatController !== "undefined" && ChatController && ChatController.currentChat) ? (ChatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (ChatController && ChatController.currentChat) {
                                    var text = ChatController.currentChat.getBotResponses();
                                    Clipboard.setText(text);
                                }
                            }
                        }
                    }
                }

                Connections {
                    target: ThemeManager
                    function onDarkModeChanged() {
                        chatDelegate.background.color = "transparent";
                        chatNameLabel.color = ThemeManager.color("buttonText");
                        chatModelLabel.color = ThemeManager.color("buttonText");
                        tokensLabel.color = ThemeManager.color("buttonText");
                        optionsButtonText.color = ThemeManager.color("buttonText");
                        optionsButton.background.color = "transparent";
                    }
                }
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: ThemeManager
        function onDarkModeChanged() {
            // Update colors for static elements
            root.background.color = ThemeManager.color("window");
            convers.color = ThemeManager.color("text");
            chatBtn.palette.buttonText = ThemeManager.color("buttonText");
            chatBtn.palette.button = ThemeManager.color("button");
        }
    }

    onOpened: chatListView.positionViewAtIndex(ChatController.currentChatIndex, ListView.Visible)
}
