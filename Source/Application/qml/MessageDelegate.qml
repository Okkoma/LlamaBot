import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: root
    width: ListView.view.width
    height: contentLayout.height + 20 // Padding

    property var messageData: modelData // {role, content}

    property bool isUser: messageData ? messageData.role === "user" : false

    RowLayout {
        id: contentLayout
        anchors.top: parent.top
        anchors.topMargin: 10
        width: parent.width - 20
        anchors.horizontalCenter: parent.horizontalCenter
        layoutDirection: isUser ? Qt.RightToLeft : Qt.LeftToRight
        spacing: 10
        
        Rectangle {
            width: 40; height: 40; radius: 20
            color: isUser ? Material.color(Material.Grey, Material.Shade700) : Material.accent
            Layout.alignment: Qt.AlignTop
            Label {
                anchors.centerIn: parent
                text: isUser ? "🧑" : "🤖"
                font.pixelSize: 20
            }
        }
        
        Rectangle {
            id: bubble
            Layout.maximumWidth: root.width * 0.7
            Layout.fillWidth: true
            Layout.preferredHeight: msgText.implicitHeight + 20
            color: isUser ? Material.color(Material.Grey, Material.Shade800) : Material.color(Material.Grey, Material.Shade900)
            radius: 10
            
            TextArea {
                id: msgText
                anchors.fill: parent
                anchors.margins: 10
                text: messageData ? messageData.content : ""
                color: Material.foreground
                wrapMode: Text.Wrap
                readOnly: true
                background: null
                textFormat: Text.MarkdownText
                selectByMouse: true
                
                // Force proper height calculation
                onContentHeightChanged: {
                    bubble.Layout.preferredHeight = Qt.binding(() => msgText.implicitHeight + 20)
                }
            }
        }
        
        Item { Layout.fillWidth: true } // Spacer
    }
}
