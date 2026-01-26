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
        model: (chatController && chatController.currentChat) ? chatController.currentChat : []
        
        delegate: MessageDelegate {
            messageData: model
        }
        // Visible scrollbar
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        // Propriété interne pour savoir si on doit auto-scroller
        property bool autoScroll: true

        onMovementEnded: {
            // Si l'utilisateur arrête de scroller et qu'il est en bas, on réactive l'auto-scroll
            // Sinon (s'il est remonté), on le désactive.
            autoScroll = atYEnd
            smartScroll()
        }

        // Helper function to conditionally scroll only if user is at the bottom
        function smartScroll() {
            if (autoScroll) {
                // On utilise Qt.callLater pour s'assurer que la ListView a fini 
                // de calculer la hauteur du nouvel élément avant de scroller.                
                Qt.callLater(messageList.positionViewAtEnd)
            }
        }

        Connections {
            target: chatController && chatController.currentChat ? chatController.currentChat : null
            function onMessagesChanged() {
                // Smart auto-scroll during streaming updates
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
