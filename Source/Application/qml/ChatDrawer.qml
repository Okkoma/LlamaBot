import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Drawer {
    id: drawer
    width: 280
    height: parent.height
    edge: Qt.LeftEdge

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
                    color: isCurrent ? "#009688" : (parent.hovered ? "#424242" : "#303030")
                    radius: 5
                }

                contentItem: ColumnLayout {
                    spacing: 2

                    Label {
                        text: chatData.name
                        font.bold: isCurrent
                        color: "white"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: chatData.model + " • " + chatData.messageCount + " messages"
                        font.pixelSize: 10
                        color: "#aaa"
                        elide: Text.ElideRight
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
