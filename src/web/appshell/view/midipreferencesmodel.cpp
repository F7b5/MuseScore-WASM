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
#include "midipreferencesmodel.h"

#include "types/ret.h"
#include "log.h"

using namespace muse;
using namespace mu::appshell;
using namespace muse::midi;

MidiPreferencesModel::MidiPreferencesModel(QObject* parent)
    : QObject(parent)
{
}

void MidiPreferencesModel::load()
{
    LOGI() << "[MidiPrefs] load() called";
    LOGI() << "[MidiPrefs] Output devices available: " << midiOutPort()->availableDevices().size();
    for (const MidiDevice& dev : midiOutPort()->availableDevices()) {
        LOGI() << "[MidiPrefs]   output: id=" << dev.id << " name=" << dev.name;
    }
    LOGI() << "[MidiPrefs] Input devices available: " << midiInPort()->availableDevices().size();
    for (const MidiDevice& dev : midiInPort()->availableDevices()) {
        LOGI() << "[MidiPrefs]   input: id=" << dev.id << " name=" << dev.name;
    }
    LOGI() << "[MidiPrefs] Current output device: " << midiOutPort()->deviceID()
           << " connected=" << midiOutPort()->isConnected();
    LOGI() << "[MidiPrefs] Current input device: " << midiInPort()->deviceID()
           << " connected=" << midiInPort()->isConnected();

    midiInPort()->availableDevicesChanged().onNotify(this, [this]() {
        LOGI() << "[MidiPrefs] Input devices changed notification";
        emit devicesChanged();
    });

    midiOutPort()->availableDevicesChanged().onNotify(this, [this]() {
        LOGI() << "[MidiPrefs] Output devices changed notification";
        emit devicesChanged();
    });

    emit devicesChanged();
}

QVariantList MidiPreferencesModel::inputDevices() const
{
    QVariantList list;
    QVariantMap none;
    none["id"] = QString::fromStdString(NONE_DEVICE_ID);
    none["name"] = tr("None");
    list << none;

    for (const MidiDevice& dev : midiInPort()->availableDevices()) {
        QVariantMap m;
        m["id"] = QString::fromStdString(dev.id);
        m["name"] = QString::fromStdString(dev.name);
        list << m;
    }
    return list;
}

QVariantList MidiPreferencesModel::outputDevices() const
{
    QVariantList list;
    QVariantMap none;
    none["id"] = QString::fromStdString(NONE_DEVICE_ID);
    none["name"] = tr("None");
    list << none;

    for (const MidiDevice& dev : midiOutPort()->availableDevices()) {
        QVariantMap m;
        m["id"] = QString::fromStdString(dev.id);
        m["name"] = QString::fromStdString(dev.name);
        list << m;
    }
    return list;
}

QString MidiPreferencesModel::currentInputDeviceId() const
{
    return QString::fromStdString(midiInPort()->deviceID());
}

QString MidiPreferencesModel::currentOutputDeviceId() const
{
    return QString::fromStdString(midiOutPort()->deviceID());
}

void MidiPreferencesModel::setInputDevice(const QString& deviceId)
{
    std::string id = deviceId.toStdString();
    if (id == NONE_DEVICE_ID) {
        midiInPort()->disconnect();
    } else {
        midiInPort()->connect(id);
    }
    emit devicesChanged();
}

void MidiPreferencesModel::setOutputDevice(const QString& deviceId)
{
    std::string id = deviceId.toStdString();
    LOGI() << "[MidiPrefs] setOutputDevice: " << id;
    if (id == NONE_DEVICE_ID) {
        midiOutPort()->disconnect();
        LOGI() << "[MidiPrefs] Output disconnected";
    } else {
        Ret ret = midiOutPort()->connect(id);
        LOGI() << "[MidiPrefs] Output connect result: " << ret.toString()
               << " isConnected=" << midiOutPort()->isConnected()
               << " deviceID=" << midiOutPort()->deviceID();
    }
    emit devicesChanged();
}
