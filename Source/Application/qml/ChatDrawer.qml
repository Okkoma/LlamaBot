import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Drawer {
    id: drawer
    width: 280
    height: parent.height
    edge: Qt.LeftEdge

    Material.theme: application ? (application.currentTheme === "Dark" ? Material.Dark : Material.Light) : Material.Dark

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 10

        Label {
            text: "Conversations"
            font.pixelSize: 18
            font.bold: true
            Layout.fillWidth: true
        }

        Button {
            text: "+ New Chat"
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

                background: Rectangle {
                    color: isCurrent ? Material.accent : (parent.hovered ? Material.listHighlightColor : "transparent")
                    radius: 5
                }

                contentItem: ColumnLayout {
                    spacing: 2

                    Label {
                        text: chatData.name
                        font.bold: isCurrent
                        Layout.fillWidth: true
                    }

                    Label {
                        text: chatData.model + " • " + chatData.messageCount + " messages"
                        font.pixelSize: 10
                        color: Material.hintTextColor
                        Layout.fillWidth: true
                    }
                }

                onClicked: {
                    if (chatController)
                        chatController.switchToChat(chatData.index)
                    drawer.close()
                }

                // Right-click menu
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: contextMenu.popup()
                }

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
                }
            }
        }
    }
}

