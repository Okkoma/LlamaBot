pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls

import LlamaBotQml

Item {
    id: root

    // Use QtObject to satisfy Connections target requirement and qmllint check
    // Use var to avoid qmllint being unable to resolve Chat -> QObject inheritance
    readonly property var currentChat: ChatController.currentChat

    ListView {
        id: messageList
        anchors.top: parent.top
        anchors.bottom: inputArea.top
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        spacing: 10
        
        model: ChatController.currentChat
        
        delegate: MessageDelegate {
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
            target: root.currentChat
            // Only handle signals if target is valid
            enabled: root.currentChat !== null
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
        anchors.bottom: messageErrorView.top
        anchors.left: parent.left
        anchors.right: parent.right
        
        onAccepted: (text) => {
            if (ChatController)
                ChatController.sendMessage(text)
        }
    }

    // Message Error View
    ListView {
        id: messageErrorView
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        visible: false
        height: visible ? 40 : 0

        delegate: Text {
            required property var modelData
            required property int index
            text: index !== -1 ? modelData : ""
            color: ThemeManager.color("text")
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
        }
    }

    Connections {
        target: ErrorMessenger
        function onErrorPopped(error) {
            messageErrorView.model = ErrorMessenger.get(10)
            messageErrorView.visible = true
        }
    }    
}
