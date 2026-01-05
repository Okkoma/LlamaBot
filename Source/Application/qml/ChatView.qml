import QtQuick
import QtQuick.Controls

Item {
    id: root

    ListView {
        id: messageList
        anchors.top: parent.top
        anchors.bottom: inputArea.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        spacing: 10
        
        // Ensure we handle null currentChat
        model: (chatController && chatController.currentChat) ? chatController.currentChat.history : []
        
        delegate: MessageDelegate {
            messageData: modelData
        }
        
        add: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1.0; duration: 200 }
            NumberAnimation { property: "scale"; from: 0.95; to: 1.0; duration: 200 }
        }

        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 200; easing.type: Easing.OutQuad }
        }

        // Auto-scroll when new messages are added
        onCountChanged: {
            positionViewAtEnd()
        }
        
        // Auto-scroll during streaming updates
        Connections {
            target: chatController && chatController.currentChat ? chatController.currentChat : null
            function onHistoryChanged() {
                messageList.positionViewAtEnd()
            }
        }
    }
    
    InputArea {
        id: inputArea
        height: 80
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        
        onAccepted: (text) => {
            if (chatController)
                chatController.sendMessage(text)
        }
    }
}
