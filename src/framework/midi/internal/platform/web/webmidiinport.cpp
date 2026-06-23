/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
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
#include "webmidiinport.h"

#include <emscripten/val.h>
#include <emscripten/bind.h>

#include "log.h"

using namespace muse;
using namespace muse::midi;

static WebMidiInPort* g_webMidiInPort = nullptr;

static void webMidiOnMessage(uint32_t data)
{
    if (g_webMidiInPort) {
        g_webMidiInPort->onMidiMessage(data);
    }
}

static void webMidiOnDevicesChanged()
{
    if (g_webMidiInPort) {
        g_webMidiInPort->onDevicesChanged();
    }
}

EMSCRIPTEN_BINDINGS(WebMidiIn) {
    emscripten::function("webMidiOnMessage", &webMidiOnMessage);
    emscripten::function("webMidiOnDevicesChanged", &webMidiOnDevicesChanged);
}

void WebMidiInPort::init()
{
    g_webMidiInPort = this;
    LOGI() << "WebMidiInPort initialized";
}

void WebMidiInPort::deinit()
{
    disconnect();
    g_webMidiInPort = nullptr;
}

MidiDeviceList WebMidiInPort::availableDevices() const
{
    MidiDeviceList devices;

    // Always include "no device" option
    MidiDevice noDevice;
    noDevice.id = NONE_DEVICE_ID;
    noDevice.name = "No device";
    devices.push_back(noDevice);

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (midiDriver.isUndefined() || midiDriver.isNull()) {
        return devices;
    }

    emscripten::val inputs = midiDriver.call<emscripten::val>("getInputDevices");
    if (inputs.isUndefined() || inputs.isNull()) {
        return devices;
    }

    int length = inputs["length"].as<int>();
    for (int i = 0; i < length; ++i) {
        emscripten::val dev = inputs[i];
        MidiDevice d;
        d.id = dev["id"].as<std::string>();
        d.name = dev["name"].as<std::string>();
        devices.push_back(d);
    }

    return devices;
}

async::Notification WebMidiInPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

Ret WebMidiInPort::connect(const MidiDeviceID& deviceID)
{
    if (deviceID == NONE_DEVICE_ID) {
        disconnect();
        return Ret(true);
    }

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (midiDriver.isUndefined() || midiDriver.isNull()) {
        return make_ret(Err::MidiFailedConnect);
    }

    bool ok = midiDriver.call<bool>("connectInput", deviceID);
    if (!ok) {
        LOGE() << "Failed to connect MIDI input device: " << deviceID;
        return make_ret(Err::MidiFailedConnect);
    }

    m_deviceID = deviceID;
    m_deviceChanged.notify();

    LOGI() << "Connected MIDI input device: " << deviceID;
    return Ret(true);
}

void WebMidiInPort::disconnect()
{
    if (m_deviceID.empty()) {
        return;
    }

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (!midiDriver.isUndefined() && !midiDriver.isNull()) {
        midiDriver.call<void>("disconnectInput");
    }

    m_deviceID.clear();
    m_deviceChanged.notify();
}

bool WebMidiInPort::isConnected() const
{
    return !m_deviceID.empty();
}

MidiDeviceID WebMidiInPort::deviceID() const
{
    return m_deviceID;
}

async::Notification WebMidiInPort::deviceChanged() const
{
    return m_deviceChanged;
}

async::Channel<tick_t, Event> WebMidiInPort::eventReceived() const
{
    return m_eventReceived;
}

void WebMidiInPort::onMidiMessage(uint32_t midiData)
{
    Event e = Event::fromMidi10Package(midiData);
    if (e.isValid()) {
        e = e.toMIDI20();
        m_eventReceived.send(0, e);
    }
}

void WebMidiInPort::onDevicesChanged()
{
    m_availableDevicesChanged.notify();
}
