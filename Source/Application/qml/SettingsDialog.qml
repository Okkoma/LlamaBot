import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Dialog {
    id: dialog
    title: "Settings"
    modal: true
    width: 400
    height: 600
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

        // RAG Section
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: themeManager.color("border")
        }

        Label {
            text: "RAG (Document Knowledge)"
            font.bold: true
        }

        Switch {
            id: ragToggle
            text: "Enable RAG"
            checked: chatController ? chatController.ragEnabled : false
            onToggled: { if (chatController) chatController.ragEnabled = checked; }
            ToolTip.visible: hovered
            ToolTip.text: "Retrieve context from indexed documents"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            enabled: ragToggle.checked

            Button {
                text: "📁 Index Folder"
                Layout.fillWidth: true
                onClicked: folderDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: "Select a folder containing PDF, TXT, or MD files"
            }

            Button {
                text: "🗑️ Clear Index"
                onClicked: {
                    if (chatController && chatController.ragService) chatController.ragService.clearCollection()
                }
            }
        }

        Label {
            text: chatController && chatController.ragService ? chatController.ragService.collectionStatus : "N/A"
            font.pixelSize: 12
            color: themeManager.color("textSecondary")
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
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

    FolderDialog {
        id: folderDialog
        title: "Select Documents Folder"
        onAccepted: {
            // Convert file:// URL to local path
            var path = selectedFolder.toString()
            path = path.replace(/^(file:\/{2})/,"")
            chatController.ragService.ingestDirectory(path)
        }
    }
}