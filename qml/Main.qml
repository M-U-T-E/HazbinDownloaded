import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    minimumWidth: 880
    minimumHeight: 576
    visible: true
    color: "#1a1a1a"
    title: "HazbinDownloaded"

    property bool isConsoleVisible: false

    // OS-specific FFmpeg download URLs
    property string ffmpegWindowsDownloadUrl: "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl-shared.zip"
    property string ffmpegLinuxDownloadUrl: "https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-lgpl-shared.tar.xz"

    // OS-specific yt-dlp download URLs
    property string ytDlpWindowsDownloadUrl: "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp.exe"
    property string ytDlpLinuxDownloadUrl: "https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp_linux"

    // OS-specific deno download URLs
    property string denoWindowsDownloadUrl: "https://github.com/denoland/deno/releases/latest/download/deno-x86_64-pc-windows-msvc.zip"
    property string denoLinuxDownloadUrl: "https://github.com/denoland/deno/releases/latest/download/deno-x86_64-unknown-linux-gnu.zip"

    property bool ffmpegIsInstalled: false
    property bool ytDlpIsInstalled: false // New property to track yt-dlp installation status
    property bool denoIsInstalled: false
    property string localYtDlpVersionString: "N/A" // Stores local version for display
    property bool ytDlpUpdateIsAvailable: false // Stores update availability

    // Function to update the consolidated yt-dlp status label
    function updateYtDlpDisplayStatus() {
        if (!root.ytDlpIsInstalled) {
            footerView.ytDlpConsolidatedStatusText = "yt-dlp (Not Found)";
            footerView.ytDlpStatusColor = "#c23628"; // Red
        } else if (root.ytDlpUpdateIsAvailable) {
            footerView.ytDlpConsolidatedStatusText = "yt-dlp (Update Available)";
            footerView.ytDlpStatusColor = "#FFC107"; // Amber
        } else {
            footerView.ytDlpConsolidatedStatusText = "yt-dlp (" + root.localYtDlpVersionString + ")";
            footerView.ytDlpStatusColor = "#4CAF50"; // Green
        }
    }

    // Connections to the global toolsManager singleton
    Connections {
        target: toolsManager

        function onProcessStarted() {
            footerView.busy = true;
            footerView.progressTextColor = "#bbbbbb";
            consoleView.internalConsole.append("----- Starting Tool Update -----");
        }

        function onProgressChanged(percentage, description) {
            footerView.progressValue = percentage / 100.0;
            footerView.progressText = description;
        }

        function onProcessFinished(success) {
            if (success) {
                footerView.busy = false;
                consoleView.internalConsole.append("----- Tool Update Finished Successfully -----");
            } else {
                footerView.progressTextColor = "#c23628";
                errorDisplayTimer.start();
                consoleView.internalConsole.append("----- Tool Update Finished with Errors -----");
            }
        }

        function onYtDlpStatus(found) {
            root.ytDlpIsInstalled = found;
            root.updateYtDlpDisplayStatus();
        }

        function onDenoStatus(found) {
            root.denoIsInstalled = found;
        }

        function onLocalYtDlpVersion(version) {
            root.localYtDlpVersionString = version;
            consoleView.internalConsole.append("Local yt-dlp version: " + version);
            root.updateYtDlpDisplayStatus();
        }

        function onLatestYtDlpVersion(version) {
            consoleView.internalConsole.append("Latest yt-dlp version: " + version);
        }

        function onYtDlpUpdateAvailable(available) {
            root.ytDlpUpdateIsAvailable = available;
            if (available) {
                consoleView.internalConsole.append("yt-dlp: Update Available!");
            } else {
                consoleView.internalConsole.append("yt-dlp: Up to Date.");
            }
            root.updateYtDlpDisplayStatus();
        }
    }

    // Connections for real-time output from youtubeService
    Connections {
        target: youtubeService

        function onProcessOutput(output) {
            consoleView.internalConsole.append(output);
        }

        function onProcessError(error) {
            consoleView.internalConsole.append(error);
        }

        function onVideoInfoError(error) {
            consoleView.internalConsole.append("ERROR: " + error);
        }

        function onDownloadProgress(progress, speed, eta, totalSize) {
            // Pass these to MainContent as well for the new overlay
            mainContentView.downloadProgressValue = progress / 100.0;
            mainContentView.downloadSpeed = speed;
            mainContentView.downloadEta = eta;
            mainContentView.downloadTotalSize = totalSize;
            mainContentView.isDownloading = true;
        }

        function onDownloadStatus(status) {
            mainContentView.downloadStatus = status;
        }

        function onDownloadFinished(filePath) {
            mainContentView.isDownloading = false;
            consoleView.internalConsole.append("Download finished: " + filePath);
        }

        function onDownloadError(errorString) {
            mainContentView.isDownloading = false;
            consoleView.internalConsole.append("Download ERROR: " + errorString);
        }
    }

    Timer {
        id: errorDisplayTimer
        interval: 5000
        repeat: false
        onTriggered: {
            footerView.busy = false;
        }
    }

    Component.onCompleted: {
        toolsManager.checkToolsOnStartup();
        toolsManager.checkYtDlpVersion();
    }

    StackLayout {
        id: mainStack
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: consoleView.visible ? consoleView.top : parent.bottom
        currentIndex: 0

        MainContent {
            id: mainContentView
            onIsLoadingChanged: footerView.updateButtonEnabled = !isLoading
        }

        Loader {
            id: settingsLoader
            sourceComponent: settingsComponent
        }
    }

    Component {
        id: settingsComponent
        Settings {
            id: settingsView
        }
    }

    Console {
        id: consoleView
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 150
        visible: root.isConsoleVisible

        mapTarget: mainContentView
        footerHeight: footerView.height
        windowHeight: root.height

        onTextChanged: {
            if (footerView.autoScrollChecked) {
                internalConsole.cursorPosition = internalConsole.length;
            }
        }
    }

    footer: Footer {
        id: footerView
        consoleControlsEnabled: root.isConsoleVisible

        onShowConsoleClicked: root.isConsoleVisible = !root.isConsoleVisible
        onSettingsClicked: mainStack.currentIndex = 1
        onCopyClicked: consoleView.copy()
        onClearClicked: consoleView.clear()

        onUpdateToolsClicked: {
            var downloadInitiated = false;

            if (!root.ffmpegIsInstalled) {
                toolsManager.startDownload("ffmpeg", Qt.platform.os === "windows" ? root.ffmpegWindowsDownloadUrl : root.ffmpegLinuxDownloadUrl, Qt.platform.os === "windows" ? "zip" : "tar.xz");
                downloadInitiated = true;
            } else {
                consoleView.internalConsole.append("FFmpeg is already installed. Skipping download.");
            }

            if (root.ytDlpUpdateIsAvailable || !root.ytDlpIsInstalled) {
                toolsManager.startDownload("yt-dlp", Qt.platform.os === "windows" ? root.ytDlpWindowsDownloadUrl : root.ytDlpLinuxDownloadUrl, Qt.platform.os === "windows" ? "exe" : "bin");
                downloadInitiated = true;
            } else {
                consoleView.internalConsole.append("yt-dlp is already up to date. Skipping download.");
            }

            if (!root.denoIsInstalled) {
                toolsManager.startDownload("deno", Qt.platform.os === "windows" ? root.denoWindowsDownloadUrl : root.denoLinuxDownloadUrl, "zip");
                downloadInitiated = true;
            } else {
                consoleView.internalConsole.append("Deno is already installed. Skipping download.");
            }

            if (!downloadInitiated) {
                footerView.busy = false;
                consoleView.internalConsole.append("----- No tools needed updating. -----\n");
            }
        }
    }
}
