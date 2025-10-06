import QtQuick
import QtQuick.Controls

Button {
    id: control
    hoverEnabled: true
    flat: true

    leftPadding: 10
    rightPadding: 10
    topPadding: 8
    bottomPadding: 8

    font.pixelSize: 12

    property color foregroundColor: "white"
    property color checkedForegroundColor: "white"

    // Use a basic Text item instead of a Label. 
    // A Text item is not affected by complex styling and will render the exact color.
    contentItem: Text {
        text: control.text
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: control.checked ? control.checkedForegroundColor : control.foregroundColor
    }

    background: Rectangle {
        radius: 3
        color: {
            if (control.pressed) {
                "#992211"
            } else if (control.checked) {
                "#b03020"
            } else if (control.hovered) {
                "#c23628"
            } else {
                "#2a2a2a"
            }
        }
    }
}
