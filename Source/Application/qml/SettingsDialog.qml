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

    width: Math.min(parent.width * 0.75, 400)
    height: Math.min(parent.height * 0.75, 600)
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

            // spacer
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.spacerColor
            }

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
                onActivated: {
                    ThemeManager.setTheme(themeSelector.currentValue);
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
            // spacer
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.spacerColor
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
                color: ThemeManager.textColor
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            // Context Settings Section
            // spacer
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.spacerColor
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

            // spacer
            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: ThemeManager.spacerColor
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

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: "Reset Settings"
                    onClicked: {
                        console.log("Reset Settings...");
                        ChatController.resetSettings();
                        ThemeManager.resetSettings();
                        fontSizeSelector.value = ThemeManager.currentFontSize;
                        themeSelector.currentIndex = themeSelector.find(ThemeManager.currentTheme);
                        autoExpandToggle.checked = ChatController.autoExpandContext;
                        contextSizeSpin.value = ChatController.defaultContextSize;
                        ragToggle.checked = ChatController.ragEnabled;
                        console.log("Reset Settings ... end !");
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
