import QtQuick
import QtQuick.Controls

ComboBox {
    id: modelSelector
    
    model: chatController ? chatController.getAvailableModels() : []
    textRole: "name"
    
    displayText: chatController && chatController.currentChat ? chatController.currentChat.currentModel : "No Model"
    
    onActivated: (index) => {
        if (chatController && currentValue) {
            chatController.setModel(currentValue.name)
        }
    }
    
    delegate: ItemDelegate {
        width: modelSelector.width
        contentItem: Column {
            Label {
                text: modelData.name
                font.bold: true
                color: "white"
            }
            Label {
                text: modelData.params || "Unknown size"
                font.pixelSize: 10
                color: "#aaa"
            }
        }
        highlighted: modelSelector.highlightedIndex === index
    }
}
