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
    id: root

    visible: true
    width: 1000
    height: 800
    title: qsTr("ChatBot QML")
    
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
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

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
                messageErrorView.model = ErrorMessenger.get(10)
                consoleDrawer.visible = true
                consoleHideTimer.restart()
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
                    text: "+"
                    onClicked: { if (ChatController) ChatController.createChat() }
                    ToolTip.visible: hovered
                    ToolTip.text: "New Chat"
                }

                Item { Layout.fillWidth: true }

                Label {
                    id: lbl_Api
                    text: "API:"
                    color: ThemeManager.color("text")
                }
                                
                APISelector {
                    Layout.preferredWidth: 130
                }
                    
                Label {
                    id: lbl_Model
                    text: "Model:"
                    color: ThemeManager.color("text")
                }
                    
                ModelSelector {   
                    Layout.preferredWidth: 300                 
                    Layout.maximumWidth: 350
                }

                Item { Layout.fillWidth: true }
                
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
                            loadingSpinner.connectToController(ChatController)
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
                            text: "Model Store"
                            onTriggered: modelStorePopup.open()
                        }
                        MenuSeparator {}
                        MenuItem {
                            text: ThemeManager.darkMode ? "☀ Light Theme" : "🌙 Dark Theme"
                            onTriggered: {
                                ThemeManager.setDarkMode(!ThemeManager.darkMode)
                            }
                        }
                        MenuItem {
                            text: "Settings"
                            onTriggered: settingsDialog.open()
                        }
                        MenuItem { text: "About" }
                    }
                }
            }
        }
        
        // Chat View
        ChatView {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    // Model Store Dialog
    Dialog {
        id: modelStorePopup
        title: "Model Store"
        width: 800
        height: 800
        modal: true
        anchors.centerIn: parent
        padding: 0
        
        contentItem: Loader {
            id: dialogLoader
            active: modelStorePopup.opened
            source: "ModelStoreDialog.qml"
        }

        Connections {
            target: dialogLoader.item
            // Use ignoreUnknownSignals because the linter can't know the exact type of the loaded item
            ignoreUnknownSignals: true
            function onCloseRequested() {
                modelStorePopup.close()
            }
        }
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
            onWheel: (event)=> {
                event.accepted = true
                var sign = Math.sign(event.angleDelta.y)
                if (sign < 0 && ThemeManager.currentFontSize > 10 || sign > 0 && ThemeManager.currentFontSize < 40)
                    ThemeManager.currentFontSize += sign;
            }
        }
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: ThemeManager
        function onThemeChanged() {
            console.log("Theme changed in QML:", ThemeManager.currentTheme);
        }        
        function onDarkModeChanged() {
            lbl_Model.color = ThemeManager.color("text")
            lbl_Api.color = ThemeManager.color("text")
        }
    }    
}
