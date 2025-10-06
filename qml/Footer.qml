import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import HazbinDownloaded 1.0
import QtQuick.Controls.Material

Pane {
    id: root

    implicitHeight: 35

    signal showConsoleClicked()
    signal copyClicked()
    signal clearClicked()
    signal updateToolsClicked()

    property alias autoScrollChecked: autoScrollButton.checked
    property bool consoleControlsEnabled: false

    property bool busy: false
    property alias progressValue: progressBar.value
    property alias progressText: progressLabel.text
    property color progressTextColor: "#bbbbbb"

    // Consolidated yt-dlp status properties
    property alias ytDlpConsolidatedStatusText: ytDlpStatusLabel.text
    property alias ytDlpStatusColor: ytDlpStatusLabel.color // Keep color alias for the consolidated label

    onBusyChanged: {
        console.log("Footer.qml: root.busy changed to " + root.busy + ", setting stackLayout.currentIndex to " + (root.busy ? 1 : 0));
    }

    //padding: 5
    background: Rectangle {
        color: "transparent"
    }

    Item {
        anchors.fill: parent

        Row {
            id: leftRow
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            StyledButton {
                text: "Show console"
                onClicked: root.showConsoleClicked()
            }
            StyledButton {
                text: "Copy"
                visible: root.consoleControlsEnabled
                onClicked: root.copyClicked()
            }
            StyledButton {
                text: "Clear"
                visible: root.consoleControlsEnabled
                onClicked: root.clearClicked()
            }
            StyledButton {
                id: autoScrollButton
                text: "Auto scroll"
                checkable: true
                checked: true
                checkedForegroundColor: "black"
                visible: root.consoleControlsEnabled
            }
        }

        StackLayout {
            id: stackLayout
            anchors.left: leftRow.right
            anchors.right: parent.right
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            currentIndex: root.busy ? 1 : 0 // Direct binding to busy property

            height: childrenRect.height // Ensure StackLayout takes height of its children

            RowLayout {
                spacing: 8 // Add spacing between elements

                Item { Layout.fillWidth: true } // Spacer to push content to the right

                ColumnLayout {
                    Layout.alignment: Qt.AlignVCenter // Vertically center the labels column

                    // Consolidated yt-dlp status
                    Label {
                        id: ytDlpStatusLabel
                        text: "yt-dlp (Checking...)"
                        color: "#bbbbbb"
                        font.pixelSize: 12
                    }
                }
                
                StyledButton {
                    id: updateToolsButton
                    text: "Update tools"
                    onClicked: root.updateToolsClicked()
                    Layout.alignment: Qt.AlignVCenter // Vertically center the button
                }
            }

            Column {
                Layout.preferredWidth: 220

                ProgressBar {
                    id: progressBar
                    width: parent.width
                    value: 0
                    Material.accent: "#c23628" // Set progress bar fill color to red

                }
                Label {
                    id: progressLabel
                    width: parent.width
                    text: "Starting..."
                    color: root.progressTextColor
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
