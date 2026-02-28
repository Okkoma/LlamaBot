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
    width: Math.min(parent.width * 0.85, 400)
    height: Math.min(parent.height * 0.85, 600)
    anchors.centerIn: parent

    palette {
        buttonText: ThemeManager.buttonTextColor
        button: ThemeManager.buttonColor
        window: ThemeManager.windowColor
        text: ThemeManager.textColor
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 20

        // spacer
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: ThemeManager.spacerColor
        }

        // Service Selection
        Label {
            text: "Services"
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.maximumHeight: 120

            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.width: 5
            contentWidth: availableWidth // never move in horizontal
            background: Rectangle {
                opacity: 0.3
                color: ThemeManager.windowDarkerColor
            }

            ListView {
                id: servicesList
                anchors.fill: parent
                model: ChatController.getAvailableAPIs()

                highlight: Rectangle {
                    // Indispensable pour l'alignement
                    width: servicesList.width
                    height: servicesList.currentItem ? servicesList.currentItem.height : 0

                    color: 'transparent'
                    border.color: ThemeManager.accentColor
                    border.width: 2
                    radius: 10
                }

                highlightFollowsCurrentItem: true
                highlightMoveDuration: 200

                delegate: ItemDelegate {
                    id: servicesDelegate
                    width: ListView.view.width

                    required property int index
                    required property string name

                    contentItem: Text {
                        verticalAlignment: Text.AlignVCenter
                        text: servicesDelegate.name
                        font.bold: true
                        color: ThemeManager.textColor
                    }

                    highlighted: ListView.isCurrentItem
                    onClicked: {
                        ListView.view.currentIndex = index;
                        ChatController.setCurrentApi(servicesDelegate.name);
                        modifyOllamaBtn.enabled = servicesDelegate.name == "Ollama";
                        modelStoreBtn.enabled = servicesDelegate.name == "LlamaCpp";
                        deleteModelBtn.enabled = servicesDelegate.name == "LlamaCpp";
                    }

                    background: Item {}
                }

                Component.onCompleted: {
                    model = ChatController.getAvailableAPIs();
                    currentIndex = ChatController.currentApiIndex();
                }
                
                // Refresh model list when available services change
                Connections {
                    target: ChatController
                    function onAvailableAPIsChanged() {
                        servicesList.model = ChatController.getAvailableAPIs();
                        console.log("Services: onAvailableAPIsChanged:", servicesList.currentIndex, ChatController.currentChat.currentApi);
                    }
                    function onCurrentApiChanged() {
                        servicesList.currentIndex = ChatController.currentApiIndex();
                        console.log("Services: onCurrentAPIChanged:", servicesList.currentIndex, ChatController.currentApi);
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            Button {
                id: modifyOllamaBtn
                text: "Modify"
                enabled: false
                onClicked: {
                    console.log("Modify Service");
                    ollamaServiceModifier.open();
                }
            }
            Button {
                id: modelStoreBtn
                text: "📥 Models Store"
                onClicked: {
                    console.log("Models Store");
                    modelStoreDialog.open();
                }
            }
        }

        // spacer
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: ThemeManager.spacerColor
        }

        // Model Selection
        Label {
            text: "Available Models"
            font.bold: true
        }

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.width: 5
            contentWidth: availableWidth // never move in horizontal

            background: Rectangle {
                opacity: 0.3
                color: ThemeManager.windowDarkerColor
            }

            ListView {
                id: modelsList
                anchors.fill: parent

                model: ChatController.getAvailableModels()

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
                    target: ChatController
                    function onAvailableModelsChanged() {
                        modelsList.model = ChatController.getAvailableModels();
                        var currentModelName = ChatController.currentChat.currentModel;
                        console.log("Models: onAvailableModelsChanged:", modelsList.currentIndex, currentModelName);
                    }
                    function onCurrentModelChanged() {
                        modelsList.currentIndex = ChatController.currentModelIndex();
                        console.log("Models: onCurrentModelChanged:", modelsList.currentIndex, ChatController.currentModel);
                    }
                }
                Connections {
                    target: ModelStore
                    function onModelsListChanged(models) {
                        modelsList.model = ChatController.getAvailableModels();
                        modelsList.currentIndex = ChatController.currentModelIndex();
                        console.log("Models: onModelsListChanged:", modelsList.currentIndex, ChatController.currentModel);
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
                id: deleteModelBtn
                text: "Delete"
                onClicked: {
                    console.log("Delete Model", modelsList.currentIndex, ChatController.getAvailableModels()[modelsList.currentIndex].name);
                    ChatController.deleteModel(modelsList.currentIndex);
                }
            }
        }
    }

    // Model Store Dialog
    ModelStoreDialog {
        id: modelStoreDialog
    }

    OllamaServiceModifier {
        id: ollamaServiceModifier
    }
}
