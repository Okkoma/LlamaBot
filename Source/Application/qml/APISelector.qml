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
        buttonText: ThemeManager.buttonTextColor
        button: ThemeManager.buttonColor
        window: ThemeManager.windowColor
        text: ThemeManager.textColor
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
                color: ThemeManager.textColor
                font.bold: true
                anchors.verticalCenter: parent.verticalCenter
            }
            Label {
                id: apiReadyLbl
                text: apiDelegate.modelData.ready ? "●" : "○"
                color: apiDelegate.modelData.ready ? ThemeManager.windowDarkerColor : ThemeManager.windowDarker2Color
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        highlighted: root.highlightedIndex === index
    }
}
