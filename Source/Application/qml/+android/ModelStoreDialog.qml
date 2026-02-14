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
    width: Screen.width
    height: Screen.height
    anchors.centerIn: parent

    padding: 0
    topPadding: 0
    spacing: 0

    background: Rectangle {
        radius: 0
        border.width: 0
    }

    header: Pane {

        padding: 0

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
        radius: 0
        anchors.fill: parent
        color: ThemeManager.color("window")

        property bool isDownloading: ModelStore.isDownloading
        property string statusMessage: ModelStore.statusMessage

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5

            // Toolbar: Source Selection and Filter by size
            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                // Source Selector
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: sourceComboLbl
                        text: "Source"
                        font.bold: true
                        color: ThemeManager.color("text")
                    }
                    ComboBox {
                        id: sourceCombo
                        model: ModelStore.availableSources
                        Layout.preferredWidth: 150

                        palette {
                            buttonText: ThemeManager.color("buttonText")
                            button: ThemeManager.color("button")
                            window: ThemeManager.color("window")
                            text: ThemeManager.color("text")
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
                        color: ThemeManager.color("text")
                    }
                    ComboBox {
                        id: sortCombo
                        model: ["Trending", "Likes", "Date"]
                        palette {
                            buttonText: ThemeManager.color("buttonText")
                            button: ThemeManager.color("button")
                            window: ThemeManager.color("window")
                            text: ThemeManager.color("text")
                        }
                        onCurrentTextChanged: ModelStore.setSort(currentText)
                    }
                }

                // Size Filter
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: sizeFilterLbl
                        text: "By size"
                        font.bold: true
                        color: ThemeManager.color("text")
                    }
                    ComboBox {
                        id: sizeFilter
                        model: ["All", "2B", "4B", "8B", "20B"]
                        Layout.preferredWidth: 85
                        palette {
                            buttonText: ThemeManager.color("buttonText")
                            button: ThemeManager.color("button")
                            window: ThemeManager.color("window")
                            text: ThemeManager.color("text")
                        }
                        onCurrentTextChanged: ModelStore.setSizeFilter(currentText)
                    }
                }
            }

            // Toolbar: Filter by Text
            RowLayout {
                Layout.fillWidth: true
                spacing: 5
                // Name Filter
                ColumnLayout {
                    spacing: 5
                    Label {
                        id: mustContainsFieldLbl
                        text: "By name"
                        font.bold: true
                        color: ThemeManager.color("text")
                    }
                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        TextField {
                            id: mustContainsField
                            text: ModelStore.searchName
                            Layout.preferredWidth: 250
                            Layout.fillWidth: true
                            palette {
                                text: ThemeManager.color("text")
                                base: ThemeManager.color("windowDarker")
                            }
                            onEditingFinished: ModelStore.searchName = text
                        }
                        Button {
                            id: refreshBtn
                            text: "Refresh"
                            palette {
                                buttonText: ThemeManager.color("buttonText")
                                button: ThemeManager.color("button")
                            }
                            onClicked: ModelStore.fetchModels()
                        }
                    }
                }
            }

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
                        color: modelListView.currentIndex === modelDelegate.index ? ThemeManager.color("windowDarker") : (modelDelegate.hovered ? ThemeManager.color("windowDarker2") : "transparent")
                        border.color: modelListView.currentIndex === modelDelegate.index ? ThemeManager.color("button") : "transparent"
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
                                color: ThemeManager.color("text")
                            }

                            Label {
                                id: sizeLbl
                                Layout.alignment: Qt.AlignTop | Qt.AlignRight
                                font.pixelSize: 14
                                text: modelDelegate.modelData.size ? (modelDelegate.modelData.size / 1024 / 1024 / 1024).toFixed(2) + " GB" : "Unknown"
                                color: ThemeManager.color("text")
                            }
                        }

                        Label {
                            id: descLbl
                            Layout.fillWidth: true
                            opacity: 0.7
                            font.pixelSize: 12
                            elide: Text.ElideRight
                            text: modelDelegate.modelData.description || ""
                            color: ThemeManager.color("text")
                        }
                    }

                    onClicked: {
                        modelListView.currentIndex = index;
                        detailsPanel.manifest = modelsModel.get(index);
                        detailsPanel.details = ({});
                        ModelStore.fetchModelDetails(modelData.name || modelData.tag);
                    }

                    Connections {
                        target: ThemeManager
                        function onDarkModeChanged() {
                            delegateBackground.color = "transparent";
                            delegateBackground.border.color = ThemeManager.color("button");
                            nameLbl.color = sizeLbl.color = descLbl.color = ThemeManager.color("text");
                        }
                    }
                }
            }

            // Model Details Panel
            Rectangle {
                id: detailsPanel
                Layout.fillWidth: true
                Layout.preferredHeight: 300

                visible: modelListView.currentIndex >= 0
                color: ThemeManager.color("windowDarker")
                border.color: ThemeManager.color("button")
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
                            color: ThemeManager.color("text")
                        }
                        Button {
                            text: "Close"
                            Layout.alignment: Qt.AlignTop | Qt.AlignRight
                            palette {
                                buttonText: ThemeManager.color("buttonText")
                                button: ThemeManager.color("button")
                            }
                            onClicked: {
                                modelListView.currentIndex = -1;
                            }
                        }
                    }

                    RowLayout {
                        Layout.alignment: Qt.AlignLeft
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? "🔥" : " "
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelTrendingLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? detailsPanel.manifest.trending : " "
                            font.pixelSize: 14
                            color: ThemeManager.color("text")
                        }
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? "⭐" : " "
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelLikesLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? detailsPanel.manifest.likes : " "
                            font.pixelSize: 14
                            color: ThemeManager.color("text")
                        }
                        Label {
                            text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? "📥" : " "
                            font.family: ThemeManager.colorEmojiFont
                            font.pixelSize: 18
                        }
                        Label {
                            id: detailsPanelDownloadsLbl
                            text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? detailsPanel.manifest.downloads : " "
                            font.pixelSize: 14
                            color: ThemeManager.color("text")
                        }
                    }

                    Label {
                        id: detailsPanelDescriptionLbl
                        Layout.fillWidth: true
                        Layout.preferredWidth: Screen.width * 0.8
                        Layout.alignment: Qt.AlignHCenter
                        wrapMode: Text.Wrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                        text: detailsPanel.manifest && detailsPanel.manifest.desc || "Loading description ..."
                        color: ThemeManager.color("text")
                    }

                    ComboBox {
                        id: filesCombo
                        Layout.preferredWidth: Screen.width * 0.8
                        Layout.alignment: Qt.AlignHCenter
                        model: detailsPanel.details.files
                        textRole: "name"
                        palette {
                            buttonText: ThemeManager.color("buttonText")
                            button: ThemeManager.color("button")
                            window: ThemeManager.color("window")
                            text: ThemeManager.color("text")
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
                                buttonText: ThemeManager.color("buttonText")
                                button: ThemeManager.color("button")
                            }
                            onClicked: {
                                ModelStore.downloadFile(detailsPanel.details.name, detailsPanel.details.files[filesCombo.currentIndex]);
                            }
                        }

                        Button {
                            text: "Download All Files"
                            enabled: !root.isDownloading
                            palette {
                                buttonText: ThemeManager.color("buttonText")
                                button: ThemeManager.color("button")
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

                    Button {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        text: "Cancel"
                        palette {
                            buttonText: ThemeManager.color("buttonText")
                            button: ThemeManager.color("button")
                        }
                        onClicked: {
                            ModelStore.cancelDownload();
                        }
                    }
                    ProgressBar {
                        id: downloadBar
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        width: parent.width * 0.75
                        value: ModelStore.downloadProgress
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff

                    Text {
                        id: downPanelMessage
                        width: implicitWidth
                        text: root.statusMessage
                        font.italic: true
                        color: ThemeManager.color("text")
                    }
                }
            }

            // reserve the place for the android bottom nav bar
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                Layout.alignment: Qt.AlignBottom
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

        Connections {
            target: ThemeManager
            function onDarkModeChanged() {
                root.color = ThemeManager.color("window");
                sourceComboLbl.color = ThemeManager.color("text");
                sourceCombo.palette.buttonText = ThemeManager.color("buttonText");
                sourceCombo.palette.button = ThemeManager.color("button");
                sourceCombo.palette.window = ThemeManager.color("window");
                sourceCombo.palette.text = ThemeManager.color("text");
                sortComboLbl.color = ThemeManager.color("text");
                sortCombo.palette.buttonText = ThemeManager.color("buttonText");
                sortCombo.palette.button = ThemeManager.color("button");
                sortCombo.palette.window = ThemeManager.color("window");
                sortCombo.palette.text = ThemeManager.color("text");
                sizeFilterLbl.color = ThemeManager.color("text");
                sizeFilter.palette.buttonText = ThemeManager.color("buttonText");
                sizeFilter.palette.button = ThemeManager.color("button");
                sizeFilter.palette.window = ThemeManager.color("window");
                sizeFilter.palette.text = ThemeManager.color("text");
                mustContainsFieldLbl.color = ThemeManager.color("text");
                mustContainsField.palette.text = ThemeManager.color("text");
                mustContainsField.palette.base = ThemeManager.color("windowDarker");
                detailsPanel.color = ThemeManager.color("windowDarker");
                detailsPanel.border.color = ThemeManager.color("button");
                detailsPanelNameLbl.color = ThemeManager.color("text");
                detailsPanelTrendingLbl.color = ThemeManager.color("text");
                detailsPanelLikesLbl.color = ThemeManager.color("text");
                detailsPanelDownloadsLbl.color = ThemeManager.color("text");
                detailsPanelDescriptionLbl.color = ThemeManager.color("text");
                filesCombo.palette.buttonText = ThemeManager.color("buttonText");
                filesCombo.palette.button = ThemeManager.color("button");
                filesCombo.palette.window = ThemeManager.color("window");
                filesCombo.palette.text = ThemeManager.color("text");
                downPanelMessage.color = ThemeManager.color("text");
            }
        }

        // Initial fetch
        Component.onCompleted: {
            ModelStore.fetchModels(); // Triggered by sourceCombo initial set or manually?
            // Let sourceCombo trigger it via bindings
        }
    }
}
