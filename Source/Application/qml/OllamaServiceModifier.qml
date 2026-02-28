pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import LlamaBotQml

Dialog {
    id: root

    title: "Ollama service"
    modal: true

    width: parent.width - 20
    height: 400

    anchors.centerIn: parent

    palette {
        buttonText: ThemeManager.buttonTextColor
        button: ThemeManager.buttonColor
        window: ThemeManager.windowColor
        text: ThemeManager.textColor
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 20

        // Margins to prevent content from touching edges or being covered by scrollbar
        Layout.leftMargin: 20
        Layout.rightMargin: 25
        Layout.topMargin: 10
        Layout.bottomMargin: 10

        // spacer
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: ThemeManager.spacerColor
        }

        TextField {

            id: urlField
            placeholderText: "Url"
            text: ChatController.getAPI("Ollama")["url"]
            Layout.fillWidth: true

            readonly property bool isValid: urlField.acceptableInput
            // Aide visuelle pour le clavier mobile (ex: ajoute '.' et '/' sur le clavier)
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText
            // Restriction stricte : n'autorise que les lettres, chiffres,
            // tirets, points, slashs, underscores et deux-points.
            validator: RegularExpressionValidator {
                regularExpression: /[a-zA-Z0-9-._:\/]+/
            }
            // On ajoute un élément VISUEL par-dessus sans toucher au 'background'
            Rectangle {
                anchors.fill: parent
                anchors.margins: 5 // Un peu moins large pour entourer le champ
                color: 'transparent'
                border.width: 2
                border.color: "red"
                radius: 5
                visible: !urlField.isValid
                z: -1 // Pour ne pas bloquer les clics de souris
            }
        }

        TextField {
            id: apiKeyField
            placeholderText: "API Key"
            text: ChatController.getAPI("Ollama")["apikey"]
            font.pixelSize: 8
            Layout.fillWidth: true
            echoMode: TextInput.Password
            selectByMouse: false // Désactive la sélection pour empêcher la copie
        }
    }

    footer: DialogButtonBox {
        padding: 10
        alignment: Qt.AlignRight

        Button {
            text: "Apply"
            anchors.bottomMargin: 50
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: {
                if (!ChatController.modifyAPI("Ollama", "Ollama", urlField.text, apiKeyField.text)) {
                    console.log("modifyAPI: ollama service updated !");
                }
                root.close();
            }
            enabled: urlField.isValid
        }

        Button {
            text: "Cancel"
            anchors.bottomMargin: 50
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: {
                root.close();
            }
        }
    }
}
