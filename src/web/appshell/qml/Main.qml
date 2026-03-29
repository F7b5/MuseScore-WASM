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
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui 1.0
import Muse.UiComponents
import Muse.Interactive

import MuseScore.AppShell 1.0
import MuseScore.Playback 1.0

AppWindow {
    id: root

    flags: Qt.FramelessWindowHint

    function revealWindow() {
        root.opacity = 1.0
    }

    InteractiveProvider {
        id: interactiveProvider
        topParent: root

        onRequestedDockPage: function(uri, params) {
            root.revealWindow()
            Qt.callLater(interactiveProvider.onPageOpened)
        }
    }

    Component.onCompleted: {
        Qt.callLater(root.revealWindow)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            id: topChrome
            Layout.fillWidth: true
            Layout.preferredHeight: Math.max(appMenuBar.implicitHeight, playbackToolBar.implicitHeight) + 12

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 6
                anchors.bottomMargin: 6
                spacing: 16

                AppMenuBar {
                    id: appMenuBar

                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter

                    appWindow: root
                    availableWidth: Math.max(0, topChrome.width - playbackToolBar.implicitWidth - 32)

                    Component.onCompleted: {
                        console.info("WebMain: app menu bar created, availableWidth=", availableWidth)
                    }
                }

                PlaybackToolBar {
                    id: playbackToolBar

                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
            }
        }

        NotationFrame {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
