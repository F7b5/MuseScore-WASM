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
import Muse.Dock

import MuseScore.AppShell 1.0

AppWindow {
    id: root

    flags: Qt.FramelessWindowHint

    function revealWindow() {
        root.opacity = 1.0
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        AppMenuBar {
            id: appMenuBar
            Layout.fillWidth: true
            appWindow: root
        }

        DockWindow {
            id: dockWindow
            Layout.fillWidth: true
            Layout.fillHeight: true

            onPageLoaded: {
                interactiveProvider.onPageOpened()
                root.revealWindow()
            }

            InteractiveProvider {
                id: interactiveProvider
                topParent: root

                onRequestedDockPage: function(uri, params) {
                    dockWindow.loadPage(uri, params)
                }
            }

            NavigationSection {
                id: topToolbarKeyNavSec
                name: "TopTool"
                order: 1
            }

            pages: [
                NotationPage {
                    topToolbarKeyNavSec: topToolbarKeyNavSec
                }
            ]
        }
    }

    Component.onCompleted: {
        dockWindow.init()
        Qt.callLater(function() {
            api.launcher.open("musescore://notation")
        })
    }
}
