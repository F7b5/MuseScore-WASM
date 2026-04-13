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
#include <emscripten/val.h>
#endif

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
    interactive()->open("musescore://notation").onResolve(this, [this](const Val&) {
        if (m_startupScoreFile.isValid()) {
            dispatcher()->dispatch("file-open", muse::actions::ActionData::make_arg2<QUrl, QString>(
                                       m_startupScoreFile.url, m_startupScoreFile.displayNameOverride));
        }

        // Notify the JS bridge that the notation page is open and the
        // dispatcher is ready. The JS layer (createMuApi) decides whether
        // to load a provided score or open the new score dialog.
        //
        // We always set Module._appReady = true so a JS handler installed
        // after this point can detect the missed signal and run itself.
        // NOTE: use EM_ASM / module_property — NOT emscripten::val::global("Module").
        // In a MODULARIZE build global("Module") is the factory function, not the
        // running instance. module_property() and EM_ASM's Module both target the
        // actual instance.
#ifdef Q_OS_WASM
        LOGI() << "[startupscenario] notation page open — firing onAppReady";
        EM_ASM({ Module['_appReady'] = true; });

        emscripten::val onAppReady = emscripten::val::module_property("onAppReady");
        if (!onAppReady.isUndefined() && !onAppReady.isNull()) {
            LOGI() << "[startupscenario] onAppReady callback is set — calling it";
            onAppReady();
        } else {
            LOGW() << "[startupscenario] onAppReady not set on Module — JS will poll _appReady";
        }
#endif

        m_startupCompleted = true;
    });
}

bool StartupScenario::startupCompleted() const
{
    return m_startupCompleted;
}
