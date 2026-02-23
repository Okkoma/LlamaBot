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

    title: "Models"
    modal: true
    width: 400
    height: 600
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

        // Model Selection
        Label {
            text: "Available Models"
            font.bold: true
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            implicitHeight: 200
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.width: 5
            contentWidth: availableWidth // never move in horizontal

            background: Rectangle {
                opacity: 0.2
                color: ThemeManager.windowDarkerColor
            }

            ListView {
                id: modelsList
                anchors.fill: parent
                model: ChatController ? ChatController.getAvailableModels() : []

                highlight: Rectangle {
                    // Indispensable pour l'alignement
                    width: modelsList.width
                    height: modelsList.currentItem ? modelsList.currentItem.height : 0

                    color: 'transparent'
                    border.color: ThemeManager.accentColor
                    border.width: 2
                    radius: 10
                }

                highlightFollowsCurrentItem: true
                highlightMoveDuration: 200

                delegate: ItemDelegate {
                    id: modelsDelegate
                    width: ListView.view.width

                    required property int index
                    required property string name

                    contentItem: Text {
                        verticalAlignment: Text.AlignVCenter
                        text: modelsDelegate.name
                        font.bold: true
                        color: ThemeManager.textColor
                    }

                    highlighted: ListView.isCurrentItem
                    onClicked: ListView.view.currentIndex = index

                    background: Item {}
                }

                // Refresh model list when available models change
                Connections {
                    target: ChatController || null
                    function onAvailableModelsChanged() {
                        if (ChatController) {
                            modelsList.model = ChatController.getAvailableModels();
                            var currentModelName = ChatController.currentChat.currentModel;
                            console.log("Models: onAvailableModelsChanged:", modelsList.currentIndex, currentModelName);
                        }
                    }
                    function onCurrentModelChanged() {
                        modelsList.currentIndex = ChatController.currentModelIndex();
                        console.log("Models: onCurrentModelChanged:", modelsList.currentIndex, ChatController.currentModel);
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20
            Button {
                text: "Apply"
                onClicked: {
                    console.log("Select Model", modelsList.currentIndex, ChatController.getAvailableModels()[modelsList.currentIndex].name);
                    ChatController.setCurrentModel(ChatController.getAvailableModels()[modelsList.currentIndex].name);
                }
            }
            Button {
                text: "Delete"
                onClicked: {
                    console.log("Delete Model", modelsList.currentIndex, ChatController.getAvailableModels()[modelsList.currentIndex].name);
                    ChatController.deleteModel(modelsList.currentIndex);
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: ThemeManager.buttonColor
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Button {
                text: "Download Model"
                onClicked: {
                    console.log("Download Model");
                    modelStoreDialog.open();
                }
            }
        }
    }

    // Model Store Dialog
    ModelStoreDialog {
        id: modelStoreDialog
    }
}
