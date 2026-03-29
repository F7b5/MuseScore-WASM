# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

muse_create_module(appshell)

target_sources(appshell PRIVATE
    appshellmodule.cpp
    appshellmodule.h
    iappshellconfiguration.h
    appshelltypes.h

    internal/applicationuiactions.cpp
    internal/applicationuiactions.h
    internal/appshellconfiguration.cpp
    internal/appshellconfiguration.h
    internal/applicationactioncontroller.cpp
    internal/applicationactioncontroller.h
    internal/istartupscenario.h
    internal/startupscenario.cpp
    internal/startupscenario.h

    view/appmenumodel.cpp
    view/appmenumodel.h
    view/notationpagemodel.cpp
    view/notationpagemodel.h
    view/notationstatusbarmodel.cpp
    view/notationstatusbarmodel.h
    view/navigableappmenumodel.cpp
    view/navigableappmenumodel.h
)

muse_module_add_qrc(appshell ${CMAKE_CURRENT_LIST_DIR}/appshell.qrc)

target_link_libraries(appshell PRIVATE
    muse::ui
    muse::uicomponents
    muse::dockwindow
    Qt::Quick
)
