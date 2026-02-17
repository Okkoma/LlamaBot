pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls

import LlamaBotQml

ListView {
    id: root

    clip: true
    spacing: 10

    model: ChatController.currentChat

    delegate: MessageDelegate {}

    // Visible scrollbar
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
    }

    // Propriété interne pour savoir si on doit auto-scroller
    property bool autoScroll: true

    onMovementEnded: {
        // Si l'utilisateur arrête de scroller et qu'il est en bas, on réactive l'auto-scroll
        // Sinon (s'il est remonté), on le désactive.
        autoScroll = atYEnd;
        smartScroll();
    }

    // Helper function to conditionally scroll only if user is at the bottom
    function smartScroll() {
        if (autoScroll) {
            // On utilise Qt.callLater pour s'assurer que la ListView a fini
            // de calculer la hauteur du nouvel élément avant de scroller.
            Qt.callLater(root.positionViewAtEnd);
        }
    }

    Connections {
        target: ChatController.currentChat
        // Only handle signals if target is valid
        enabled: ChatController.currentChat !== null
        function onMessagesChanged() {
            // Smart auto-scroll during streaming updates
            root.smartScroll();
        }
    }
}
