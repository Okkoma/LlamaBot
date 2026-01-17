import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: themeManager.color("window")

    signal closeRequested()

    property bool isDownloading: modelStoreDialog.isDownloading
    property string statusMessage: modelStoreDialog.statusMessage

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
                    color: themeManager.color("text")
                    font.bold: true
                }
                ComboBox {
                    id: sourceCombo
                    model: modelStoreDialog.availableSources
                    Layout.preferredWidth: 150
                    palette {
                        buttonText: themeManager.color("buttonText")
                        button: themeManager.color("button")
                        window: themeManager.color("window")
                        text: themeManager.color("text")
                    }
                    
                    onCurrentTextChanged: {
                        if (currentText !== "") {
                            modelStoreDialog.currentSource = currentText
                            modelStoreDialog.fetchModels()
                        }
                    }
                    
                    Component.onCompleted: {
                        // Set initial selection
                        currentIndex = find(modelStoreDialog.currentSource)
                    }
                }
            }

            // Sort Order
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Sort By"
                    color: themeManager.color("text")
                    font.bold: true
                }
                ComboBox {
                    id: sortCombo
                    model: ["Trending", "Likes", "Date"]
                    Layout.preferredWidth: 140
                    palette {
                        buttonText: themeManager.color("buttonText")
                        button: themeManager.color("button")
                        window: themeManager.color("window")
                        text: themeManager.color("text")
                    }
                    onCurrentTextChanged: modelStoreDialog.setSort(currentText)
                }
            }
            
            // Size Filter
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Size Filter"
                    color: themeManager.color("text")
                    font.bold: true
                }
                ComboBox {
                    id: sizeFilter
                    model: ["All", "2B", "4B", "8B", "20B"]
                    Layout.preferredWidth: 100
                    palette {
                        buttonText: themeManager.color("buttonText")
                        button: themeManager.color("button")
                        window: themeManager.color("window")
                        text: themeManager.color("text")
                    }
                    onCurrentTextChanged: modelStoreDialog.setSizeFilter(currentText)
                }
            }

            // Name Filter
            ColumnLayout {
                spacing: 5
                Label { 
                    text: "Name Filter"
                    color: themeManager.color("text")
                    font.bold: true
                }
                TextField {
                    id: mustContainsField
                    text: modelStoreDialog.searchName
                    Layout.preferredWidth: 250
                    palette {
                        text: themeManager.color("text")
                        base: themeManager.color("windowDarker")
                    }
                    onEditingFinished: modelStoreDialog.searchName = text
                }
            }            

            Item { Layout.fillWidth: true } // Spacer
            
            Button {
                text: "Refresh"
                palette {
                    buttonText: themeManager.color("buttonText")
                    button: themeManager.color("button")
                }
                onClicked: modelStoreDialog.fetchModels()
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
                    color: themeManager.color("text")
                    font.bold: true
                }
                TextField {
                    id: bearerTokenField
                    placeholderText: "hf_..."
                    text: modelStoreDialog.bearerToken
                    echoMode: TextField.Password
                    Layout.preferredWidth: 250
                    palette {
                        text: themeManager.color("text")
                        base: themeManager.color("windowDarker")
                    }
                    onEditingFinished: modelStoreDialog.bearerToken = text
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
            
            model: ListModel { id: modelsModel }

            // Fermer detailsPanel lors du défilement
            onMovementStarted: {
                currentIndex = -1
            }

            delegate: ItemDelegate {
                id: modelDelegate
                width: modelListView.width
                height: 60
                
                background: Rectangle {
                    color: modelListView.currentIndex === index ? themeManager.color("windowDarker") : (modelDelegate.hovered ? themeManager.color("windowDarker2") : "transparent")
                    border.color: modelListView.currentIndex === index ? themeManager.color("button") : "transparent"
                    border.width: 1
                }
                
                contentItem: ColumnLayout {
                    spacing: 2
                    
                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: model.name || ""
                            font.bold: true
                            font.pixelSize: 16
                            color: themeManager.color("text")
                            Layout.fillWidth: true
                        }
                        Label {
                            text: model.size ? (model.size / 1024 / 1024 / 1024).toFixed(2) + " GB" : "Unknown"
                            color: themeManager.color("text")
                            font.pixelSize: 14
                        }
                    }
                    
                    Label {
                        text: model.description || ""
                        color: themeManager.color("text")
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
                    modelStoreDialog.fetchModelDetails(model.name || model.tag)
                }                
            }                   
        }

        // Model Details Panel
        Rectangle {
            id: detailsPanel
            Layout.fillWidth: true
            Layout.preferredHeight: 250
            visible: modelListView.currentIndex >= 0
            color: themeManager.color("windowDarker")
            border.color: themeManager.color("button")
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
                        color: themeManager.color("text")
                    }
                    Item { Layout.fillWidth: true }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.trending !== "" ? "🔥" : " " 
                        font.family: themeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.trending !== "" ? detailsPanel.manifest.trending : " "
                        font.pixelSize: 14
                        color: themeManager.color("text")
                    }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.likes !== "" ? "⭐" : " "
                        font.family: themeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.likes !== "" ? detailsPanel.manifest.likes : " "
                        font.pixelSize: 14
                        color: themeManager.color("text")
                    }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.downloads !== "" ? "📥" : " "
                        font.family: themeManager.colorEmojiFont
                        font.pixelSize: 18
                    }
                    Label {
                        text: detailsPanel.manifest && detailsPanel.manifest.downloads !== "" ? detailsPanel.manifest.downloads : " "
                        font.pixelSize: 14
                        color: themeManager.color("text")
                    }
                    
                    Item { Layout.fillWidth: true } // SPACER

                    Button {
                        text: "Close"
                        Layout.alignment: Qt.AlignRight
                        palette {
                            buttonText: themeManager.color("buttonText")
                            button: themeManager.color("button")
                        }
                        onClicked: { 
                            modelListView.currentIndex = -1 
                        }
                    }                    
                }
                
                Label {
                    text: detailsPanel.manifest && detailsPanel.manifest.desc || "Loading description ..."
                    color: themeManager.color("text")
                }
                
                ComboBox {
                    id: filesCombo                    
                    model: detailsPanel.details.files
                    textRole: "name"
                    Layout.preferredWidth: 400
                    palette {
                        buttonText: themeManager.color("buttonText")
                        button: themeManager.color("button")
                        window: themeManager.color("window")
                        text: themeManager.color("text")
                    }
                    popup.height: 200
                }

                Item { Layout.fillHeight: true }
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    ProgressBar {
                        id: downloadBar
                        visible: root.isDownloading
                        value: modelStoreDialog.downloadProgress
                        Layout.fillWidth: true
                    }

                    Button {
                        text: "Download File"
                        visible: !root.isDownloading
                        palette {
                            buttonText: themeManager.color("buttonText")
                            button: themeManager.color("button")
                        }
                        onClicked: {
                            modelStoreDialog.downloadFile(detailsPanel.details.name, detailsPanel.details.files[filesCombo.currentIndex]);
                        }
                    }

                    Button {
                        text: "Download All Files"
                        visible: !root.isDownloading
                        palette {
                            buttonText: themeManager.color("buttonText")
                            button: themeManager.color("button")
                        }
                        onClicked: {
                            modelStoreDialog.downloadAllFiles(detailsPanel.details.name, detailsPanel.details.files);                            
                        }
                    }

                    Button {
                        text: "Cancel"
                        visible: root.isDownloading
                        palette {
                            buttonText: themeManager.color("buttonText")
                            button: themeManager.color("button")
                        }
                        onClicked: {
                            modelStoreDialog.cancelDownload()
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
                    color: themeManager.color("text")
                    font.italic: true
                    width: implicitWidth
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Close"
                Layout.alignment: Qt.AlignRight
                palette {
                    buttonText: themeManager.color("buttonText")
                    button: themeManager.color("button")
                }
                onClicked: root.closeRequested()
            }
        }
    }
    
    Connections {
        target: modelStoreDialog
        
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
            if (success && chatController) {
                // Rafraîchir la liste des modèles disponibles après un téléchargement réussi
                chatController.refreshModels()
            }
        }
    }
    
    // Initial fetch
    Component.onCompleted: {
        modelStoreDialog.fetchModels() // Triggered by sourceCombo initial set or manually?
        // Let sourceCombo trigger it via bindings
    }
}
