pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

import LlamaBotQml

ApplicationWindow {
    id: main

    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("LlamaBot QML")

    Material.theme: ThemeManager.darkMode ? Material.Dark : Material.Light
    Material.primary: '#efefef'   // Sets the main color (e.g., ToolBar, raised Button)
    Material.accent: "#FF4081"    // Sets the accent color (e.g., CheckBox, Slider)

    // Chat Drawer
    ChatDrawer {
        id: chatDrawer
    }

    // Console
    Drawer {
        id: consoleDrawer
        width: parent.width
        height: 50
        edge: Qt.BottomEdge

        background: Rectangle {
            color: ThemeManager.color("window")
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            // Message Error View
            ListView {
                id: messageErrorView
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                delegate: Text {
                    required property var modelData
                    required property int index
                    text: index !== -1 ? modelData : ""
                    color: "red"
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }
            }
        }

        Timer {
            id: consoleHideTimer
            interval: 3000
            repeat: false
            running: false
            onTriggered: consoleDrawer.visible = false
        }

        Connections {
            target: ErrorMessenger
            function onErrorPopped(error) {
                messageErrorView.model = ErrorMessenger.get(10);
                consoleDrawer.visible = true;
                consoleHideTimer.restart();
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar / Header
        ToolBar {
            id: toolBar
            Layout.fillWidth: true
            background: Rectangle {
                color: ThemeManager.color("window")
            }
            RowLayout {
                anchors.fill: parent
                spacing: 10

                ToolButton {
                    icon.name: "menu"
                    text: "🗃️"
                    font.pixelSize: 24
                    onClicked: chatDrawer.open()
                    ToolTip.visible: hovered
                    ToolTip.text: "Conversations"
                }

                Item {
                    Layout.fillWidth: true
                }

                ModelSelector {
                    id: modelSelector
                    Layout.preferredWidth: Screen.width * 0.6
                    Layout.maximumWidth: Screen.width * 0.8
                    Connections {
                        target: chatDrawer
                        function onChatChanged() {
                            modelSelector.displayText = ChatController.currentChat.currentModel;
                        }
                    }                    
                }

                Item {
                    Layout.fillWidth: true
                }

                // RAG Status Indicator
                ToolButton {
                    text: "📚"
                    font.pixelSize: 24
                    visible: ChatController ? ChatController.ragEnabled : false
                    enabled: false
                    ToolTip.visible: hovered
                    ToolTip.text: "RAG Active: " + (ChatController && ChatController.ragService ? ChatController.ragService.collectionStatus : "N/A")
                    opacity: 0.7
                }

                ToolButton {
                    text: "⚙️"
                    font.pixelSize: 24
                    ToolTip.visible: hovered
                    ToolTip.text: "Menu"
                    onClicked: menu.open()

                    Menu {
                        id: menu
                        MenuItem {
                            text: "Local Models"
                            onTriggered: modelLocalDialog.open()
                        }                        
                        MenuItem {
                            text: "Model Store"
                            onTriggered: modelStoreDialog.open()
                        }
                        MenuSeparator {}
                        MenuItem {
                            text: ThemeManager.darkMode ? "☀ Light Theme" : "🌙 Dark Theme"
                            onTriggered: {
                                ThemeManager.setDarkMode(!ThemeManager.darkMode);
                            }
                        }
                        MenuItem {
                            text: "Settings"
                            onTriggered: settingsDialog.open()
                        }
                        MenuItem {
                            text: "About"
                        }
                    }
                }
            }
        }

        // Chat container
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ChatView {
                id: messageView
                anchors.bottom: inputArea.top
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
            }

            AssetContent {
                id: assetContent
                anchors.bottom: inputArea.top
                anchors.left: parent.left
                anchors.right: parent.right
            }

            InputArea {
                id: inputArea
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                onAccepted: (text) => {
                    if (ChatController) {
                        console.log("setModel:", modelSelector.currentValue.name);
                        ChatController.setModel(modelSelector.currentValue.name);                        
                        ChatController.sendMessage(text);
                    }
                }
            }
        }
    }

    // Model Local Dialog
    ModelLocalDialog {
        id: modelLocalDialog
    }
    // Model Store Dialog
    ModelStoreDialog {
        id: modelStoreDialog
    }
    // Settings Dialog
    SettingsDialog {
        id: settingsDialog
    }

    // Wheel Handler
    Item {
        id: wheelHandler
        anchors.fill: parent
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            onWheel: event => {
                event.accepted = true;
                var sign = Math.sign(event.angleDelta.y);
                if (sign < 0 && ThemeManager.currentFontSize > 10 || sign > 0 && ThemeManager.currentFontSize < 40)
                    ThemeManager.currentFontSize += sign;
            }
        }
    }

    function createNewChat() {
        if (ChatController) {
            console.log("create new chat:", modelSelector.displayText);
            ChatController.createChat(ChatController.currentChat.currentApi, modelSelector.displayText);
            chatDrawer.aboutToShow();
        }
    }
    Connections {
        target: chatDrawer
        function onNewChat() {
            main.createNewChat();
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: ThemeManager
        function onThemeChanged() {
            console.log("Theme changed in QML:", ThemeManager.currentTheme);
        }
        function onDarkModeChanged() {
            console.log("Dark Mode changed:", ThemeManager.color("window"));
            toolBar.background.color = ThemeManager.color("window");
        }
    }
}
