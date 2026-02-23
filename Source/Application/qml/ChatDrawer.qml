pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Drawer {
    id: root

    signal chatChanged()
    signal newChat()

    width: 280
    height: parent.height
    edge: Qt.LeftEdge
    background: Rectangle {
        color: ThemeManager.windowColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            id: convers
            text: "Conversations"
            color: ThemeManager.textColor
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            id: chatBtn
            text: "+ New Chat"
            Layout.fillWidth: true
            palette {
                buttonText: ThemeManager.buttonTextColor
                button: ThemeManager.buttonColor
            }
            onClicked: {
                root.newChat();
                root.aboutToShow();
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
                    color: chatDelegate.isCurrent ? ThemeManager.windowDarkerColor : (chatDelegate.hovered ? ThemeManager.windowDarker2Color : 'transparent')
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
                            color: ThemeManager.buttonTextColor
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
                                color: ThemeManager.buttonTextColor
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                id: tokensLabel
                                text: chatDelegate.chatObject ? " • " + chatDelegate.chatObject.contextSizeUsed + "/" + chatDelegate.chatObject.contextSize : ""
                                color: ThemeManager.buttonTextColor
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
                            color: ThemeManager.buttonTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 30
                            implicitHeight: 30
                            color: optionsButton.hovered ? ThemeManager.windowDarker2Color : 'transparent'
                            radius: 4
                        }

                        onClicked: contextMenu.popup()
                    }
                }

                onClicked: {
                    if (ChatController) {
                        ChatController.switchToChat(modelData.index);
                        root.chatChanged();
                    }
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
                            if (ChatController) {
                                ChatController.deleteChat(chatDelegate.modelData.index);
                                chatListView.currentIndex = ChatController.currentChatIndex;
                                root.aboutToShow();
                            }
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
            }
        }
    }

    onAboutToShow: chatListView.positionViewAtIndex(ChatController.currentChatIndex, ListView.Visible)
}
