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

    displayText: ChatController.currentChat ? ChatController.currentChat.currentApi : "none"

    onActivated: index => {
        if (currentValue) {
            ChatController.setAPI(currentValue.name);
        }
    }

    palette {
        buttonText: ThemeManager.color("buttonText")
        button: ThemeManager.color("button")
        window: ThemeManager.color("window")
        text: ThemeManager.color("text")
    }

    delegate: ItemDelegate {
        id: apiDelegate

        required property int index
        property var modelData: root.model[index]

        width: root.width
        contentItem: Row {
            spacing: 10
            Label {
                id: apiNameLbl
                text: apiDelegate.modelData.name
                color: ThemeManager.color("text")
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                id: apiReadyLbl
                text: apiDelegate.modelData.ready ? "●" : "○"
                color: apiDelegate.modelData.ready ? ThemeManager.color("windowDarker") : ThemeManager.color("windowDarker2")
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: root.highlightedIndex === index

        Connections {
            target: ThemeManager
            function onDarkModeChanged() {
                apiNameLbl.color = ThemeManager.color("text");
                apiReadyLbl.color = ThemeManager.color("windowDarker");
            }
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
