import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: size
    height: size
    property int size: 24
    visible: false 

    signal startLoading()
    signal endLoading()

    Image {
        id: spinner
        height: root.size * 0.65
        source: "qrc:/images/LoadingSpinner.png"
        fillMode: Image.PreserveAspectFit
        opacity: 0
        // Centrer l'image       
        anchors.centerIn: parent
        // Pivot pour la rotation (centre de l'image)
        transformOrigin: Image.Center
        
        // Animation de rotation
        RotationAnimation {
            id: rotationAnimation
            target: spinner
            property: "rotation"
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite
            running: false
        }
        
        // Animation d'opacité pour un effet plus fluide
        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }
    }

    // Propriété pour stocker le contrôleur et gérer les connexions
    property var controller: null
    
    // Méthodes publiques pour contrôler le spinner
    // Ces méthodes peuvent être appelées directement ou connectées à des signaux
    function show() {
        startLoading()
    }
    
    function hide() {
        endLoading()
    }
    
    // Méthode pour établir les connexions de manière sûre
    function connectToController(controller) {
        if (controller) {
            // Déconnecter les anciennes connexions si elles existent
            if (controller === this.controller) {
                return // Déjà connecté
            }
            
            // Nettoyer les anciennes connexions
            if (this.controller) {
                try {
                    this.controller.loadingStarted.disconnect(show)
                    this.controller.loadingFinished.disconnect(hide)
                } catch (e) {
                    console.warn("Error disconnecting old signals:", e)
                }
            }
            
            // Établir les nouvelles connexions
            this.controller = controller
            controller.loadingStarted.connect(show)
            controller.loadingFinished.connect(hide)
            console.log("LoadingSpinner connected to controller")
        }
    }
    
    // Nettoyage lors de la destruction
    Component.onDestruction: {
        if (controller) {
            try {
                controller.loadingStarted.disconnect(show)
                controller.loadingFinished.disconnect(hide)
            } catch (e) {
                console.warn("Error cleaning up signals:", e)
            }
            controller = null
        }
    }
    
    // Gestion des événements
    onStartLoading: {
        visible = true
        spinner.opacity = 1
        rotationAnimation.running = true
        console.log("Loading spinner started")
    }

    onEndLoading: {
        rotationAnimation.running = false
        spinner.opacity = 0
        // Attendre que l'animation d'opacité se termine avant de cacher complètement
        // Note: Nous ne pouvons pas facilement attendre la fin de l'animation Behavior,
        // donc nous cachons simplement après un court délai
        visible = false
        console.log("Loading spinner stopped")
    }
}