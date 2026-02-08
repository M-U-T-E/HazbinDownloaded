import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: settingsPage
    width: 400
    height: 600
    color: "#1e1e1e"

    Component.onCompleted: {
        cookiePathInput.text = settingsManager.loadCookiePath()
        denoPathInput.text = settingsManager.loadDenoPath()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Label {
            text: "Settings"
            font.pixelSize: 24
            color: "white"
        }

        TextField {
            id: cookiePathInput
            placeholderText: "Enter path to cookies.txt"
            color: "white"
            background: Rectangle {
                color: "#2d2d2d"
                border.color: "#4d4d4d"
                border.width: 1
                radius: 4
            }
        }

        TextField {
            id: denoPathInput
            placeholderText: "Enter path to deno executable"
            color: "white"
            background: Rectangle {
                color: "#2d2d2d"
                border.color: "#4d4d4d"
                border.width: 1
                radius: 4
            }
        }

        RowLayout {
            spacing: 10
            Layout.alignment: Qt.AlignLeft

            StyledButton {
                text: "Save"
                onClicked: {
                    settingsManager.saveCookiePath(cookiePathInput.text)
                    settingsManager.saveDenoPath(denoPathInput.text)
                    // Optionally, provide feedback to the user
                    console.log("Cookie path saved:", cookiePathInput.text)
                    console.log("Deno path saved:", denoPathInput.text)
                    mainStack.currentIndex = 0
                }
            }
            StyledButton {
                text: "Back"
                onClicked: {
                    mainStack.currentIndex = 0
                }
            }
        }
    }
}
