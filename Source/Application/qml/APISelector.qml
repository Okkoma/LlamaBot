pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Application.QmlApplication 1.0

ComboBox {
    id: apiSelector

    model: chatController ? chatController.getAvailableAPIs() : []
    textRole: "name"
    
    displayText: chatController && chatController.currentChat ? chatController.currentChat.currentApi : "No API"
    
    onActivated: (index) => {
        if (chatController && currentValue) {
            chatController.setAPI(currentValue.name)
        }
    }
    
    delegate: ItemDelegate {
        required property var modelData
        required property int index

        width: apiSelector.width
        contentItem: Row {
            spacing: 10
            Label {
                text: modelData.name
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                text: modelData.ready ? "●" : "○"
                color: modelData.ready ? themeManager.color("windowDarker") : themeManager.color("windowDarker2")
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: apiSelector.highlightedIndex === index
    }
}
