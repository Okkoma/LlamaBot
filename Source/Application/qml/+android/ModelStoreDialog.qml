pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Rectangle {
    id: root

    signal closeRequested()

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
                    text: "Source"
                    font.bold: true
                }
                ComboBox {
                    id: sourceCombo
                    model: ModelStore.availableSources
                    Layout.preferredWidth: 150
                    
                    onCurrentTextChanged: {
                        if (currentText !== "") {
                            ModelStore.currentSource = currentText
                            ModelStore.fetchModels()
                        }
                    }
                    
                    Component.onCompleted: {
                        // Set initial selection
                        currentIndex = find(ModelStore.currentSource)
                    }
                }
            }

            // Sort Order
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Sort By"
                    font.bold: true
                }
                ComboBox {
                    id: sortCombo
                    model: ["Trending", "Likes", "Date"]
                    Layout.preferredWidth: 140
                    onCurrentTextChanged: ModelStore.setSort(currentText)
                }
            }
            
            // Size Filter
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Size Filter"
                    font.bold: true
                }
                ComboBox {
                    id: sizeFilter
                    model: ["All", "2B", "4B", "8B", "20B"]
                    Layout.preferredWidth: 100
                    onCurrentTextChanged: ModelStore.setSizeFilter(currentText)
                }
            }

            // Name Filter
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Name Filter"
                    font.bold: true
                }
                TextField {
                    id: mustContainsField
                    text: ModelStore.searchName
                    Layout.preferredWidth: 250
                    onEditingFinished: ModelStore.searchName = text
                }
            }            

            Item { Layout.fillWidth: true } // Spacer
            
            Button {
                text: "Refresh"
                onClicked: ModelStore.fetchModels()
            }            
        }
        // Models List
        ListView {
            id: modelListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 5
            
            model: ListModel { id: modelsModel }

            // Fermer detailsPanel lors du défilement
            onMovementStarted: {
                currentIndex = -1
            }

            delegate: ItemDelegate {
                id: modelDelegate
                width: modelListView.width
                height: 60

                required property var modelData
                required property int index

                background: Rectangle {
                    border.width: 1
                }
                
                contentItem: ColumnLayout {
                    spacing: 2
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: modelDelegate.modelData.name || ""
                            font.bold: true
                            font.pixelSize: 16
                            Layout.fillWidth: true
                        }
                        Label {
                            text: modelDelegate.modelData.size ? (modelDelegate.modelData.size / 1024 / 1024 / 1024).toFixed(2) + " GB" : "Unknown"
                            font.pixelSize: 14
                        }
                    }
                    
                    Label {
                        text: modelDelegate.modelData.description || ""
                        opacity: 0.7
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
                
                onClicked: {
                    modelListView.currentIndex = index
                    detailsPanel.manifest = modelsModel.get(index)
                    detailsPanel.details = ({})
                    filesCombo.visible = false
                    ModelStore.fetchModelDetails(modelData.name || modelData.tag)
                }                
            }                   
        }

        // Model Details Panel
        Rectangle {
            id: detailsPanel
            Layout.fillWidth: true
            Layout.preferredHeight: 250
            visible: modelListView.currentIndex >= 0
            border.width: 1
            radius: 4
            
            // Property to hold current manifest, details
            property var manifest: ({})
            property var details: ({})

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 5
                
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.name || "Select a model"
                        font.bold: true
                        font.pixelSize: 18
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? "🔥" : " " 
                        font.family: ThemeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.trending) ? detailsPanel.manifest.trending : " "
                        font.pixelSize: 14
                    }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? "⭐" : " "
                        font.family: ThemeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.likes) ? detailsPanel.manifest.likes : " "
                        font.pixelSize: 14
                    }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? "📥" : " "
                        font.family: ThemeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: (detailsPanel.manifest && detailsPanel.manifest.downloads) ? detailsPanel.manifest.downloads : " "
                        font.pixelSize: 14
                    }
                    
                    Item { Layout.fillWidth: true } // SPACER

                    Button {
                        text: "Close"
                        Layout.alignment: Qt.AlignRight
                        onClicked: { 
                            modelListView.currentIndex = -1 
                        }
                    }                    
                }
                
                Label {
                    text: detailsPanel.manifest && detailsPanel.manifest.desc || "Loading description ..."
                }
                
                ComboBox {
                    id: filesCombo                    
                    model: detailsPanel.details.files
                    textRole: "name"
                    Layout.preferredWidth: 400
                    popup.height: 200
                }

                Item { Layout.fillHeight: true }
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    ProgressBar {
                        id: downloadBar
                        visible: root.isDownloading
                        value: ModelStore.downloadProgress
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Download File"
                        visible: !root.isDownloading
                        onClicked: {
                            ModelStore.downloadFile(detailsPanel.details.name, detailsPanel.details.files[filesCombo.currentIndex]);
                        }
                    }

                    Button {
                        text: "Download All Files"
                        visible: !root.isDownloading
                        onClicked: {
                            ModelStore.downloadAllFiles(detailsPanel.details.name, detailsPanel.details.files);                            
                        }
                    }

                    Button {
                        text: "Cancel"
                        visible: root.isDownloading
                        onClicked: {
                            ModelStore.cancelDownload()
                        }
                    }                    
                }
            }
        }
        
        // Status Bar        
        RowLayout {
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 20
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                
                Text {
                    text: root.statusMessage
                    font.italic: true
                    width: implicitWidth
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                Layout.alignment: Qt.AlignRight
                onClicked: root.closeRequested()
            }
        }
    }
    
    Connections {
        target: ModelStore
        
        function onModelsListChanged(models) {
            modelsModel.clear()
            for (var i = 0; i < models.length; i++) {
                modelsModel.append(models[i])
            }
            // Clear selection
            modelListView.currentIndex = -1
            detailsPanel.manifest = ({})
            detailsPanel.details = ({})
        }
        
        function onModelDetailsChanged(details) {
        
            detailsPanel.manifest.size = details.maxSize
            detailsPanel.details = details
            if (detailsPanel.details.files && detailsPanel.details.files.length > 0) {
                filesCombo.visible = true
            }
        }
        
        function onErrorOccurred(error) {
            // Can show a popup or just status
            console.error(error)
        }
        
        function onDownloadFinished(success) {
            if (success && ChatController) {
                // Rafraîchir la liste des modèles disponibles après un téléchargement réussi
                ChatController.refreshModels()
            }
        }
    }
    
    // Initial fetch
    Component.onCompleted: {
        ModelStore.fetchModels() // Triggered by sourceCombo initial set or manually?
        // Let sourceCombo trigger it via bindings
    }
}
