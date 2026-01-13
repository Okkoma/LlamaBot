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
        
        // Visible scrollbar
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        
        // Helper function to conditionally scroll only if user is at the bottom
        function smartScroll() {
            // Only auto-scroll if the user is already viewing the end
            // atYEnd is true when the view is scrolled to the bottom
            if (messageList.atYEnd) {
                positionViewAtEnd()
            }
        }
        
        // Auto-scroll when new messages are added (always scroll for new messages)
        onCountChanged: {
            positionViewAtEnd()
        }
        
        // Smart auto-scroll during streaming updates (only if user is at bottom)
        Connections {
            target: chatController && chatController.currentChat ? chatController.currentChat : null
            function onHistoryChanged() {
                messageList.smartScroll()
            }
        }
    }

    AssetContent {
        id: assetContent
        anchors.bottom: inputArea.top
        anchors.left: parent.left
        anchors.right: parent.right
    }

    InputArea {
        id: inputArea
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        
        onAccepted: (text) => {
            if (chatController)
                chatController.sendMessage(text)
        }
    }
}
