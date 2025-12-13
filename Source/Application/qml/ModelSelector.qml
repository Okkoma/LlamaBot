import QtQuick
import QtQuick.Controls

ComboBox {
    id: modelSelector
    
    property var modelList: chatController ? chatController.getAvailableModels() : []
    
    model: modelList
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
    
    // Refresh model list when available models change
    Connections {
        target: chatController || null
        function onAvailableModelsChanged() {
            if (chatController)
                modelSelector.modelList = chatController.getAvailableModels()
        }
    }
}
