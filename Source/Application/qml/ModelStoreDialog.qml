pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Dialog {
    id: rootdlg

    title: "Model Store"

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
        color: ThemeManager.windowColor

        property bool isDownloading: ModelStore.isDownloading
        property string statusMessage: ModelStore.statusMessage

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            // Toolbar: Source Selection and Filters
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                // Source Selector
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: sourceComboLbl
                        text: "Source"
                        color: ThemeManager.textColor
                        font.bold: true
                    }

                    ComboBox {
                        id: sourceCombo
                        model: ModelStore.availableSources
                        Layout.preferredWidth: 150

                        palette {
                            buttonText: ThemeManager.buttonTextColor
                            button: ThemeManager.buttonColor
                            window: ThemeManager.windowColor
                            text: ThemeManager.textColor
                        }

                        onCurrentTextChanged: {
                            if (currentText !== "") {
                                ModelStore.currentSource = currentText;
                                ModelStore.fetchModels();
                            }
                        }

                        Component.onCompleted: {
                            // Set initial selection
                            currentIndex = find(ModelStore.currentSource);
                        }
                    }
                }

                // Sort Order
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: sortComboLbl
                        text: "Sort By"
                        font.bold: true
                        color: ThemeManager.textColor
                    }
                    ComboBox {
                        id: sortCombo
                        model: ["Trending", "Likes", "Date"]
                        Layout.preferredWidth: 140
                        palette {
                            buttonText: ThemeManager.buttonTextColor
                            button: ThemeManager.buttonColor
                            window: ThemeManager.windowColor
                            text: ThemeManager.textColor
                        }
                        onCurrentTextChanged: ModelStore.setSort(currentText)
                    }
                }

                // Size Filter
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: sizeFilterLbl
                        text: "By param"
                        font.bold: true
                        color: ThemeManager.textColor
                    }
                    ComboBox {
                        id: sizeFilter
                        model: ["All", "<2B", "<4B", "<8B", "<20B"]
                        Layout.preferredWidth: 100
                        palette {
                            buttonText: ThemeManager.buttonTextColor
                            button: ThemeManager.buttonColor
                            window: ThemeManager.windowColor
                            text: ThemeManager.textColor
                        }
                        onCurrentTextChanged: ModelStore.setSizeFilter(currentText)
                    }
                }

                // Name Filter
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: mustContainsFieldLbl
                        text: "By tag"
                        font.bold: true
                        color: ThemeManager.textColor
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        TextField {
                            id: mustContainsField
                            text: ModelStore.searchName
                            Layout.preferredWidth: 250
                            palette {
                                text: ThemeManager.textColor
                                base: ThemeManager.windowDarkerColor
                            }
                            Layout.fillWidth: true
                            onEditingFinished: ModelStore.searchName = text
                        }
                        Button {
                            id: refreshBtn
                            text: "Refresh"
                            palette {
                                buttonText: ThemeManager.buttonTextColor
                                button: ThemeManager.buttonColor
                            }
                            onClicked: ModelStore.fetchModels()
                        }
                    }
                }
            }
            /*
            RowLayout {
                Layout.fillWidth: true
                spacing: 15

                // HF Token
                ColumnLayout {
                    spacing: 5
                    Label {
                        text: "Auth Token (optional)"
                        color: ThemeManager.textColor
                        font.bold: true
                    }
                    TextField {
                        id: bearerTokenField
                        placeholderText: "hf_..."
                        text: ModelStore.bearerToken
                        echoMode: TextField.Password
                        Layout.preferredWidth: 250
                        palette {
                            text: ThemeManager.textColor
                            base: ThemeManager.windowDarkerColor
                        }
                        onEditingFinished: ModelStore.bearerToken = text
                    }
                }
            }
    */
            // Models List
            ListView {
                id: modelListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 5

                model: ListModel {
                    id: modelsModel
                }

                // Fermer detailsPanel lors du défilement
                onMovementStarted: {
                    currentIndex = -1;
                }

                delegate: ItemDelegate {
                    id: modelDelegate
                    width: modelListView.width
                    height: 60

                    required property var modelData
                    required property int index

                    background: Rectangle {
                        id: delegateBackground
                        color: modelListView.currentIndex === modelDelegate.index ? ThemeManager.windowDarkerColor : (modelDelegate.hovered ? ThemeManager.windowDarker2Color : 'transparent')
                        border.color: modelListView.currentIndex === modelDelegate.index ? ThemeManager.buttonColor : 'transparent'
                        border.width: 1
                    }

                    contentItem: ColumnLayout {
                        spacing: 2

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10

                            Label {
                                id: nameLbl
                                Layout.fillWidth: true
                                Layout.preferredWidth: 100
                                font.bold: true
                                font.pixelSize: 16
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                text: modelDelegate.modelData.name || ""
                                color: ThemeManager.textColor
                            }

                            Label {
                                id: sizeLbl
                                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                                font.pixelSize: 14
                                text: modelDelegate.modelData.size ? (modelDelegate.modelData.size / 1024 / 1024 / 1024).toFixed(2) + " GB" : ""

                                color: ThemeManager.textColor
                            }
                        }

                        Label {
                            id: descLbl
                            text: modelDelegate.modelData.description || ""
                            color: ThemeManager.textColor
                            opacity: 0.7
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    onClicked: {
                        modelListView.currentIndex = index;
                        detailsPanel.manifest = modelsModel.get(index);
                        detailsPanel.details = ({});
                        ModelStore.fetchModelDetails(modelData.name || modelData.tag);
                    }
                }
            }

            // Model Details Panel
            Rectangle {
                id: detailsPanel
                Layout.fillWidth: true
                Layout.preferredHeight: 300

                visible: modelListView.currentIndex >= 0
                color: ThemeManager.windowDarkerColor
                border.color: ThemeManager.buttonColor
                border.width: 1
                radius: 16

                // Property to hold current manifest, details
                property var manifest: ({})
                property var details: ({})

                ColumnLayout {
                    anchors.fill: parent
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.margins: 10
                    spacing: 5

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Label {
                            id: detailsPanelNameLbl
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            font.bold: true
                            font.pixelSize: 18
                            wrapMode: Text.Wrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                            text: detailsPanel.manifest && detailsPanel.manifest.name || "Select a model"
                            color: ThemeManager.textColor
                        }

                        Button {
                            text: "Close"
                            Layout.alignment: Qt.AlignTop | Qt.AlignRight
                            palette {
                                buttonText: ThemeManager.buttonTextColor
                                button: ThemeManager.buttonColor
                            }
                            onClicked: {
                                modelListView.currentIndex = -1;
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignLeft
                        
                        Label {
                            id: detailsPanelDateLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.date) ? "lastUpdate: " + detailsPanel.manifest.date : ""
                            font.pixelSize: 14
                            color: ThemeManager.textColor
                        }
                        Label {
                            id: detailsPanelPipelineLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.pipeline) ? "- " + detailsPanel.manifest.pipeline : ""
                            font.pixelSize: 14
                            color: ThemeManager.textColor
                        }                        
                        Item { // Spacer
                            Layout.fillWidth: true
                        }
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? "🔥" : ""
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelTrendingLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? detailsPanel.manifest.trending : ""
                            font.pixelSize: 14
                            color: ThemeManager.textColor
                        }
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? "⭐" : ""
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelLikesLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? detailsPanel.manifest.likes : ""
                            font.pixelSize: 14
                            color: ThemeManager.textColor
                        }
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? "📥" : ""
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelDownloadsLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? detailsPanel.manifest.downloads : ""
                            font.pixelSize: 14
                            color: ThemeManager.textColor
                        }
                    }

                    Label {
                        id: detailsPanelDescriptionLbl
                        Layout.fillWidth: true
                        Layout.preferredWidth: parent.width * 0.8
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.Wrap
                        maximumLineCount: 5
                        elide: Text.ElideRight
                        text: detailsPanel.manifest && detailsPanel.manifest.desc || "Loading description ..."
                        color: ThemeManager.textColor
                    }

                    ComboBox {
                        id: filesCombo
                        Layout.preferredWidth: parent.width * 0.8
                        Layout.alignment: Qt.AlignHCenter
                        model: detailsPanel.details.files
                        textRole: "name"
                        palette {
                            buttonText: ThemeManager.buttonTextColor
                            button: ThemeManager.buttonColor
                            window: ThemeManager.windowColor
                            text: ThemeManager.textColor
                        }
                        popup.height: 200
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter

                        Button {
                            text: "Download File"
                            enabled: !root.isDownloading
                            palette {
                                buttonText: ThemeManager.buttonTextColor
                                button: ThemeManager.buttonColor
                            }
                            onClicked: {
                                ModelStore.downloadFile(detailsPanel.details.name, detailsPanel.details.files[filesCombo.currentIndex]);
                            }
                        }

                        Button {
                            text: "Download All Files"
                            enabled: !root.isDownloading
                            palette {
                                buttonText: ThemeManager.buttonTextColor
                                button: ThemeManager.buttonColor
                            }
                            onClicked: {
                                ModelStore.downloadAllFiles(detailsPanel.details.name, detailsPanel.details.files);
                            }
                        }
                    }
                }
            }

            // Status Bar
            ColumnLayout {
                Layout.fillWidth: true

                visible: root.statusMessage.length > 0

                Rectangle {
                    id: downloadPanel
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    visible: root.isDownloading
                    color: ThemeManager.windowColor

                    Button {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        text: "Cancel"
                        palette {
                            buttonText: ThemeManager.buttonTextColor
                            button: ThemeManager.buttonColor
                        }
                        onClicked: {
                            ModelStore.cancelDownload();
                        }
                    }
                    ProgressBar {
                        id: downloadBar
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        width: parent.width * 0.85
                        value: ModelStore.downloadProgress
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                    Text {
                        id: downPanelMessage
                        text: root.statusMessage
                        color: ThemeManager.textColor
                        font.italic: true
                        width: implicitWidth
                    }
                }
            }
        }

        Connections {
            target: ModelStore

            function onModelsListChanged(models) {
                modelsModel.clear();
                for (var i = 0; i < models.length; i++) {
                    modelsModel.append(models[i]);
                }
                // Clear selection
                modelListView.currentIndex = -1;
                detailsPanel.manifest = ({});
                detailsPanel.details = ({});
            }

            function onModelDetailsChanged(details) {
                detailsPanel.manifest.size = details.maxSize;
                detailsPanel.details = details;
                if (detailsPanel.details.files && detailsPanel.details.files.length > 0) {
                    filesCombo.visible = true;
                }
            }

            function onErrorOccurred(error) {
                // Can show a popup or just status
                console.error(error);
            }

            function onDownloadFinished(success) {
                if (success && ChatController) {
                    // Rafraîchir la liste des modèles disponibles après un téléchargement réussi
                    ChatController.refreshModels();
                }
            }
        }

        // Initial fetch
        Component.onCompleted: {
            ModelStore.fetchModels(); // Triggered by sourceCombo initial set or manually?
            // Let sourceCombo trigger it via bindings
        }
    }
}
