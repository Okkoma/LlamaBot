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

    displayText: availableModel()

    onActivated: index => {
        console.log("ModelSelector:", index, currentValue.name);
        ChatController.currentModel = displayText = currentValue.name;
    }

    palette {
        buttonText: ThemeManager.buttonTextColor
        button: ThemeManager.buttonColor
        window: ThemeManager.windowColor
        text: ThemeManager.textColor
    }

    delegate: ItemDelegate {
        id: modelDelegate

        required property int index
        required property var modelData

        width: root.width

        contentItem: Column {
            Label {
                id: modelNameLbl
                text: modelDelegate.modelData.name
                font.bold: true
                color: ThemeManager.textColor
            }
            Label {
                id: modelSizeLbl
                text: modelDelegate.modelData.params || "Unknown size"
                font.pixelSize: 10
                color: ThemeManager.textColor
            }
        }
        highlighted: root.highlightedIndex === index
    }

    function availableModel() {
        var index = root.find(ChatController.currentChat.currentModel);
        if (index === -1) {
            index = root.find(ChatController.currentModel);
        }
        if (index === -1) {
            index = 0;
        }
        root.currentIndex = index;
        root.displayText = root.currentValue.name;
        return root.currentValue.name;
    }

    // Refresh model list when available models change
    Connections {
        target: ChatController || null
        function onAvailableModelsChanged() {
            root.model = ChatController.getAvailableModels();
            var index = root.find(root.displayText);
            if (index !== -1) {
                root.currentIndex = index;
            }
            console.log("ModelSelector: onAvailableModelsChanged:", index, root.displayText, root.currentValue.name);
            root.displayText = root.currentValue.name;
        }
        function onCurrentChatChanged() {
            root.availableModel();
            console.log("ModelSelector: onCurrentChatChanged: result:", root.displayText);
        }
        function onCurrentModelChanged() {
            root.currentIndex = root.find(ChatController.currentModel);
            root.displayText = root.currentValue.name;
            console.log("ModelSelector: currentModelChanged: ", root.currentIndex, root.displayText);
        }
    }
}
