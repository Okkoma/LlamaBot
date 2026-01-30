pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls

import LlamaBotQml

ComboBox {
    id: root

    model: ChatController.getAvailableAPIs()
    textRole: "name"
    
    displayText: ChatController.currentChat ? ChatController.currentChat.currentApi : "No API"
    
    onActivated: (index) => {
        if (currentValue) {
            ChatController.setAPI(currentValue.name)
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
                color: apiDelegate.modelData.ready ? ThemeManager.color("windowDarker") : ThemeManager.color("windowDarker2")
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: root.highlightedIndex === index
    }
}
