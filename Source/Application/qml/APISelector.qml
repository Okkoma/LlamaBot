pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import LlamaBotQml

ComboBox {
    id: root

    readonly property ThemeManager themeManager: app.themeManager
    readonly property ChatController chatController: app.chatController

    model: chatController ? chatController.getAvailableAPIs() : []
    textRole: "name"
    
    displayText: chatController && chatController.currentChat ? chatController.currentChat.currentApi : "No API"
    
    onActivated: (index) => {
        if (chatController && currentValue) {
            chatController.setAPI(currentValue.name)
        }
    }
    
    delegate: ItemDelegate {
        id: apiDelegate

        required property int index
        property var modelData: root.model[index]

        width: root.width
        contentItem: Row {
            spacing: 10
            Label {
                text: apiDelegate.modelData.name
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                text: apiDelegate.modelData.ready ? "●" : "○"
                color: apiDelegate.modelData.ready ? root.themeManager.color("windowDarker") : root.themeManager.color("windowDarker2")
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: root.highlightedIndex === index
    }
}
