import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


Rectangle {
    id: root
    width: 600
    height: 800
    color: "#212121" // Dark theme background

    signal closeRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        Label {
            text: "Ollama models Store"
            font.pixelSize: 24
            font.bold: true
            color: "white"
            Layout.alignment: Qt.AlignHCenter
        }

        TextField {
            id: searchField
            placeholderText: "Model Name (e.g. llama3, phi3:medium)"
            Layout.fillWidth: true
            color: "white"
            background: Rectangle { color: "#424242"; radius: 5 }
            onAccepted: fetchBtn.clicked()
        }

        Button {
            id: fetchBtn
            text: "Fetch Manifest"
            Layout.fillWidth: true
            onClicked: {
                if (searchField.text === "") return
                statusLabel.text = "Fetching manifest for " + searchField.text + "..."
                ollamaModelStoreDialog.fetchManifest(searchField.text, function(success, manifest, error) {
                    if (success) {
                        statusLabel.text = "Found! Size: " + (manifest.layers[0].size / 1024 / 1024).toFixed(2) + " MB"
                        downloadBtn.enabled = true
                        downloadBtn.digest = manifest.layers[0].digest
                        downloadBtn.modelName = searchField.text
                    } else {
                        statusLabel.text = "Error: " + error
                        downloadBtn.enabled = false
                    }
                })
            }
        }

        // Dynamic List of Models
        ListModel {
            id: modelsModel
        }
        
        Component.onCompleted: {
            statusLabel.text = "Loading library..."
            ollamaModelStoreDialog.fetchLibraryQml()
        }

        ListView {
            id: modelListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: modelsModel
            clip: true
            spacing: 5
            delegate: ItemDelegate {
                width: modelListView.width
                text: model.name + " (" + (model.digest ? model.digest.substring(0, 7) : "Select to fetch") + ")"
                onClicked: {
                    searchField.text = model.name
                    fetchBtn.clicked()
                }
            }
        }

        Label {
            id: statusLabel
            text: "Ready to search."
            color: "#cccccc"
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        ProgressBar {
            id: downloadBar
            visible: false
            value: 0
            Layout.fillWidth: true
        }

        Button {
            id: downloadBtn
            text: "Download Model"
            enabled: false
            property string digest: ""
            property string modelName: ""
            Layout.fillWidth: true
            onClicked: {
                if (digest === "") return
                // Determine save path: AppData Location
                // Note: QML cannot easily get StandardPaths without C++ helper or context property
                // We will assume 'QStandardPaths::AppDataLocation' is passed as base path or handled in C++
                // actually, for this demo let's let C++ decide the path or pass a temp name
                
                downloadBar.value = 0
                downloadBar.visible = true
                statusLabel.text = "Downloading..."
                
                // We'll construct a filename logic in C++ on the wrapper or here if we had the path.
                // Simpler: Just trigger downloadLayer on client and let it decide or pass a fixed temp path.
                // Wait, OllamaModelStoreDialog::downloadLayer takes 'savePath'.
                // We need a helper to get the generic app data path.
                
                // For now, let's call a wrapper method we should add to Application or Client
                // OR: expose a helper.
                // Let's assume we pass the file path from QML? No, C++ is safer.
                // Let's modify OllamaModelStoreDialog to have a convenience 'downloadModel(name, digest)' that handles path.
                
                // Use the new high-level C++ method that handles persistent storage path
                ollamaModelStoreDialog.downloadModel(modelName, digest)
            }
        }
        
        Item { Layout.fillHeight: true } // Spacer
        
        Button {
            text: "Close"
            Layout.fillWidth: true
            onClicked: root.closeRequested()
        }
    }
    
    Connections {
        target: ollamaModelStoreDialog
        function onLibraryFetched(models) {
            modelsModel.clear()
            for (var i = 0; i < models.length; i++) {
                modelsModel.append(models[i])
            }
            statusLabel.text = "Library loaded (" + models.length + " models found)."
        }
        function onLibraryFetchError(error) {
            statusLabel.text = "Error loading library: " + error
        }
        function onDownloadProgress(received, total) {
            downloadBar.value = received / total
            statusLabel.text = "Downloading: " + (received/1024/1024).toFixed(1) + " / " + (total/1024/1024).toFixed(1) + " MB"
        }
        function onDownloadFinished(success, message) {
            statusLabel.text = success ? "Download Complete! Saved to: " + message : "Failed: " + message
            downloadBar.visible = false
        }
    }
}
