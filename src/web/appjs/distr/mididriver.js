let MidiDriver = (function() {
    let midiAccess = null;
    let connectedInput = null;
    let connectedOutput = null;

    return {
        requestAccess: async function() {
            console.log("[MidiDriver] requestAccess called");
            console.log("[MidiDriver] navigator.requestMIDIAccess available?", !!navigator.requestMIDIAccess);
            if (!navigator.requestMIDIAccess) {
                console.warn("[MidiDriver] Web MIDI API not supported in this browser");
                return;
            }
            try {
                console.log("[MidiDriver] Calling navigator.requestMIDIAccess()...");
                midiAccess = await navigator.requestMIDIAccess({ sysex: false });
                midiAccess.onstatechange = function(e) {
                    if (typeof Module !== 'undefined') {
                        if (Module.webMidiOnDevicesChanged) {
                            Module.webMidiOnDevicesChanged();
                        }
                        if (Module.webMidiOutOnDevicesChanged) {
                            Module.webMidiOutOnDevicesChanged();
                        }
                    }
                };
                console.log("[MidiDriver] MIDI access granted, inputs:", midiAccess.inputs.size, "outputs:", midiAccess.outputs.size);
            } catch (err) {
                console.warn("[MidiDriver] MIDI access denied:", err);
            }
        },

        getInputDevices: function() {
            if (!midiAccess) return [];
            var devices = [];
            midiAccess.inputs.forEach(function(input) {
                devices.push({ id: input.id, name: input.name || input.id });
            });
            return devices;
        },

        getOutputDevices: function() {
            if (!midiAccess) return [];
            var devices = [];
            midiAccess.outputs.forEach(function(output) {
                devices.push({ id: output.id, name: output.name || output.id });
            });
            return devices;
        },

        connectInput: function(deviceId) {
            if (!midiAccess) return false;
            var input = midiAccess.inputs.get(deviceId);
            if (!input) return false;

            if (connectedInput) {
                connectedInput.onmidimessage = null;
            }

            connectedInput = input;
            input.onmidimessage = function(msg) {
                var d = msg.data;
                // Pack MIDI bytes into uint32: status in LSB (little-endian)
                // Matches Event::fromMidi10Package layout
                var packed = d[0]
                    | (d.length > 1 ? d[1] << 8 : 0)
                    | (d.length > 2 ? d[2] << 16 : 0);
                Module.webMidiOnMessage(packed);
            };

            console.log("[MidiDriver] Connected input:", input.name);
            return true;
        },

        disconnectInput: function() {
            if (connectedInput) {
                connectedInput.onmidimessage = null;
                connectedInput = null;
            }
        },

        connectOutput: function(deviceId) {
            console.log("[MidiDriver] connectOutput called, deviceId:", deviceId, "midiAccess:", !!midiAccess);
            if (!midiAccess) {
                console.warn("[MidiDriver] connectOutput: no midiAccess");
                return false;
            }
            console.log("[MidiDriver] connectOutput: available outputs:");
            midiAccess.outputs.forEach(function(output) {
                console.log("[MidiDriver]   id:", output.id, "name:", output.name, "state:", output.state);
            });
            var output = midiAccess.outputs.get(deviceId);
            if (!output) {
                console.warn("[MidiDriver] connectOutput: device not found:", deviceId);
                return false;
            }
            connectedOutput = output;
            console.log("[MidiDriver] Connected output:", output.name, "id:", output.id);
            return true;
        },

        disconnectOutput: function() {
            console.log("[MidiDriver] disconnectOutput, was:", connectedOutput ? connectedOutput.name : "null");
            connectedOutput = null;
        },

        sendMidiBytes: function(byte0, byte1, byte2, count) {
            if (!connectedOutput) {
                console.warn("[MidiDriver] sendMidiBytes: no connectedOutput!");
                return;
            }
            var data = [byte0];
            if (count > 1) data.push(byte1);
            if (count > 2) data.push(byte2);
            console.log("[MidiDriver] sendMidiBytes:", data.map(function(b){return '0x'+b.toString(16)}).join(' '), "to:", connectedOutput.name);
            connectedOutput.send(data);
        }
    };
})();

export default MidiDriver;
