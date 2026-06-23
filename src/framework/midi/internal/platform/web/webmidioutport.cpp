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
#include "webmidioutport.h"

#include <emscripten.h>
#include <emscripten/val.h>

#include "log.h"
#include "midierrors.h"

using namespace muse;
using namespace muse::midi;

static WebMidiOutPort* g_webMidiOutPort = nullptr;

void WebMidiOutPort::init()
{
    g_webMidiOutPort = this;
    LOGI() << "WebMidiOutPort initialized";
}

void WebMidiOutPort::deinit()
{
    disconnect();
    g_webMidiOutPort = nullptr;
}

MidiDeviceList WebMidiOutPort::availableDevices() const
{
    MidiDeviceList devices;

    MidiDevice noDevice;
    noDevice.id = NONE_DEVICE_ID;
    noDevice.name = "No device";
    devices.push_back(noDevice);

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (midiDriver.isUndefined() || midiDriver.isNull()) {
        return devices;
    }

    emscripten::val outputs = midiDriver.call<emscripten::val>("getOutputDevices");
    if (outputs.isUndefined() || outputs.isNull()) {
        return devices;
    }

    int length = outputs["length"].as<int>();
    for (int i = 0; i < length; ++i) {
        emscripten::val dev = outputs[i];
        MidiDevice d;
        d.id = dev["id"].as<std::string>();
        d.name = dev["name"].as<std::string>();
        devices.push_back(d);
    }

    return devices;
}

async::Notification WebMidiOutPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

Ret WebMidiOutPort::connect(const MidiDeviceID& deviceID)
{
    if (deviceID == NONE_DEVICE_ID) {
        disconnect();
        return Ret(true);
    }

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (midiDriver.isUndefined() || midiDriver.isNull()) {
        return make_ret(Err::MidiFailedConnect);
    }

    bool ok = midiDriver.call<bool>("connectOutput", deviceID);
    if (!ok) {
        LOGE() << "Failed to connect MIDI output device: " << deviceID;
        return make_ret(Err::MidiFailedConnect);
    }

    m_connectedDeviceID = deviceID;
    m_deviceChanged.notify();

    LOGI() << "Connected MIDI output device: " << deviceID;
    return Ret(true);
}

void WebMidiOutPort::disconnect()
{
    if (m_connectedDeviceID.empty()) {
        return;
    }

    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (!midiDriver.isUndefined() && !midiDriver.isNull()) {
        midiDriver.call<void>("disconnectOutput");
    }

    m_connectedDeviceID.clear();
    m_deviceChanged.notify();
}

bool WebMidiOutPort::isConnected() const
{
    return !m_connectedDeviceID.empty();
}

MidiDeviceID WebMidiOutPort::deviceID() const
{
    return m_connectedDeviceID;
}

async::Notification WebMidiOutPort::deviceChanged() const
{
    return m_deviceChanged;
}

bool WebMidiOutPort::supportsMIDI20Output() const
{
    return false;
}

Ret WebMidiOutPort::sendEvent(const Event& e)
{
    // Convert MIDI 2.0 events to MIDI 1.0 for the Web MIDI API
    if (e.messageType() == Event::MessageType::ChannelVoice20) {
        auto events = e.toMIDI10();
        for (const Event& e10 : events) {
            sendEvent(e10);
        }
        return Ret(true);
    }

    if (e.messageType() != Event::MessageType::ChannelVoice10) {
        return Ret(true);
    }

    unsigned char bytes[3];
    size_t count = e.toMidi10Bytes(bytes);

    if (count == 0) {
        return Ret(true);
    }

    int b0 = static_cast<int>(bytes[0]);
    int b1 = static_cast<int>(count > 1 ? bytes[1] : 0);
    int b2 = static_cast<int>(count > 2 ? bytes[2] : 0);
    int c = static_cast<int>(count);

    // Try midiDriver on main thread Module first (main thread context)
    emscripten::val midiDriver = emscripten::val::module_property("midiDriver");
    if (!midiDriver.isUndefined() && !midiDriver.isNull()) {
        midiDriver.call<void>("sendMidiBytes", b0, b1, b2, c);
        return Ret(true);
    }

    // In AudioWorklet context: relay MIDI to main thread via postMessage
    emscripten::val sendFn = emscripten::val::global("sendMidiToMain");
    if (!sendFn.isUndefined() && !sendFn.isNull()) {
        sendFn(b0, b1, b2, c);
        return Ret(true);
    }

    return make_ret(Err::MidiNotConnected);
}

void WebMidiOutPort::onDevicesChanged()
{
    m_availableDevicesChanged.notify();
}
