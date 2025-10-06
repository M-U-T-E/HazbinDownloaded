import QtQuick
import QtQuick.Controls

Item {
    id: root

    function copy() { textArea.copy() }
    function clear() { textArea.clear() }

    signal textChanged()

    property Item mapTarget: parent
    property int footerHeight: 0
    property int windowHeight: 0

    property alias internalConsole: textArea

    // The resize handle now sits at the top of the console area
    Rectangle {
        id: resizeHandle
        width: parent.width
        height: 9
        color: "#2a2a2a"
        anchors.top: parent.top

        Row {
            anchors.centerIn: parent
            spacing: 3
            Rectangle { width: 4; height: 4; color: "#777777"; radius: 2 }
            Rectangle { width: 4; height: 4; color: "#777777"; radius: 2 }
            Rectangle { width: 4; height: 4; color: "#777777"; radius: 2 }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            property var startDragY: 0
            property var startHeight: 0

            onPressed: function(mouse) {
                startDragY = mouseArea.mapToItem(root.mapTarget, 0, mouse.y).y;
                startHeight = root.height;
            }

            onPositionChanged: function(mouse) {
                if (pressed) {
                    var currentY = mouseArea.mapToItem(root.mapTarget, 0, mouse.y).y;
                    var delta = startDragY - currentY;
                    var newHeight = startHeight + delta;

                    if (newHeight > 40 && newHeight < (root.windowHeight - root.footerHeight - 50)) {
                        root.height = newHeight;
                    }
                }
            }
        }
    }

    // The TextArea is now wrapped in a ScrollView
    ScrollView {
        anchors.top: resizeHandle.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AlwaysOn // Keep this to ensure scrollbar is visible

        background: Rectangle {
            color: "#151515"

        }

        TextArea {
            id: textArea
            readOnly: true
            wrapMode: Text.Wrap

			topPadding: 7
            leftPadding: 10
            rightPadding: 0
            font.pixelSize: 12
            selectionColor: "#992211"
            color: "white"
            text: "Log started...\n"

            // The TextArea background must be transparent inside a ScrollView
            background: null

            onTextChanged: root.textChanged()
        }
    }
}
