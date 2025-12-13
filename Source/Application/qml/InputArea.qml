import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#202020"
    
    signal accepted(string text)
    
    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                id: inputField
                placeholderText: "Type a message..."
                wrapMode: Text.Wrap
                selectByMouse: true
                color: "white"
                
                // Handle Shift+Enter
                Keys.onReturnPressed: (event) => {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false
                    } else {
                        event.accepted = true
                        if (inputField.text.trim().length > 0)
                            sendBtn.clicked()
                    }
                }
            }
        }
        
        Button {
            id: sendBtn
            text: "Send"
            enabled: inputField.text.trim().length > 0
            onClicked: {
                parent.parent.accepted(inputField.text)
                inputField.clear()
            }
        }
    }
}
