import QtQuick
import QtQuick.Controls

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
        width: apiSelector.width
        contentItem: Row {
            spacing: 10
            Label {
                text: modelData.name
                font.bold: true
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                text: modelData.ready ? "●" : "○"
                color: modelData.ready ? "#4CAF50" : "#F44336"
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: apiSelector.highlightedIndex === index
    }
}
