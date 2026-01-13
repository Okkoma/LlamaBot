import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    // Fix: Utiliser une hauteur fixe basée sur le nombre d'items plutôt que contentHeight
    height: assetList.count > 0 ? 150 : 0
    visible: assetList.count > 0
    color: themeManager.color("windowDarker")
    border.color: themeManager.color("windowDarker2")
    border.width: 1
    
    Behavior on height {
        NumberAnimation { duration: 200 }
    }
    
    ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff  // Fix: Pas besoin de scroll vertical
        
        ListView {
            id: assetList
            orientation: ListView.Horizontal
            spacing: 10
            // Fix: Ajouter une vérification de null et utiliser explicitement la propriété
            model: chatController ? chatController.pendingAssets : null
            
            // Fix: Définir explicitement la hauteur des items
            implicitHeight: 120
            
            delegate: Rectangle {
                width: 120
                height: 120
                color: themeManager.color("window")
                border.color: themeManager.color("windowDarker2")
                border.width: 1
                radius: 8
                
                // Debug: Afficher les données du modèle
                /*
                Component.onCompleted: {
                    console.log("Asset delegate - index:", index, "modelData:", JSON.stringify(modelData))
                }
                */
                Column {
                    anchors.fill: parent
                    anchors.margins: 5
                    
                    // Image preview
                    Rectangle {
                        width: parent.width
                        height: parent.height - 30
                        color: "transparent"
                        clip: true
                        
                        Image {
                            id: assetImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            // Fix: Accéder explicitement aux propriétés du modelData
                            source: {
                                if (modelData && modelData.base64) {
                                    return modelData.base64
                                }
                                return ""
                            }
                            asynchronous: false
                            
                            // Debug
                            onStatusChanged: {
                                if (status === Image.Error) {
                                    console.log("Image error for asset:", modelData ? modelData.name : "unknown")
                                }
                            }
                        }
                    }
                    
                    // Nom du fichier et bouton supprimer
                    RowLayout {
                        width: parent.width
                        height: 25
                        
                        Text {
                            Layout.fillWidth: true
                            text: (modelData && modelData.name) ? modelData.name : "Image"
                            elide: Text.ElideMiddle
                            font.pixelSize: 10
                            color: themeManager.color("text")
                        }
                        
                        Button {
                            flat: true
                            width: 14
                            height: 14
                            font.family: themeManager.colorEmojiFont
                            text: "❌"
                            font.pixelSize: 14
                            palette {
                                buttonText: themeManager.color("text")
                                button: "transparent"
                            }
                            onClicked: {
                                if (chatController) {
                                    chatController.removeAsset(index)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Debug: Afficher le nombre d'assets
    Text {
        anchors.centerIn: parent
        text: "Assets: " + (chatController ? chatController.pendingAssets.length : 0)
        color: "red"
        visible: false  // Mettre à true pour déboguer
    }
    
    Connections {
        target: chatController
        function onPendingAssetsChanged() {
            console.log("pendingAssetsChanged - count:", chatController ? chatController.pendingAssets.length : 0)
        }
    }
    
    Connections {
        target: themeManager
        function onDarkModeChanged() {
            root.color = themeManager.color("windowDarker")
            root.border.color = themeManager.color("windowDarker2")
        }
    }
}