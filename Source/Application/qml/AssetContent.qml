pragma ComponentBehavior: Bound

// qmllint disable import
import QtQuick
// qmllint enable import
import QtQuick.Controls
import QtQuick.Layouts

import LlamaBotQml

Rectangle {
    id: root
    
    // Fix: Utiliser une hauteur fixe basée sur le nombre d'items plutôt que contentHeight
    height: assetList.count > 0 ? 150 : 0
    visible: assetList.count > 0
    color: ThemeManager.windowDarkerColor
    border.color: ThemeManager.windowDarker2Color
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
            model: ChatController.pendingAssets
            
            // Fix: Définir explicitement la hauteur des items
            implicitHeight: 120
            
            delegate: Rectangle {
                id: assetDelegate

                required property var index
                property var assetData: assetList.model[index]

                width: 120
                height: 120
                color: ThemeManager.windowColor
                border.color: ThemeManager.windowDarker2Color
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
                        color: 'transparent'
                        clip: true
                        
                        Image {
                            id: assetImage
                            anchors.fill: parent
                            fillMode: Image.PreserveAspectFit
                            // Fix: Accéder explicitement aux propriétés du modelData
                            source: {
                                if (assetDelegate.assetData && assetDelegate.assetData.base64) {
                                    return assetDelegate.assetData.base64
                                }
                                return ChatController.getMimeTypeIconFor(assetDelegate.assetData.name);
                            }
                            asynchronous: false
                            
                            // Debug
                            onStatusChanged: {
                                if (status === Image.Error) {
                                    console.log("Image error for asset:", assetDelegate.assetData ? assetDelegate.assetData.name : "unknown")
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
                            text: (assetDelegate.assetData && assetDelegate.assetData.name) ? assetDelegate.assetData.name : "Image"
                            elide: Text.ElideMiddle
                            font.pixelSize: 10
                            color: ThemeManager.textColor
                        }
                        
                        Button {
                            flat: true
                            width: 14
                            height: 14
                            font.family: ThemeManager.colorEmojiFont
                            text: "❌"
                            font.pixelSize: 14
                            palette {
                                buttonText: ThemeManager.textColor
                                button: 'transparent'
                            }
                            onClicked: {
                                if (ChatController) {
                                    ChatController.removeAsset(assetDelegate.index)
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
        text: "Assets: " + ChatController.pendingAssets.length
        color: "red"
        visible: false  // Mettre à true pour déboguer
    }
    
    Connections {
        target: ChatController
        function onPendingAssetsChanged() {
            console.log("pendingAssetsChanged - count:", ChatController.pendingAssets.length)
        }
    }
}