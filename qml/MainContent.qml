import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QtQuick.Controls.Material

Rectangle {
    id: root
    color: "#1c1c1c"

    property bool showStatus: false
    property bool isValid: false
    property string contentType: ""
    property string videoUrl: ""
    property bool isLoading: loadingOverlay.visible

    property var selectedVideoFormat: null
    property var selectedAudioFormat: null
    property real totalSizeInBytes: (selectedVideoFormat ? parseSize(selectedVideoFormat.filesize) : 0) + (selectedAudioFormat ? parseSize(selectedAudioFormat.filesize) : 0)

    ListModel { id: videoFormatsModel }
    ListModel { id: audioFormatsModel }

    function parseSize(sizeString) {
        if (!sizeString || sizeString === "N/A") return 0;
        var parts = sizeString.match(/([\d\.]+)\s*(\w+)/);
        if (!parts) return 0;
        var size = parseFloat(parts[1]);
        var unit = parts[2].toUpperCase();
        if (unit === "KIB" || unit === "KB") return size * 1024;
        if (unit === "MIB" || unit === "MB") return size * 1024 * 1024;
        if (unit === "GIB" || unit === "GB") return size * 1024 * 1024 * 1024;
        if (unit === "TIB" || unit === "TB") return size * 1024 * 1024 * 1024 * 1024;
        if (unit === "B") return size;
        return 0;
    }

    function formatSize(bytes) {
        if (bytes <= 0) return "0 B";
        var k = 1024;
        var sizes = ['B', 'KiB', 'MiB', 'GiB', 'TiB'];
        var i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    function validateYouTubeUrl(url) {
        if (!url) return { isValid: false, type: "" };
        var videoRegex = /^(?:https?:\/\/)?(?:www\.|m\.)?(?:youtube\.com\/watch\?v=|youtu\.be\/)([\w-]{11})/;
        var match = videoRegex.exec(url);
        if (match) return { isValid: true, type: "video" };

        var shortRegex = /^(?:https?:\/\/)?(?:www\.|m\.)?youtube\.com\/shorts\/([\w-]{11})/;
        match = shortRegex.exec(url);
        if (match) return { isValid: true, type: "short" };

        var playlistRegex = /^(?:https?:\/\/)?(?:www\.|m\.)?youtube\.com\/playlist\?list=([\w-]{13,})/;
        match = playlistRegex.exec(url);
        if (match) return { isValid: true, type: "playlist" };

        var musicRegex = /^(?:https?:\/\/)?music\.youtube\.com\/(?:watch\?v=|playlist\?list=)([\w-]+)/;
        match = musicRegex.exec(url);
        if (match) return { isValid: true, type: "youtube music" };

        return { isValid: false, type: "" };
    }

    Connections {
        target: youtubeService

        function onVideoInfoReady(info) {
            loadingOverlay.visible = false;
            root.videoUrl = info.webpage_url;
            thumbnailImage.source = info.thumbnail
            titleText.text = info.title
            channelText.text = "<b>Channel:</b> " + info.channel
            durationText.text = "<b>Duration:</b> " + info.duration
            artistText.text = "<b>Artist:</b> " + info.artist
            albumText.text = "<b>Album:</b> " + info.album

            videoFormatsModel.clear()
            audioFormatsModel.clear()

            var videoFormats = []
            var audioFormats = []

            for (var i = 0; i < info.formats.length; i++) {
                const format = info.formats[i];
                if (format.filesize === "N/A") continue;
                if (format.vcodec !== "none") {
                    videoFormats.push(format);
                } else if (format.acodec !== "none") {
                    audioFormats.push(format);
                }
            }

            audioFormats.sort((a, b) => b.abr - a.abr);

            for (var i = 0; i < videoFormats.length; i++) {
                videoFormats[i].checked = false;
                videoFormatsModel.append(videoFormats[i]);
            }
            for (var i = 0; i < audioFormats.length; i++) {
                audioFormats[i].checked = false;
                audioFormatsModel.append(audioFormats[i]);
            }

            videoInfoDisplay.visible = true

            root.selectedVideoFormat = null;
            root.selectedAudioFormat = null;
        }

        function onVideoInfoError(error) {
            loadingOverlay.visible = false;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 15

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            ColumnLayout {
                spacing: 5
                Layout.alignment: Qt.AlignTop
                TextField {
                    id: linkInput
                    Layout.preferredWidth: 400
                    Layout.preferredHeight: 32
                    placeholderText: "Enter link"
                    Material.accent: focus ? "#c23628" : "#555555"
                    Material.theme: Material.Dark
                    font.pixelSize: 13
                    color: "white"
                    onTextChanged: {
                        showStatus = text.length > 0;
                        var validationResult = validateYouTubeUrl(text);
                        isValid = validationResult.isValid;
                        contentType = validationResult.type;
                        videoInfoDisplay.visible = false;
                        videoFormatsModel.clear();
                        audioFormatsModel.clear();
                    }
                }
                RowLayout {
                    visible: showStatus
                    spacing: 5
                    Image {
                        source: isValid ? "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24'><path fill='%234CAF50' d='M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z'/></svg>" : "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' width='14' height='14' viewBox='0 0 24 24'><path fill='%23c23628' d='M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 13.41 17.59 19 19 17.59 13.41 12z'/></svg>"
                    }
                    Label {
                        text: isValid ? "Type: " + contentType : "Invalid link!"
                        color: isValid ? "#4CAF50" : "#c23628"
                        font.pixelSize: 11
                    }
                }
            }
            StyledButton {
                text: "GET"
                font.pixelSize: 14
                Layout.preferredHeight: 46
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: -7
                enabled: isValid && !isLoading
                onClicked: {
                    loadingOverlay.visible = true;
                    videoInfoDisplay.visible = false;
                    youtubeService.fetchVideoInfo(linkInput.text);
                }
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            id: loadingOverlay
            visible: false
            color: "#2a2a2a"
            radius: 5
            Layout.fillWidth: true
            Layout.fillHeight: true
            z: 10

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 15
                BusyIndicator {
                    running: true
                    Layout.alignment: Qt.AlignHCenter
					Material.accent: "#c23628"
                }
                Text {
                    text: "Fetching video info..."
                    color: "white"
                    font.pixelSize: 16
                }
            }
        }

        Rectangle {
            id: videoInfoDisplay
            visible: false
            color: "#2a2a2a"
            radius: 5
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                RowLayout {
                    spacing: 10
                    Image {
                        id: thumbnailImage
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 90
                        fillMode: Image.PreserveAspectCrop
                        clip: true
                    }
                    ColumnLayout {
                        spacing: 5
                        Layout.fillWidth: true
                        Text {
                            id: titleText
                            font.bold: true
                            font.pixelSize: 16
                            color: "white"
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true // Ensures the text is constrained to wrap
                        }
                        Text {
                            id: channelText
                            font.pixelSize: 12
                            color: "#bbbbbb"
                            wrapMode: Text.WordWrap
                            visible: contentType !== "youtube music"
                        }
                        Text {
                            id: durationText
                            font.pixelSize: 12
                            color: "#bbbbbb"
                        }
                        Text {
                            id: artistText
                            font.pixelSize: 12
                            color: "#bbbbbb"
                            visible: contentType === "youtube music"
                        }
                        Text {
                            id: albumText
                            font.pixelSize: 12
                            color: "#bbbbbb"
                            visible: contentType === "youtube music"
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ColumnLayout {
                        id: videoFormatsColumn
                        visible: contentType !== "youtube music"
                        Layout.fillWidth: true
                        Text { text: "Video Formats"; color: "#cccccc"; font.bold: true; font.pixelSize: 14 }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Quality"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 70; padding: 8 }
                            Text { text: "Size"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 70; padding: 8 }
                            Text { text: "Codecs"; color: "#cccccc"; font.bold: true; Layout.fillWidth: true; padding: 8 }
                            Text { text: "FPS"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 40; padding: 8 }
                            Text { text: "Ext"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 50; padding: 8 }
                        }
                        ScrollView {
                            id: videoScrollView
                            Layout.fillWidth: true
                            Layout.preferredHeight: 200
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                            ListView {
                                id: videoFormatsView
                                width: parent.width
                                height: contentHeight
                                model: videoFormatsModel
                                delegate: Rectangle {
                                    width: videoFormatsView.width
                                    height: 40
                                    color: model.checked ? "#c23628" : (index % 2 === 0 ? "#2a2a2a" : "#333333")
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var isCurrentlyChecked = videoFormatsModel.get(index).checked;

                                            for (var i = 0; i < videoFormatsModel.count; i++) {
                                                if (i !== index) {
                                                    videoFormatsModel.setProperty(i, "checked", false);
                                                }
                                            }
                                            videoFormatsModel.setProperty(index, "checked", !isCurrentlyChecked);

                                            if (!isCurrentlyChecked) {
                                                root.selectedVideoFormat = videoFormatsModel.get(index);
                                            } else {
                                                root.selectedVideoFormat = null;
                                            }
                                        }
                                    }
                                    RowLayout {
                                        anchors.fill: parent
                                        Text { text: model.quality; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 70; padding: 8 }
                                        Text { text: model.filesize; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 70; padding: 8 }
                                        Text { text: model.vcodec + ", " + model.acodec; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.fillWidth: true; padding: 8 }
                                        Text { text: model.fps; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 40; padding: 8 }
                                        Text { text: model.ext; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 50; padding: 8 }
                                    }
                                }
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: "Audio Formats"; color: "#cccccc"; font.bold: true; font.pixelSize: 14 }
                        RowLayout {
                            Layout.fillWidth: true
                            Text { text: "Quality"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 80; padding: 8 }
                            Text { text: "Size"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 70; padding: 8 }
                            Text { text: "Codec"; color: "#cccccc"; font.bold: true; Layout.fillWidth: true; padding: 8 }
                            Text { text: "Ext"; color: "#cccccc"; font.bold: true; Layout.preferredWidth: 50; padding: 8 }
                        }
                        ScrollView {
                            id: audioScrollView
                            Layout.fillWidth: true
                            Layout.preferredHeight: 200
                            clip: true
                            ScrollBar.vertical.policy: ScrollBar.AlwaysOn

                            ListView {
                                id: audioFormatsView
                                width: parent.width
                                height: contentHeight
                                model: audioFormatsModel
                                delegate: Rectangle {
                                    width: audioFormatsView.width
                                    height: 40
                                    color: model.checked ? "#c23628" : (index % 2 === 0 ? "#2a2a2a" : "#333333")
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            var isCurrentlyChecked = audioFormatsModel.get(index).checked;

                                            for (var i = 0; i < audioFormatsModel.count; i++) {
                                                if (i !== index) {
                                                    audioFormatsModel.setProperty(i, "checked", false);
                                                }
                                            }
                                            audioFormatsModel.setProperty(index, "checked", !isCurrentlyChecked);

                                            if (!isCurrentlyChecked) {
                                                root.selectedAudioFormat = audioFormatsModel.get(index);
                                            } else {
                                                root.selectedAudioFormat = null;
                                            }
                                        }
                                    }
                                    RowLayout {
                                        anchors.fill: parent
                                        Text { text: Math.round(model.abr) + " kbps"; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 80; padding: 8 }
                                        Text { text: model.filesize; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 70; padding: 8 }
                                        Text { text: model.acodec; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.fillWidth: true; padding: 8 }
                                        Text { text: model.ext; color: "white"; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter; Layout.preferredWidth: 50; padding: 8 }
                                    }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    color: "#333333"
                    radius: 5
                    visible: root.selectedVideoFormat || root.selectedAudioFormat

                    RowLayout {
                        anchors.fill: parent
						anchors.leftMargin:10
						anchors.rightMargin:10

                        ColumnLayout {
                            Layout.fillWidth: true
							Layout.alignment: Qt.AlignVCenter
                            Text {
                                text: "<b>Selected Video:</b> " + (root.selectedVideoFormat ? root.selectedVideoFormat.quality + " (" + root.selectedVideoFormat.filesize + ")" : "<i>None</i>")
                                color: "white"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                visible: contentType !== "youtube music"
                            }
                            Text {
                                text: "<b>Selected Audio:</b> " + (root.selectedAudioFormat ? Math.round(root.selectedAudioFormat.abr) + " kbps" + " (" + root.selectedAudioFormat.filesize + ")" : "<i>None</i>")
                                color: "white"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                            }

                        }
						Item{ Layout.fillWidth:true }
						ColumnLayout {
							Layout.fillWidth: true
							Layout.alignment: Qt.AlignVCenter
							StyledButton {
								id: downloadbtn
								text: "DOWNLOAD"
								font.pixelSize: 14
								Layout.preferredWidth: 120
								Layout.preferredHeight: 40
								Layout.alignment: Qt.AlignRight

								background: Rectangle {
									color: {
										if (downloadbtn.pressed) {
											"#992211"
										} else if (downloadbtn.checked) {
											"#b03020"
										} else if (downloadbtn.hovered) {
											"#2a2a2a"
										} else {
											"#c23628"
										}
									}
								}
								enabled: root.selectedVideoFormat || root.selectedAudioFormat
								onClicked: {
									if (root.selectedVideoFormat || root.selectedAudioFormat) {
										youtubeService.download(root.videoUrl, root.selectedVideoFormat || {}, root.selectedAudioFormat || {});
									}
								}
							}
							Text {
								text: "<b>Estimated Size:</b> " + formatSize(totalSizeInBytes)
								Layout.alignment: Qt.AlignLeft
								color: "white"
								font.pixelSize: 13
								font.bold: true
							}
						}
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }
    }
}
