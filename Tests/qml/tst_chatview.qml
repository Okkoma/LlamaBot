import QtQuick 2.15
import QtTest 1.2

TestCase {
    name: "ChatView"

    function test_initial_state() {
        var component = Qt.createComponent("../../Source/Application/qml/ChatView.qml")
        verify(component.status === Component.Ready, "Le composant ChatView doit être prêt - error: " + component.errorString())

        var view = component.createObject(null)
        verify(view !== null, "L'instance de ChatView doit être créée")

        // Exemple : adapter en fonction de la propriété réelle du composant
        if (view.messages !== undefined && view.messages !== null) {
            compare(view.messages.length, 0, "La liste de messages doit être vide au démarrage")
        }

        view.destroy()
    }
}
