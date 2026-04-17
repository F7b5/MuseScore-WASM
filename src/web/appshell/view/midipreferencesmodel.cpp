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

using namespace muse;
using namespace mu::appshell;
using namespace muse::midi;

MidiPreferencesModel::MidiPreferencesModel(QObject* parent)
    : QObject(parent)
{
}

void MidiPreferencesModel::load()
{
    midiInPort()->availableDevicesChanged().onNotify(this, [this]() {
        emit devicesChanged();
    });

    midiOutPort()->availableDevicesChanged().onNotify(this, [this]() {
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
    if (id == NONE_DEVICE_ID) {
        midiOutPort()->disconnect();
    } else {
        midiOutPort()->connect(id);
    }
    emit devicesChanged();
}
