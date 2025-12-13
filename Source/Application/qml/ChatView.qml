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
        
        // Auto-scroll logic
        onCountChanged: {
            Qt.callLater(() => positionViewAtEnd())
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
