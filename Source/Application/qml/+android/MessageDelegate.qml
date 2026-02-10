pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Item {
    id: root

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
            border.width: 0
            opacity: root.isThought ? 0.8 : 1.0
            radius: 10
            
            Flickable {
                id: flick
                anchors.fill: bubble
                anchors.margins: 10
                contentWidth: msgText.contentWidth
                contentHeight: msgText.contentHeight
                clip: true

                ScrollBar.horizontal : ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                function ensureVisible(r)
                {
                    if (contentX >= r.x)
                        contentX = r.x;
                    else if (contentX+width <= r.x+r.width)
                        contentX = r.x+r.width-width;
                    if (contentY >= r.y)
                        contentY = r.y;
                    else if (contentY+height <= r.y+r.height)
                        contentY = r.y+r.height-height;
                }
     
                TextEdit {
                    id: msgText
                    text: root.modelData ? root.modelData.content : ""
                    font.family: ThemeManager.currentFont
                    font.pixelSize: root.isThought ? ThemeManager.currentFontSize * 0.8 : ThemeManager.currentFontSize
                    font.italic: root.isThought
                    wrapMode: TextEdit.Wrap
                    textFormat: TextEdit.MarkdownText
                    selectByMouse: true
                    readOnly: true
                    width: flick.width
                    focus: true
                    onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)                    
                }
            }
        }
        
        Item { Layout.fillWidth: true } // Spacer
    }

    // Add connection to themeManager to listen for theme changes
    Connections {
        target: ThemeManager
        function onFontChanged() {
            msgText.font.family = ThemeManager.currentFont
            msgText.font.pixelSize = root.isThought ? ThemeManager.currentFontSize * 0.8 : ThemeManager.currentFontSize
        }
    }
}
