pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Controls.Universal
import QtQuick.Layouts

import LlamaBotQml

ApplicationWindow {
    id: main

    visible: true
    width: 1000
    height: 800
    title: qsTr("LlamaBot QML")

    Material.theme: ThemeManager.darkMode ? Material.Dark : Material.Light
    Universal.theme: ThemeManager.darkMode ? Universal.Dark : Universal.Light

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
                    text: "☰"
                    onClicked: chatDrawer.open()
                    ToolTip.visible: hovered
                    ToolTip.text: "Conversations"
                }

                ToolButton {
                    id: newChat
                    text: "+"
                    onClicked: main.createNewChat()
                    ToolTip.visible: hovered
                    ToolTip.text: "New Chat"
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    id: lbl_Api
                    text: "API:"
                    color: ThemeManager.color("text")
                }

                APISelector {
                    id: apiSelector
                    Layout.preferredWidth: 130
                }

                Label {
                    id: lbl_Model
                    text: "Model:"
                    color: ThemeManager.color("text")
                }

                ModelSelector {
                    id: modelSelector
                    Layout.preferredWidth: 300
                    Layout.maximumWidth: 350

                    Connections {
                        target: chatDrawer
                        function onChatChanged() {
                            console.log("onChatChanged:");
                            ChatController.setAPI(ChatController.currentChat.currentApi);
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
                    visible: ChatController ? ChatController.ragEnabled : false
                    enabled: false
                    ToolTip.visible: hovered
                    ToolTip.text: "RAG Active: " + (ChatController && ChatController.ragService ? ChatController.ragService.collectionStatus : "N/A")
                    opacity: 0.7
                }

                // Loading icon - avec gestion robuste des connexions
                LoadingSpinner {
                    id: loadingSpinner
                    size: parent.height

                    // Connect to ChatController using safe method
                    Component.onCompleted: {
                        if (ChatController) {
                            loadingSpinner.connectToController(ChatController);
                        }
                    }
                }

                ToolButton {
                    text: "⋮"
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
            console.log("create new chat:", apiSelector.displayText, modelSelector.displayText);
            ChatController.createChat(apiSelector.displayText, modelSelector.displayText);
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
            lbl_Model.color = ThemeManager.color("text");
            lbl_Api.color = ThemeManager.color("text");
            toolBar.background.color = ThemeManager.color("window");
        }
    }
}
