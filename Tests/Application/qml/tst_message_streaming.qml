import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtTest 1.2

Item {
    id: root
    width: 800
    height: 600

    // Mocking ThemeManager for the test
    QtObject {
        id: mockThemeManager
        property bool darkMode: true
        function color(name) {
            if (name === "windowDarker") return "#2a2a2a";
            if (name === "windowDarker2") return "#1a1a1a";
            if (name === "text") return "white";
            return "black";
        }
    }

    TestCase {
        name: "MessageStreaming"
        
        Component {
            id: delegateComponent
            Loader {
                source: "../../Source/Application/qml/MessageDelegate.qml"
            }
        }

        function test_streaming_display_update() {
            var loader = delegateComponent.createObject(root)
            verify(loader !== null)
            
            // Wait for loader to be ready
            while(loader.status !== Loader.Ready) {
                wait(10);
            }
            
            var delegate = loader.item
            verify(delegate !== null)
            
            // Inject mock themeManager and initial modelData
            delegate.themeManager = mockThemeManager
            delegate.messageData = { role: "assistant", content: "<think>..." }
            
            // Allow some time for bindings to settle
            wait(50)

            var textEdit = findChild(delegate, "msgText")
            verify(textEdit !== null, "Could not find msgText in MessageDelegate")
            
            compare(textEdit.text.trim(), "<think>...", "Initial content should match")

            // Simulate streaming
            
            wait(100)
            compare(textEdit.text.trim(), "<think>Thinking... Done. </think>Answer: 42", "Updated content should match")

            loader.destroy()
        }

        function findChild(parent, objectName) {
            if (parent.objectName === objectName) return parent;
            if (parent.children) {
                for (var i = 0; i < parent.children.length; i++) {
                    var child = parent.children[i];
                    var found = findChild(child, objectName);
                    if (found) return found;
                }
            }
            return null;
        }
    }
}
