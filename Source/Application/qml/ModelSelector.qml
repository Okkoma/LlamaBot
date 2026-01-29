pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import LlamaBotQml

ComboBox {
    id: root
    
    readonly property ThemeManager themeManager: app.themeManager
    readonly property ChatController chatController: app.chatController

    model: chatController ? chatController.getAvailableModels() : []
    textRole: "name"
    
    displayText: chatController && chatController.currentChat ? chatController.currentChat.currentModel : "No Model"
    
    onActivated: (index) => {
        if (chatController && currentValue) {
            chatController.setModel(currentValue.name)
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
                color: root.themeManager.color("text")
            }
            Label {
                text: modelDelegate.modelData.params || "Unknown size"
                font.pixelSize: 10
                color: root.themeManager.color("text")
            }
        }
        highlighted: root.highlightedIndex === index
    }
    
    // Refresh model list when available models change
    Connections {
        target: root.chatController || null
        function onAvailableModelsChanged() {
            if (root.chatController)
                root.model = root.chatController.getAvailableModels()
        }
    }
}
