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
#pragma once

#include "global/async/asyncable.h"

#include "global/modularity/ioc.h"
#include "interactive/iinteractive.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "audio/main/istartaudiocontroller.h"
#include "audio/main/isoundfontcontroller.h"

namespace mu::appjs {
class WebApi : public muse::async::Asyncable, public muse::Contextable
{
    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<mu::context::IGlobalContext> globalContext = { this };
    muse::ContextInject<muse::audio::IStartAudioController> startAudioController = { this };
    muse::ContextInject<muse::audio::ISoundFontController> soundFontController = { this };

public:

    static WebApi* instance();

    void init(const muse::modularity::ContextPtr& iocCtx);
    void deinit();

    void load(const void* source, unsigned int len);
    void addSoundFont(const std::string& uri);
    void startAudioProcessing();
    void serializeAsXml();

private:

    WebApi() : muse::Contextable(muse::modularity::globalCtx) {}

    void onProjectSaved(const muse::io::path_t& path, mu::project::SaveMode mode);
    void onNeedSaveChanged();

    project::INotationProjectPtr m_currentProject;
};
}
