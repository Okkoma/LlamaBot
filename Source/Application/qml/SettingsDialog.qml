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

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: false
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.width: 5
        ScrollBar.vertical.x: parent.width + 15

        ColumnLayout {
            width: settingsScroll.availableWidth
            spacing: 20
            
            // Margins to prevent content from touching edges or being covered by scrollbar
            Layout.leftMargin: 20
            Layout.rightMargin: 25 
            Layout.topMargin: 10
            Layout.bottomMargin: 10

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

            // FontSize Selection
            Label {
                text: "Font Size"
                font.bold: true
            }

            Slider {
                id: fontSizeSelector
                Layout.fillWidth: true
                from: 10
                value: themeManager.currentFontSize
                to: 40   
                onMoved: themeManager.currentFontSize = value
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
                color: themeManager.color("text")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // Context Settings Section
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: themeManager.color("border")
            }

            Label {
                text: "Chat Context Settings"
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Default Context Size:"
                    Layout.fillWidth: true
                }
                SpinBox {
                    id: contextSizeSpin
                    from: 2048
                    to: 128000
                    stepSize: 4096
                    editable: true
                    value: chatController ? chatController.defaultContextSize : 2048
                    onValueModified: { if (chatController) chatController.defaultContextSize = value }
                }
            }

            Switch {
                id: autoExpandToggle
                text: "Auto-expand Context"
                checked: chatController ? chatController.autoExpandContext : true
                onToggled: { if (chatController) chatController.autoExpandContext = checked }
                ToolTip.visible: hovered
                ToolTip.text: "Automatically double context size when full (up to 128k)"
            }
        }
    }

    footer: DialogButtonBox {
        padding: 10        
        alignment: Qt.AlignRight
        
        Button {
            text: "Apply"
            anchors.bottomMargin: 50
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                themeManager.setTheme(themeSelector.model[themeSelector.currentIndex == -1 ? 0 : themeSelector.currentIndex]);                
                themeManager.setFontSize(fontSizeSelector.value);
                if (themeManager.currentStyle != styleSelector.model[styleSelector.currentIndex]) {
                    themeManager.setStyle(styleSelector.model[styleSelector.currentIndex]);
                    validateDialog.open();
                }
                else
                    dialog.close();
            }
        }
        
        Button {
            text: "Cancel"
            anchors.bottomMargin: 50
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: dialog.close()
        }
    }

    MessageDialog {
        id: validateDialog
        text: qsTr("The style need to relaunch the application.")
        informativeText: qsTr("Do you want to restart now ?")
        buttons: MessageDialog.Ok | MessageDialog.Cancel
        onButtonClicked: function (button, role) {
            switch (button) {
            case MessageDialog.Ok:
                themeManager.restartApplication();
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