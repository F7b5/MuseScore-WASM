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
#ifndef MU_APPSHELL_MIDIPREFERENCESMODEL_H
#define MU_APPSHELL_MIDIPREFERENCESMODEL_H

#include <QObject>
#include <QVariantList>

#include "modularity/ioc.h"
#include "midi/imidiinport.h"
#include "midi/imidioutport.h"
#include "async/asyncable.h"

namespace mu::appshell {
class MidiPreferencesModel : public QObject, public muse::async::Asyncable
{
    Q_OBJECT

    muse::GlobalInject<muse::midi::IMidiInPort> midiInPort;
    muse::GlobalInject<muse::midi::IMidiOutPort> midiOutPort;

    Q_PROPERTY(QVariantList inputDevices READ inputDevices NOTIFY devicesChanged)
    Q_PROPERTY(QVariantList outputDevices READ outputDevices NOTIFY devicesChanged)
    Q_PROPERTY(QString currentInputDeviceId READ currentInputDeviceId NOTIFY devicesChanged)
    Q_PROPERTY(QString currentOutputDeviceId READ currentOutputDeviceId NOTIFY devicesChanged)

public:
    explicit MidiPreferencesModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();

    QVariantList inputDevices() const;
    QVariantList outputDevices() const;
    QString currentInputDeviceId() const;
    QString currentOutputDeviceId() const;

    Q_INVOKABLE void setInputDevice(const QString& deviceId);
    Q_INVOKABLE void setOutputDevice(const QString& deviceId);

signals:
    void devicesChanged();
};
}

#endif // MU_APPSHELL_MIDIPREFERENCESMODEL_H
