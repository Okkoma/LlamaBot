pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Drawer {
    id: root

    readonly property ThemeManager themeManager: app.themeManager
    readonly property ChatController chatController: app.chatController
    readonly property Clipboard clipboard: app.clipboard

    width: 280
    height: parent.height
    edge: Qt.LeftEdge
    background: Rectangle {
        color: root.themeManager.color("window")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            id: convers
            text: "Conversations"
            color: root.themeManager.color("text")
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            id: chatBtn
            text: "+ New Chat"
            palette {
                buttonText: root.themeManager.color("buttonText")
                button: root.themeManager.color("button")
            }            
            Layout.fillWidth: true
            onClicked: {
                if (root.chatController)
                    root.chatController.createChat()
            }
        }

        ListView {
            id: chatListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5

            model: root.chatController ? root.chatController.chatList : []

            delegate: ItemDelegate {

                id: chatDelegate
                width: ListView.view.width
                height: 60

                required property var modelData
                required property int index

                property bool isCurrent: (typeof root.chatController !== "undefined" && root.chatController) ? (root.chatController.currentChatIndex === modelData.index) : false
                // Get the actual Chat object directly from model data
                property var chatObject: modelData.chatObject

                background: Rectangle {
                    color: chatDelegate.isCurrent ? root.themeManager.color("windowDarker") : (parent.hovered ? root.themeManager.color("windowDarker2") : "transparent")
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
                            color: root.themeManager.color("buttonText")
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
                                color: root.themeManager.color("buttonText")
                                font.pixelSize: 10
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Label {
                                id: tokensLabel
                                text: chatDelegate.chatObject ? " • " + chatDelegate.chatObject.contextSizeUsed + "/" + chatDelegate.chatObject.contextSize : ""
                                color: root.themeManager.color("buttonText")
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
                            color: root.themeManager.color("buttonText")
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            implicitWidth: 30
                            implicitHeight: 30
                            color: optionsButton.hovered ? root.themeManager.color("windowDarker2") : "transparent"
                            radius: 4
                        }

                        onClicked: contextMenu.popup()
                    }
                }

                onClicked: {
                    if (root.chatController)
                        root.chatController.switchToChat(modelData.index)
                    root.close()
                }

                // Support for mobile long press
                onPressAndHold: contextMenu.popup()

                Menu {
                    id: contextMenu
                    MenuItem {
                        text: "Delete"
                        enabled: (typeof root.chatController !== "undefined" && root.chatController) ? (root.chatController.chatList.length > 1) : false
                        onTriggered: {
                            if (root.chatController)
                                root.chatController.deleteChat(chatDelegate.modelData.index)
                        }
                    }
                    Menu {
                        title: "Set Context Size"
                        MenuItem { text: "2048"; onTriggered: chatDelegate.chatObject.contextSize = 2048 }
                        MenuItem { text: "8192"; onTriggered: chatDelegate.chatObject.contextSize = 8192 }
                        MenuItem { text: "16384"; onTriggered: chatDelegate.chatObject.contextSize = 16384 }
                        MenuItem { text: "65536"; onTriggered: chatDelegate.chatObject.contextSize = 65536 }
                        MenuItem { text: "128000"; onTriggered: chatDelegate.chatObject.contextSize = 128000 }
                    }
                    MenuSeparator {}                    
                    Menu {
                        title: "Copy Chat to Clipboard"
                        MenuItem {
                            text: "Full Conversation"
                            enabled: (typeof root.chatController !== "undefined" && root.chatController && root.chatController.currentChat) ? (root.chatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (root.chatController && root.chatController.currentChat) {
                                    var text = root.chatController.currentChat.getFullConversation()
                                    root.clipboard.setText(text)
                                }
                            }
                        }
                        MenuItem {
                            text: "User Prompts Only"
                            enabled: (typeof root.chatController !== "undefined" && root.chatController && root.chatController.currentChat) ? (root.chatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (root.chatController && root.chatController.currentChat) {
                                    var text = root.chatController.currentChat.getUserPrompts()
                                    root.clipboard.setText(text)
                                }
                            }
                        }
                        MenuItem {
                            text: "Bot Responses Only"
                            enabled: (typeof root.chatController !== "undefined" && root.chatController && root.chatController.currentChat) ? (root.chatController.currentChat.rowCount() > 0) : false
                            onTriggered: {
                                if (root.chatController && root.chatController.currentChat) {
                                    var text = root.chatController.currentChat.getBotResponses()
                                    root.clipboard.setText(text)
                                }
                            }
                        }
                    }
                }

                Connections {
                    target: root.themeManager
                    function onDarkModeChanged() {
                        // Update colors for static elements
                        chatNameLabel.color = root.themeManager.color("buttonText")
                        chatModelLabel.color = root.themeManager.color("buttonText")
                        tokensLabel.color = root.themeManager.color("buttonText")
                        optionsButtonText.color = root.themeManager.color("buttonText")
                    }
                }
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: root.themeManager
        function onDarkModeChanged() {
            // Update colors for static elements
            root.background.color = root.themeManager.color("window")
            convers.color = root.themeManager.color("text")
            chatBtn.palette.buttonText = root.themeManager.color("buttonText")
            chatBtn.palette.button = root.themeManager.color("button")
        }
    }
}
