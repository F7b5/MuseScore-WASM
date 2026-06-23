/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import MuseScore.AppShell 1.0

StyledDialogView {
    id: root

    title: "Preferences"

    contentWidth: 450
    contentHeight: 350

    MidiPreferencesModel {
        id: prefsModel
    }

    Component.onCompleted: {
        prefsModel.load()
    }

    Rectangle {
        anchors.fill: parent
        color: ui.theme.backgroundPrimaryColor

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            StyledTextLabel {
                text: "MIDI Configuration"
                font: ui.theme.headerBoldFont
            }

            // MIDI Input
            ColumnLayout {
                spacing: 8
                Layout.fillWidth: true

                StyledTextLabel {
                    text: "MIDI Input Device"
                    font: ui.theme.bodyBoldFont
                }

                StyledDropdown {
                    id: inputDropdown
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32

                    model: {
                        var items = []
                        var devices = prefsModel.inputDevices
                        for (var i = 0; i < devices.length; i++) {
                            items.push({ text: devices[i].name, value: devices[i].id })
                        }
                        return items
                    }

                    currentIndex: {
                        var currentId = prefsModel.currentInputDeviceId
                        var devices = prefsModel.inputDevices
                        for (var i = 0; i < devices.length; i++) {
                            if (devices[i].id === currentId) return i
                        }
                        return 0
                    }

                    onActivated: function(index, value) {
                        prefsModel.setInputDevice(value)
                    }
                }
            }

            // MIDI Output
            ColumnLayout {
                spacing: 8
                Layout.fillWidth: true

                StyledTextLabel {
                    text: "MIDI Output Device"
                    font: ui.theme.bodyBoldFont
                }

                StyledDropdown {
                    id: outputDropdown
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32

                    model: {
                        var items = []
                        var devices = prefsModel.outputDevices
                        for (var i = 0; i < devices.length; i++) {
                            items.push({ text: devices[i].name, value: devices[i].id })
                        }
                        return items
                    }

                    currentIndex: {
                        var currentId = prefsModel.currentOutputDeviceId
                        var devices = prefsModel.outputDevices
                        for (var i = 0; i < devices.length; i++) {
                            if (devices[i].id === currentId) return i
                        }
                        return 0
                    }

                    onActivated: function(index, value) {
                        prefsModel.setOutputDevice(value)
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 12

                FlatButton {
                    text: "Close"
                    onClicked: root.hide()
                }
            }
        }
    }
}
