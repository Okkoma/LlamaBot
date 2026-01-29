pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Item {
    id: root

    readonly property ThemeManager themeManager: app.themeManager

    required property var modelData

    width: ListView.view.width
    height: contentLayout.height + 20 // Padding

    function bubbleWidth() {
        return root.width * 0.85;
    }

    property bool isUser: modelData ? modelData.role === "user" : false
    property bool isThought: modelData ? modelData.role === "thought" : false

    RowLayout {
        id: contentLayout
        anchors.top: root.top
        anchors.topMargin: 10
        width: root.width - 20
        anchors.horizontalCenter: root.horizontalCenter
        layoutDirection: root.isUser ? Qt.RightToLeft : Qt.LeftToRight
        spacing: 10
        
        Rectangle {
            id: userframe
            width: 40; height: 40; radius: 20
            color: root.isUser ? root.themeManager.color("windowDarker") : root.themeManager.color("windowDarker2")
            opacity: root.isThought ? 0.6 : 1.0
            Layout.alignment: Qt.AlignTop
            Label {
                anchors.centerIn: userframe
                text: root.isUser ? "🧑" : (root.isThought ? "💭" : "🤖")
                font.pixelSize: 20
                opacity: root.isThought ? 0.7 : 1.0
            }
        }
        
        Rectangle {
            id: bubble
            Layout.maximumWidth: root.width * 0.95
            Layout.preferredWidth: root.bubbleWidth()
            Layout.preferredHeight: msgText.contentHeight + 20
            color: root.isUser ? root.themeManager.color("windowDarker") : root.themeManager.color("windowDarker2")
            border.width: 0
            opacity: root.isThought ? 0.8 : 1.0
            radius: 10
            
            Item {
                anchors.fill: bubble
                anchors.margins: 10
                TextEdit {
                    id: msgText
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    text: root.modelData ? root.modelData.content : ""
                    color: root.isThought ? root.themeManager.color("buttonText") : root.themeManager.color("text")
                    font.family: root.themeManager.currentFont
                    font.pixelSize: root.isThought ? root.themeManager.currentFontSize * 0.8 : root.themeManager.currentFontSize
                    font.italic: root.isThought
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.MarkdownText
                    selectByMouse: true
                    readOnly: true
                }
            }
        }
        
        Item { Layout.fillWidth: true } // Spacer
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: root.themeManager
        function onDarkModeChanged() {
            userframe.color = root.isUser ? root.themeManager.color("windowDarker") : root.themeManager.color("windowDarker2")
            bubble.color = root.isUser ? root.themeManager.color("windowDarker") : root.themeManager.color("windowDarker2")
            msgText.color = root.isThought ? root.themeManager.color("buttonText") : root.themeManager.color("text")
        }
        function onFontChanged() {
            msgText.font.family = root.themeManager.currentFont
            msgText.font.pixelSize = root.isThought ? root.themeManager.currentFontSize * 0.8 : root.themeManager.currentFontSize
        }
    }
}
