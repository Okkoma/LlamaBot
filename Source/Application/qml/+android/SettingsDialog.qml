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
    width: Screen.width * 0.85
    height: Screen.height * 0.85
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
