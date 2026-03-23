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
#include "webapi.h"

#ifdef Q_OS_WASM
#include <emscripten/bind.h>
#include <emscripten/val.h>
#endif

#include "global/io/file.h"

#include "log.h"

using namespace muse;
using namespace mu::appjs;

#ifdef Q_OS_WASM
static void callJsWithBytes(const char* fnname, const uint8_t* data, size_t size)
{
    emscripten::val jsArray = emscripten::val::global("Uint8Array").new_(
        emscripten::typed_memory_view(size, data)
        );

    emscripten::val::module_property(fnname)(jsArray);
}

#else

static void callJsWithBytes(const char*, const uint8_t*, size_t)
{
    NOT_SUPPORTED;
}

#endif

WebApi* WebApi::instance()
{
    static WebApi a;

    return &a;
}

void WebApi::init(const muse::modularity::ContextPtr& iocCtx)
{
    setContext(iocCtx);

    auto onProjectChanged = [this]() {
        m_currentProject = globalContext()->currentProject();

        if (m_currentProject) {
            m_currentProject->saveComplited().onReceive(this, [this](const muse::io::path_t& path, project::SaveMode mode) {
                onProjectSaved(path, mode);
            });

            m_currentProject->needSave().notification.onNotify(this, [this]() {
                onNeedSaveChanged();
            });
        }
    };

    globalContext()->currentProjectChanged().onNotify(this, onProjectChanged);

    onProjectChanged();
}

void WebApi::deinit()
{
}

void WebApi::load(const void* source, unsigned int len)
{
    LOGI() << source << ", len: " << len;
    ByteArray data = ByteArray::fromRawData(reinterpret_cast<const char*>(source), len);
    io::path_t tempFilePath = "/mu/temp/current.mscz";

    //! NOTE Remove last previous
    io::File::remove(tempFilePath);

    //! NOTE Write new project
    io::File::writeFile(tempFilePath, data);

    dispatcher()->dispatch("file-open", actions::ActionData::make_arg1(QUrl::fromLocalFile(tempFilePath.toQString())));
}

void WebApi::addSoundFont(const std::string& uri)
{
    soundFontController()->addSoundFont(Uri(uri));
}

void WebApi::startAudioProcessing()
{
    startAudioController()->startAudioProcessing(IApplication::RunMode::GuiApp);
}

void WebApi::onProjectSaved(const muse::io::path_t& path, mu::project::SaveMode)
{
    IF_ASSERT_FAILED(io::File::exists(path)) {
        LOGE() << "file does not exist, path: " << path;
        return;
    }

    ByteArray data;
    Ret ret  = io::File::readFile(path, data);
    if (!ret) {
        LOGE() << "failed read file, path: " << path;
        return;
    }

    callJsWithBytes("onProjectSaved", data.constData(), data.size());
}

void WebApi::onNeedSaveChanged()
{
    if (!m_currentProject) {
        return;
    }

    bool needSave = m_currentProject->needSave().val;

#ifdef Q_OS_WASM
    emscripten::val::module_property("onNeedSave")(needSave);
#endif
}

void WebApi::serializeAsXml()
{
    if (!m_currentProject) {
        LOGE() << "No current project to serialize";
        return;
    }

    io::path_t tempPath = "/mu/temp/autosave.mscs";
    io::File::remove(tempPath);

    Ret ret = m_currentProject->save(tempPath, project::SaveMode::SaveCopy, false);
    if (!ret) {
        LOGE() << "Failed to serialize project as XML: " << ret.toString();
        return;
    }

    ByteArray data;
    ret = io::File::readFile(tempPath, data);
    io::File::remove(tempPath);

    if (!ret) {
        LOGE() << "Failed to read serialized XML";
        return;
    }

    callJsWithBytes("onProjectSerialized", data.constData(), data.size());
}
