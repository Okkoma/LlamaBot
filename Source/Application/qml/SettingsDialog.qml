// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import LlamaBotQml

Dialog {
    id: root

    title: "Settings"
    modal: true
    width: 400
    height: 600
    anchors.centerIn: parent

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: false
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn
        ScrollBar.vertical.width: 5
        ScrollBar.vertical.x: parent.width + 15
        contentWidth: availableWidth // never move in horizontal

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
                model: ThemeManager.availableStyles()
                Component.onCompleted: {
                    currentIndex = find(ThemeManager.currentStyle);
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
                model: ThemeManager.availableThemes()
                Component.onCompleted: {
                    currentIndex = find(ThemeManager.currentTheme);
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
                value: ThemeManager.currentFontSize
                to: 40
                onMoved: ThemeManager.currentFontSize = value
            }

            // RAG Section
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.color("border")
            }

            Label {
                text: "RAG (Document Knowledge)"
                font.bold: true
            }

            Switch {
                id: ragToggle
                text: "Enable RAG"
                checked: ChatController ? ChatController.ragEnabled : false
                onToggled: {
                    if (ChatController)
                        ChatController.ragEnabled = checked;
                }
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
                        if (ChatController && ChatController.ragService)
                            ChatController.ragService.clearCollection();
                    }
                }
            }

            Label {
                text: ChatController && ChatController.ragService ? ChatController.ragService.collectionStatus : "N/A"
                font.pixelSize: 12
                color: ThemeManager.color("text")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // Context Settings Section
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.color("border")
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
                    value: ChatController ? ChatController.defaultContextSize : 2048
                    onValueModified: {
                        if (ChatController)
                            ChatController.defaultContextSize = value;
                    }
                }
            }

            Switch {
                id: autoExpandToggle
                text: "Auto-expand Context"
                checked: ChatController ? ChatController.autoExpandContext : true
                onToggled: {
                    if (ChatController)
                        ChatController.autoExpandContext = checked;
                }
                ToolTip.visible: hovered
                ToolTip.text: "Automatically double context size when full (up to 128k)"
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.color("border")
            }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: "Cloud Storage"
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: "🛸️ Sync Now!"
                    onClicked: {
                        if (ChatController)
                            ChatController.saveChats(true);
                    }
                }
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
                ThemeManager.setTheme(themeSelector.model[themeSelector.currentIndex == -1 ? 0 : themeSelector.currentIndex]);
                ThemeManager.setFontSize(fontSizeSelector.value);
                if (ThemeManager.currentStyle != styleSelector.model[styleSelector.currentIndex]) {
                    ThemeManager.setStyle(styleSelector.model[styleSelector.currentIndex]);
                    validateDialog.open();
                } else
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
                ThemeManager.restartApplication();
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
            var path = selectedFolder.toString();
            path = path.replace(/^(file:\/{2})/, "");
            ChatController.ragService.ingestDirectory(path);
        }
    }
}
