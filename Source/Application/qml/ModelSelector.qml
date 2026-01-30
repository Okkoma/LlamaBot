pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls

import LlamaBotQml

ComboBox {
    id: root
    
    model: ChatController ? ChatController.getAvailableModels() : []
    textRole: "name"
    
    displayText: ChatController && ChatController.currentChat ? ChatController.currentChat.currentModel : "No Model"
    
    onActivated: (index) => {
        if (ChatController && currentValue) {
            ChatController.setModel(currentValue.name)
        }
    }
    
    delegate: ItemDelegate {
        id: modelDelegate

        required property int index
        property var modelData: root.model[index]

        width: root.width

        contentItem: Column {
            Label {
                text: modelDelegate.modelData.name
                font.bold: true
                color: ThemeManager.color("text")
            }
            Label {
                text: modelDelegate.modelData.params || "Unknown size"
                font.pixelSize: 10
                color: ThemeManager.color("text")
            }
        }
        highlighted: root.highlightedIndex === index
    }
    
    // Refresh model list when available models change
    Connections {
        target: ChatController || null
        function onAvailableModelsChanged() {
            if (ChatController)
                root.model = ChatController.getAvailableModels()
        }
    }
}
