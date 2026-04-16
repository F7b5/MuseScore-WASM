/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited
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
#include "startupscenario.h"

#ifdef Q_OS_WASM
#include <emscripten.h>
#include <emscripten/val.h>
#endif

#include <QTimer>

#include "log.h"

using namespace muse;
using namespace mu::appshell;

void StartupScenario::setStartupType(const std::optional<std::string>& /*type*/)
{
    NOT_IMPLEMENTED;
}

bool StartupScenario::isStartWithNewFileAsSecondaryInstance() const
{
    return false;
}

const mu::project::ProjectFile& StartupScenario::startupScoreFile() const
{
    return m_startupScoreFile;
}

void StartupScenario::setStartupScoreFile(const std::optional<project::ProjectFile>& file)
{
    m_startupScoreFile = file ? file.value() : project::ProjectFile();
}

void StartupScenario::runOnSplashScreen()
{
}

void StartupScenario::runAfterSplashScreen()
{
    interactive()->open("musescore://notation");

#ifdef Q_OS_WASM
    // Async::call queue doesn't drain on WASM singlethread, so the
    // opened()/onResolve chain never fires. Fall back to a Qt event-loop
    // timer — by the time this fires, QML has rendered and the dispatcher
    // is wired up.
    QTimer::singleShot(500, [this]() {
        if (m_startupCompleted) {
            return;
        }

        if (m_startupScoreFile.isValid()) {
            dispatcher()->dispatch("file-open", muse::actions::ActionData::make_arg2<QUrl, QString>(
                                       m_startupScoreFile.url, m_startupScoreFile.displayNameOverride));
        }

        EM_ASM({ Module["_appReady"] = true; });

        emscripten::val onAppReady = emscripten::val::module_property("onAppReady");
        if (!onAppReady.isUndefined() && !onAppReady.isNull()) {
            onAppReady();
        }

        m_startupCompleted = true;
    });
#endif
}

bool StartupScenario::startupCompleted() const
{
    return m_startupCompleted;
}
