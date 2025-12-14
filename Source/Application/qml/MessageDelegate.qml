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
            color: {
                var isDark = application ? (application.currentTheme === "Dark") : true;
                return isUser ? (isDark ? Material.color(Material.Grey, Material.Shade700) : Material.color(Material.Grey, Material.Shade400)) : Material.accent;
            }
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
            color: {
                var isDark = application ? (application.currentTheme === "Dark") : true;
                if (isUser) {
                    return isDark ? Material.color(Material.Grey, Material.Shade800) : Material.color(Material.Grey, Material.Shade200);
                } else {
                    return isDark ? Material.color(Material.Grey, Material.Shade900) : Material.color(Material.Grey, Material.Shade100);
                }
            }
            radius: 10
            
            Item {
                anchors.fill: parent
                anchors.margins: 10
                
                TextEdit {
                    id: msgText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    height: implicitHeight
                    text: messageData ? messageData.content : ""
                    color: Material.foreground
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.MarkdownText
                    selectByMouse: true
                    readOnly: true
                    
                    // Force proper height calculation
                    onImplicitHeightChanged: {
                        bubble.Layout.preferredHeight = Qt.binding(() => msgText.implicitHeight + 20)
                    }
                }
            }
        }
        
        Item { Layout.fillWidth: true } // Spacer
    }
}
