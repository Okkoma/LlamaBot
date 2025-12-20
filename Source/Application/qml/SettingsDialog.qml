import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog
    title: "Settings"
    modal: true
    width: 400
    height: 400
    anchors.centerIn: parent

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // Style Selection
        Label {
            text: "Style"
            font.bold: true
        }

        ComboBox {
            id: styleSelector
            Layout.fillWidth: true
            model: themeManager.availableStyles()
            Component.onCompleted: {
                currentIndex = find(themeManager.currentStyle)
            }            
        }

        // Theme Selection
        Label {
            text: "Theme"
            font.bold: true
        }

        ComboBox {
            id: themeSelector
            Layout.fillWidth: true
            model: themeManager.availableThemes()
            Component.onCompleted: {
                currentIndex = find(themeManager.currentTheme)
            }            
        }

        // Apply Button
        Button {
            text: "Apply"
            Layout.alignment: Qt.AlignRight
            onClicked: {                
                themeManager.setTheme(themeSelector.model[themeSelector.currentIndex == -1 ? 0 : themeSelector.currentIndex]);
                if (themeManager.currentStyle != styleSelector.model[styleSelector.currentIndex])
                    validateDialog.open();
            }
        }
    }

    MessageDialog {
        id: validateDialog
        text: qsTr("The style need to relaunch the application.")
        informativeText: qsTr("Do you want to sapply your changes ?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            switch (button) {
            case MessageDialog.Ok:
                themeManager.setStyle(styleSelector.model[styleSelector.currentIndex]);
                break;
            }
            dialog.close();
        }
    }
}