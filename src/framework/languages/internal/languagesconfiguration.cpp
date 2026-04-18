/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#include "languagesconfiguration.h"

#include <QStringList>
#include <QUrl>

#include "global/configreader.h"
#include "settings.h"
#include "languagestypes.h"

#include "log.h"

#ifdef Q_OS_WASM
#include <emscripten/val.h>
#endif

using namespace muse;
using namespace muse::languages;

static const Settings::Key LANGUAGE_KEY("languages", "language");

void LanguagesConfiguration::init()
{
    m_config = ConfigReader::read(":/configs/languages.cfg");

    std::string defaultLanguage = SYSTEM_LANGUAGE_CODE.toStdString();
#ifdef Q_OS_WASM
    // createMuApi resolves the locale from config.language / navigator.languages
    // in JS and publishes the final code on Module.__muLanguage before main()
    // runs. Use it as the default so first-boot picks up the host language
    // without the user having to touch settings.
    emscripten::val muLanguage = emscripten::val::module_property("__muLanguage");
    if (!muLanguage.isUndefined() && !muLanguage.isNull()) {
        std::string jsLanguage = muLanguage.as<std::string>();
        if (!jsLanguage.empty()) {
            defaultLanguage = jsLanguage;
        }
    }
#endif
    settings()->setDefaultValue(LANGUAGE_KEY, Val(defaultLanguage));
    settings()->valueChanged(LANGUAGE_KEY).onReceive(nullptr, [this](const Val& val) {
        m_currentLanguageCodeChanged.send(val.toQString());
    });
}

ValCh<QString> LanguagesConfiguration::currentLanguageCode() const
{
    ValCh<QString> result;
    result.ch = m_currentLanguageCodeChanged;
    result.val = settings()->value(LANGUAGE_KEY).toQString();

    return result;
}

void LanguagesConfiguration::setCurrentLanguageCode(const QString& languageCode) const
{
    Val value(languageCode.toStdString());
    settings()->setSharedValue(LANGUAGE_KEY, value);
}

QUrl LanguagesConfiguration::languagesUpdateUrl() const
{
    return QUrl(m_config.value("server_url").toQString() + "details.json");
}

QUrl LanguagesConfiguration::languageFileServerUrl(const QString& languageCode) const
{
    TRACEFUNC;
    return QUrl(m_config.value("server_url").toQString() + QString("locale_%1.zip").arg(languageCode));
}

io::path_t LanguagesConfiguration::languagesAppDataPath() const
{
    return globalConfiguration()->appDataPath() + "/locale";
}

io::path_t LanguagesConfiguration::languagesUserAppDataPath() const
{
    return globalConfiguration()->userAppDataPath() + "/locale";
}

io::path_t LanguagesConfiguration::builtinLanguagesJsonPath() const
{
    return languagesAppDataPath() + "/languages.json";
}

io::path_t LanguagesConfiguration::builtinLanguageFilePath(const QString& resourceName, const QString& languageCode) const
{
    return languagesAppDataPath() + QString("/%1_%2.qm").arg(resourceName, languageCode);
}

io::path_t LanguagesConfiguration::userLanguageFilePath(const QString& resourceName, const QString& languageCode) const
{
    return languagesUserAppDataPath() + QString("/%1_%2.qm").arg(resourceName, languageCode);
}

QStringList LanguagesConfiguration::languageResourceNames() const
{
    const Val& val = m_config.value("resource_names");
    QStringList result;
    for (const Val& item : val.toList()) {
        result.append(item.toQString());
    }
    return result;
}
