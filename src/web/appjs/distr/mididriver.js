let MidiDriver = (function() {
    let midiAccess = null;
    let connectedInput = null;
    let connectedOutput = null;

    return {
        requestAccess: async function() {
            if (!navigator.requestMIDIAccess) {
                console.warn("[MidiDriver] Web MIDI API not supported in this browser");
                return;
            }
            try {
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
            if (!midiAccess) return false;
            var output = midiAccess.outputs.get(deviceId);
            if (!output) return false;
            connectedOutput = output;
            console.log("[MidiDriver] Connected output:", output.name);
            return true;
        },

        disconnectOutput: function() {
            connectedOutput = null;
        },

        sendMidiBytes: function(byte0, byte1, byte2, count) {
            if (!connectedOutput) return;
            var data = [byte0];
            if (count > 1) data.push(byte1);
            if (count > 2) data.push(byte2);
            connectedOutput.send(data);
        }
    };
})();

export default MidiDriver;
