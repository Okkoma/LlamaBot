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
                
                Label {
                    text: "API:"
                    color: "white"
                }
                
                APISelector {
                    Layout.preferredWidth: 120
                }
                
                Label {
                    text: "Model:"
                    color: "white"
                }
                
                ModelSelector {
                    Layout.fillWidth: true
                    Layout.maximumWidth: 250
                }
                
                Item { Layout.fillWidth: true }
                
                ToolButton {
                    text: "⋮"
                    ToolTip.visible: hovered
                    ToolTip.text: "Menu"
                    onClicked: menu.open()
                    
                    Menu {
                        id: menu
                        MenuItem { 
                            text: "Model Store"
                            onTriggered: modelStoreDialog.open()
                        }
                        MenuItem { text: "Settings" }
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
        id: modelStoreDialog
        title: "Ollama Model Store"
        width: 600
        height: 800
        modal: true
        anchors.centerIn: parent
        
        contentItem: Loader {
            id: modelStoreLoader
            anchors.fill: parent
            source: "qrc:/ressources/OllamaModelStoreDialog.qml"
            
            onLoaded: {
                // Connect the closeRequested signal from the loaded QML
                if (item) {
                    item.closeRequested.connect(function() {
                        modelStoreDialog.close()
                    })
                }
            }
        }
    }
}
