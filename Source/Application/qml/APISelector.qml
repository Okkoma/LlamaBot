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

    displayText: ChatController.currentApi

    onActivated: index => {
        if (currentValue) {
            console.log("APISelector: onActivated:", index, currentValue.name);
            ChatController.setCurrentApi(currentValue.name);
        }
    }
    
    Component.onCompleted: {
        currentIndex = ChatController.currentApiIndex;
        console.log("APISelector: onCompleted:", currentIndex, currentValue.name);        
        displayText = currentValue.name;
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

    Connections {
        target: ChatController
        function onAvailableAPIsChanged() {
            root.model = ChatController.getAvailableAPIs();
            var index = root.find(root.displayText);
            if (index !== -1) {
                root.currentIndex = index;
            }
            console.log("APISelector: onAvailableAPIsChanged:", index, root.displayText, root.currentValue.name);
            root.displayText = root.currentValue.name;
        }
        function onCurrentApiChanged() {
            root.currentIndex = ChatController.currentApiIndex();
            root.displayText = root.currentValue.name;
            console.log("APISelector: onCurrentApiChanged:", root.currentIndex, root.displayText);
        }
    }
}
