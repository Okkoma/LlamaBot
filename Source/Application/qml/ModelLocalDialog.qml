pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Dialog {
    id: rootdlg
    title: "Model Local"

    modal: true
    width: 800
    height: 800
    anchors.centerIn: parent

    padding: 0
    topPadding: 0
    spacing: 0

    header: Pane {

        padding: 0 // Supprime l'espace interne du Pane

        background: Rectangle {
            color: "#FF4081"
        }

        contentItem: RowLayout {
            Label {
                text: rootdlg.title
                font.pixelSize: 24
                elide: Label.ElideRight
                Layout.fillWidth: true
                Layout.leftMargin: 10
            }

            Button {
                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                text: "Close"
                onClicked: rootdlg.close()
            }
        }
    }

    Rectangle {
        id: root
        anchors.fill: parent
        anchors.centerIn: parent
        color: ThemeManager.color("window")

        FlexboxLayout {
            id: flexLayout
            anchors.fill: parent
            wrap: FlexboxLayout.Wrap
            direction: FlexboxLayout.Row
            justifyContent: FlexboxLayout.JustifySpaceAround
            Rectangle {
                color: 'teal'
                implicitWidth: 200
                implicitHeight: 200
            }
            Rectangle {
                color: 'plum'
                implicitWidth: 200
                implicitHeight: 200
            }
            Rectangle {
                color: 'olive'
                implicitWidth: 200
                implicitHeight: 200
            }
            Rectangle {
                color: 'beige'
                implicitWidth: 200
                implicitHeight: 200
            }
            Rectangle {
                color: 'darkseagreen'
                implicitWidth: 200
                implicitHeight: 200
            }
        }
    }
}