import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import LlamaBotQml

Dialog {
    id: root

    readonly property ThemeManager themeManager: app.themeManager
    readonly property ChatController chatController: app.chatController

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
                model: root.themeManager.availableStyles()
                Component.onCompleted: {
                    currentIndex = find(root.themeManager.currentStyle)
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
                model: root.themeManager.availableThemes()
                Component.onCompleted: {
                    currentIndex = find(root.themeManager.currentTheme)
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
                value: root.themeManager.currentFontSize
                to: 40   
                onMoved: root.themeManager.currentFontSize = value
            }

            // RAG Section
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: root.themeManager.color("border")
            }

            Label {
                text: "RAG (Document Knowledge)"
                font.bold: true
            }

            Switch {
                id: ragToggle
                text: "Enable RAG"
                checked: root.chatController ? root.chatController.ragEnabled : false
                onToggled: { if (root.chatController) root.chatController.ragEnabled = checked; }
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
                        if (root.chatController && root.chatController.ragService) root.chatController.ragService.clearCollection()
                    }
                }
            }

            Label {
                text: root.chatController && root.chatController.ragService ? root.chatController.ragService.collectionStatus : "N/A"
                font.pixelSize: 12
                color: root.themeManager.color("text")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // Context Settings Section
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: root.themeManager.color("border")
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
                    value: root.chatController ? root.chatController.defaultContextSize : 2048
                    onValueModified: { if (root.chatController) root.chatController.defaultContextSize = value }
                }
            }

            Switch {
                id: autoExpandToggle
                text: "Auto-expand Context"
                checked: root.chatController ? root.chatController.autoExpandContext : true
                onToggled: { if (root.chatController) root.chatController.autoExpandContext = checked }
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
                root.themeManager.setTheme(themeSelector.model[themeSelector.currentIndex == -1 ? 0 : themeSelector.currentIndex]);                
                root.themeManager.setFontSize(fontSizeSelector.value);
                if (root.themeManager.currentStyle != styleSelector.model[styleSelector.currentIndex]) {
                    root.themeManager.setStyle(styleSelector.model[styleSelector.currentIndex]);
                    validateDialog.open();
                }
                else
                    root.close();
            }
        }
        
        Button {
            text: "Cancel"
            anchors.bottomMargin: 50
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.close()
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
                root.themeManager.restartApplication();
                break;
            }
            root.close();
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Documents Folder"
        onAccepted: {
            // Convert file:// URL to local path
            var path = selectedFolder.toString()
            path = path.replace(/^(file:\/{2})/,"")
            root.chatController.ragService.ingestDirectory(path)
        }
    }
}