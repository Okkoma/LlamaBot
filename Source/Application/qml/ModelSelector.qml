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

    onActivated: index => {
        if (ChatController && currentValue) {
            ChatController.setModel(currentValue.name);
        }
    }

    palette {
        buttonText: ThemeManager.color("buttonText")
        button: ThemeManager.color("button")
        window: ThemeManager.color("window")
        text: ThemeManager.color("text")
    }

    delegate: ItemDelegate {
        id: modelDelegate

        required property int index
        property var modelData: root.model[index]

        width: root.width

        contentItem: Column {
            Label {
                id: modelNameLbl
                text: modelDelegate.modelData.name
                font.bold: true
                color: ThemeManager.color("text")
            }
            Label {
                id: modelSizeLbl
                text: modelDelegate.modelData.params || "Unknown size"
                font.pixelSize: 10
                color: ThemeManager.color("text")
            }
        }
        highlighted: root.highlightedIndex === index

        Connections {
            target: ThemeManager
            function onDarkModeChanged() {
                modelNameLbl.color = ThemeManager.color("text");
                modelSizeLbl.color = ThemeManager.color("text");
            }
        }
    }

    // Refresh model list when available models change
    Connections {
        target: ChatController || null
        function onAvailableModelsChanged() {
            if (ChatController)
                root.model = ChatController.getAvailableModels();
        }
    }

    Connections {
        target: ThemeManager
        function onDarkModeChanged() {
            root.palette.buttonText = ThemeManager.color("buttonText");
            root.palette.button = ThemeManager.color("button");
            root.palette.window = ThemeManager.color("window");
            root.palette.text = ThemeManager.color("text");
        }
    }
}
