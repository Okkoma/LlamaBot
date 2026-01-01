import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: ListView.view.width
    height: contentLayout.height + 20 // Padding

    function bubbleWidth() {
        return root.width * 0.85;
    }

    property var messageData: modelData // {role, content}
    property bool isUser: messageData ? messageData.role === "user" : false
    property bool isThought: messageData ? messageData.role === "thought" : false

    RowLayout {
        id: contentLayout
        anchors.top: parent.top
        anchors.topMargin: 10
        width: parent.width - 20
        anchors.horizontalCenter: parent.horizontalCenter
        layoutDirection: isUser ? Qt.RightToLeft : Qt.LeftToRight
        spacing: 10
        
        Rectangle {
            id: userframe
            width: 40; height: 40; radius: 20
            color: isUser ? themeManager.color("windowDarker") : themeManager.color("windowDarker2")
            opacity: isThought ? 0.6 : 1.0
            Layout.alignment: Qt.AlignTop
            Label {
                anchors.centerIn: parent
                text: isUser ? "🧑" : (isThought ? "💭" : "🤖")
                font.pixelSize: 20
                opacity: isThought ? 0.7 : 1.0
            }
        }
        
        Rectangle {
            id: bubble
            Layout.maximumWidth: root.width * 0.95
            Layout.preferredWidth: bubbleWidth()
            Layout.preferredHeight: msgText.implicitHeight + 20
            color: isUser ? themeManager.color("windowDarker") : themeManager.color("windowDarker2")
            border.width: isThought ? 1 : 0
            border.color: themeManager.color("windowDarker")
            opacity: isThought ? 0.8 : 1.0
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
                    color: isThought ? themeManager.color("textDisabled") : themeManager.color("text")
                    font.italic: isThought
                    font.pixelSize: isThought ? 13 : 14
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

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: themeManager
        function onDarkModeChanged() {
            userframe.color = isUser ? themeManager.color("windowDarker") : themeManager.color("windowDarker2")
            bubble.color = isUser ? themeManager.color("windowDarker") : themeManager.color("windowDarker2")
            msgText.color = isThought ? themeManager.color("textDisabled") : themeManager.color("text")
        }
    }
    
}
