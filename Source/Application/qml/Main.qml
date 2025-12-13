import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

ApplicationWindow {
    id: window
    visible: true
    width: 1000
    height: 800
    title: qsTr("ChatBot QML")

    Material.theme: Material.Dark
    Material.accent: Material.Teal

    // Chat Drawer
    ChatDrawer {
        id: chatDrawer
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar / Header
        ToolBar {
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
                
                ModelSelector {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 300
                }
                
                Item { Layout.fillWidth: true }
                
                ToolButton {
                    text: "⋮"
                    ToolTip.visible: hovered
                    ToolTip.text: "Menu"
                    onClicked: menu.open()
                    
                    Menu {
                        id: menu
                        MenuItem { text: "Settings" }
                        MenuItem { text: "Model Store" }
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
}
