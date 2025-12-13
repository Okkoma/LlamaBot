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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar / Header
        ToolBar {
            Layout.fillWidth: true
            RowLayout {
                anchors.fill: parent
                ToolButton {
                    text: "+"
                    onClicked: { if (chatController) chatController.createChat() }
                    ToolTip.visible: hovered
                    ToolTip.text: "New Chat"
                }
                Label {
                    text: (chatController && chatController.currentChat) ? chatController.currentChat.currentModel : "No Model"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    horizontalAlignment: Qt.AlignHCenter
                    font.bold: true
                }
                ToolButton {
                    text: "⋮"
                    ToolTip.visible: hovered
                    ToolTip.text: "Menu"
                    onClicked: menu.open()
                    
                    Menu {
                        id: menu
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
}
