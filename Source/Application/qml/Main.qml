import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick.Controls.Material
import QtQuick.Controls.Universal

ApplicationWindow {
    Material.theme: themeManager.darkMode ? Material.Dark : Material.Light
    Universal.theme: themeManager.darkMode ? Universal.Dark : Universal.Light

    id: window
    visible: true
    width: 1000
    height: 800
    title: qsTr("ChatBot QML")

    // Listen to theme changes
    Connections {
        target: themeManager
        function onThemeChanged() {
            console.log("Theme changed in QML:", themeManager.currentTheme);
        }
    }

    // Chat Drawer
    ChatDrawer {
        id: chatDrawer
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
                    onClicked: { if (chatController) chatController.createChat() }
                    ToolTip.visible: hovered
                    ToolTip.text: "New Chat"
                }

                Item { Layout.fillWidth: true }

                Label {
                    id: lbl_Api
                    text: "API:"
                    color: themeManager.color("text")
                }
                                
                APISelector {
                    Layout.preferredWidth: 130
                }
                    
                Label {
                    id: lbl_Model
                    text: "Model:"
                    color: themeManager.color("text")
                }
                    
                ModelSelector {   
                    Layout.preferredWidth: 300                 
                    Layout.maximumWidth: 350
                }

                Item { Layout.fillWidth: true }
                
                // RAG Status Indicator
                ToolButton {
                    text: "📚"
                    visible: chatController ? chatController.ragEnabled : false
                    enabled: false
                    ToolTip.visible: hovered
                    ToolTip.text: "RAG Active: " + (chatController && chatController.ragService ? chatController.ragService.collectionStatus : "N/A")
                    opacity: 0.7
                }
                
                // Loading icon - avec gestion robuste des connexions
                LoadingSpinner {
                    id: loadingSpinner
                    size: parent.height
                    
                    // Connect to ChatController using safe method
                    Component.onCompleted: {
                        if (chatController) {
                            loadingSpinner.connectToController(chatController)
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
                            text: themeManager.darkMode ? "☀ Light Theme" : "🌙 Dark Theme"
                            onTriggered: {
                                themeManager.setDarkMode(!themeManager.darkMode)
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
            // anchors.fill: parent // Removed to avoid overlapping header
            active: modelStorePopup.opened
            source: "qrc:/ressources/ModelStoreDialog.qml"
            onLoaded: {
                item.closeRequested.connect(function() {
                    modelStorePopup.close()
                })
            }
        }
    }

    // Settings Dialog
    SettingsDialog {
        id: settingsDialog
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: themeManager
        function onDarkModeChanged() {
            lbl_Model.color = themeManager.color("text")
            lbl_Api.color = themeManager.color("text")
        }
    }    
}
