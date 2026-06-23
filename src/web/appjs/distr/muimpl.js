import config from "./config.js";
import qtLoad from "./qtloader.js";
import AudioDriver from "./audiodriver.js";
import MidiDriver from "./mididriver.js";

// Maps BCP-47-ish browser tags / config values to the codes MuseScore uses
// for its .qm filenames. Mirrors LanguagesService::effectiveLanguageCode().
function normalizeLanguageCode(code) {
    if (!code) return "";
    // Browser gives "en-US"; we want "en_US".
    code = code.replace(/-/g, "_");
    // Special cases matching LanguagesService::effectiveLanguageCode.
    const special = {
        "ca_valencia": "ca@valencia",
        "en": "en_US",
        "en_AU": "en_GB",
        "hi": "hi_IN",
        "mn": "mn_MN",
        "zh": "zh_CN",
    };
    if (special[code]) return special[code];
    return code;
}

// Resolve the language code we should load, given an explicit opt and the
// manifest from languages.json. Falls back through navigator.languages when
// opt is "system" (or absent). Returns { code, entry } with code the key
// into langsManifest, or null if none matched (→ English/built-in).
function resolveLanguage(opt, langsManifest) {
    const candidates = [];
    if (opt && opt !== "system") {
        candidates.push(opt);
    } else if (typeof navigator !== "undefined" && navigator.languages) {
        for (const tag of navigator.languages) candidates.push(tag);
        if (navigator.language) candidates.push(navigator.language);
    }
    for (const raw of candidates) {
        const code = normalizeLanguageCode(raw);
        if (langsManifest[code]) return { code, entry: langsManifest[code] };
        // Strip region and retry (e.g. "de_AT" → "de_DE" via normalize? no —
        // manifest keys are specific, so fall back to bare language match).
        const bare = code.split("_")[0].split("@")[0];
        for (const key of Object.keys(langsManifest)) {
            if (key === bare || key.startsWith(bare + "_") || key.startsWith(bare + "@")) {
                return { code: key, entry: langsManifest[key] };
            }
        }
    }
    return null;
}

// Runs as an async preRun step: downloads languages.json + the .qm files for
// the resolved locale (plus fallbacks) and writes them into MEMFS at
// /files/share/locale/ — where LanguagesService::loadLanguages() looks.
async function preloadLocale(instance, wasmBase, opt) {
    const LOCALE_DIR = "/files/share/locale";
    const mkdirs = (dir) => {
        const parts = dir.split("/").filter(Boolean);
        let p = "";
        for (const part of parts) {
            p += "/" + part;
            try { instance.FS.mkdir(p); }
            catch (e) { if (e.errno !== 20) throw e; }
        }
    };

    let manifestBytes;
    let manifest;
    try {
        const res = await fetch(wasmBase + "locale/languages.json");
        if (!res.ok) throw new Error("HTTP " + res.status);
        manifestBytes = new Uint8Array(await res.arrayBuffer());
        manifest = JSON.parse(new TextDecoder().decode(manifestBytes));
    } catch (e) {
        console.warn("[locale] languages.json unavailable, skipping preload:", e);
        instance.__muLanguage = "en_US";
        return;
    }

    const resolved = resolveLanguage(opt, manifest);
    const resolvedCode = resolved ? resolved.code : "en_US";
    console.info("[locale] resolved language:", resolvedCode, "(requested:", opt + ")");

    mkdirs(LOCALE_DIR);
    instance.FS.writeFile(LOCALE_DIR + "/languages.json", manifestBytes);

    if (!resolved) {
        instance.__muLanguage = resolvedCode;
        return;
    }

    // Load main locale + fallbacks (manifest entry has fallbackLanguages array).
    const codesToLoad = new Set([resolved.code]);
    for (const fb of (resolved.entry.fallbackLanguages || [])) {
        codesToLoad.add(fb);
    }

    const resources = ["musescore", "instruments"];
    await Promise.all([...codesToLoad].flatMap((code) =>
        resources.map(async (name) => {
            const file = name + "_" + code + ".qm";
            try {
                const res = await fetch(wasmBase + "locale/" + file);
                if (!res.ok) {
                    console.warn("[locale] missing", file, "(HTTP " + res.status + ")");
                    return;
                }
                const bytes = new Uint8Array(await res.arrayBuffer());
                instance.FS.writeFile(LOCALE_DIR + "/" + file, bytes);
            } catch (e) {
                console.warn("[locale] failed to fetch", file, e);
            }
        })
    ));

    instance.__muLanguage = resolvedCode;
}

function setupInternalCallbacks(Module) {

    // Interactive
    Module.openFileDialog = function(callback) {
        console.log("[js] openFileDialog")
        const input = document.createElement('input');
        input.type = 'file';
        input.onchange = (e) => {
            const file = e.target.files[0];
            const fileName = file.name
            const reader = new FileReader();
            reader.onload = (e) => {
                const contents = e.target.result;
                const uint8View = new Uint8Array(contents);
                console.log("[js] openFileDialog fileName: ", fileName, ", contents: ", uint8View.length, ", [0]=", uint8View[0])
                callback(fileName, uint8View);
            };
            reader.readAsArrayBuffer(file); 
        };
        input.click();
    }
}

function setupRpc(Module)
{
    // Main <=> Worker 
    // port1 - main
    // port2 - worker
    Module.main_worker_rpcChannel = new MessageChannel();

    Module.main_worker_rpcSend = function(data) {
        Module.main_worker_rpcChannel.port1.postMessage(data)
    }

    Module.main_worker_rpcListen = function(data) {} // will be overridden

    Module.main_worker_rpcChannel.port1.onmessage = function(event) {
        Module.main_worker_rpcListen(event.data)
    };

    // Worker <=> Driver (processor)
    // port1 - driver
    // port2 - worker
    Module.driver_worker_rpcChannel = new MessageChannel();
}

async function setupDriver(Module) 
{
    Module.driver = AudioDriver;

    AudioDriver.onInited = function() {
        console.log("driver on inited add sound font")

        if (Module.isNeedStartAudio) {
            Module._startAudioProcessing()
        }

        Module.ccall('addSoundFont', '', ['string'], [Module.soundFont]);
    }

    // Bridge MIDI output from AudioWorklet to Web MIDI API
    AudioDriver.onMidiOut = function(byte0, byte1, byte2, count) {
        if (Module.midiDriver) {
            Module.midiDriver.sendMidiBytes(byte0, byte1, byte2, count);
        }
    }

    if (config.MUSE_MODULE_AUDIO_WORKER == "ON") {
        await AudioDriver.setup(Module.config, Module.driver_worker_rpcChannel.port1);
    } else {
        await AudioDriver.setup(Module.config, Module.main_worker_rpcChannel.port2);
    }

}

async function setupWorker(Module)
{
    // Initialize the worker.
    Module.worker = new Worker("/wasm/distr/audioworker.js")

    var museAudioUrl = "/wasm/MuseAudio.js";

    Module.worker.onmessage = function(event) {
        if (event.data.type == "WORKER_INITED") {
            Module.ccall('addSoundFont', '', ['string'], [Module.soundFont]);
        }
    }

    Module.worker.postMessage({
    type: 'INITIALIZE_WORKER',
    mainPort: Module.main_worker_rpcChannel.port2,
    driverPort: Module.driver_worker_rpcChannel.port2,
    options: {
        museAudioUrl: museAudioUrl
    }
    }, [Module.main_worker_rpcChannel.port2, Module.driver_worker_rpcChannel.port2]);
}

const MuImpl = {

    Module: {},

    loadModule: async function(opt) {

        console.info("STEP 0: Begin load main module")

        // Resolve WASM assets relative to the MuseScoreStudio.js script, not the page URL
        var wasmBase = '/wasm/';
        var scriptEl = document.querySelector('script[src*="MuseScoreStudio.js"]');
        if (scriptEl) {
            var src = scriptEl.getAttribute('src');
            wasmBase = src.substring(0, src.lastIndexOf('/') + 1);
        }

        this.Module = {
            config: config, // static configuration

            locateFile: function(filename) {
                return wasmBase + filename;
            },

            qt: {
                onLoaded: opt.onLoaded,
                onExit: opt.onExit,
                entryFunction: window.MuseScoreStudio_entry, // from MuseScoreStudio.js
                containerElements: [opt.screen],
                environment: {
                    TZ: "UTC"
                }
            },

            soundFont: opt.soundFont,

            // Block emscripten's start until locale preload finishes writing
            // .qm files into MEMFS at /files/share/locale/. LanguagesService
            // reads from there during onPreInit, so the files must be present
            // before main() runs.
            preRun: [function(instance) {
                instance.addRunDependency("mu-locale-preload");
                preloadLocale(instance, wasmBase, opt.language)
                    .catch(function(e) { console.warn("[locale] preload failed:", e); })
                    .finally(function() { instance.removeRunDependency("mu-locale-preload"); });
            }],

            // called from cpp
            onStartApp: this._onStartApp.bind(this)
        }

        setupRpc(this.Module);
        console.info("STEP 0.1: End setupRpc")
        setupInternalCallbacks(this.Module);
        console.info("STEP 0.2: End setupInternalCallbacks")
        // Attach MidiDriver before qtLoad so _onStartApp can use it
        this.Module.midiDriver = MidiDriver;
        console.info("STEP 0.2.2: Attached MidiDriver to Module")

        this.Module = await qtLoad(this.Module);
        console.info("STEP 0.3: End load main module")

        // Re-attach to the qtLoad-returned Module for C++ emscripten::val access
        this.Module.midiDriver = MidiDriver;
        console.info("STEP 0.3.1: Re-attached MidiDriver")

        return this.Module;
    },

    _onStartApp: async function() {
        console.info("STEP 1: Begin on onStartApp")

        // Request MIDI access (triggers browser permission prompt)
        console.info("STEP 1.0: midiDriver present?", !!this.Module.midiDriver)
        if (this.Module.midiDriver) {
            console.info("STEP 1.0: Calling midiDriver.requestAccess()...")
            this.Module.midiDriver.requestAccess();
            console.info("STEP 1.0: MIDI access requested (async)")
        } else {
            console.warn("STEP 1.0: midiDriver NOT found on Module")
        }

        if (config.MUSE_AUDIO_ENGINE !== "ON") {
            console.info("STEP 1.1: Skip setupDriver (audio engine disabled)")
            return;
        }

        await setupDriver(this.Module);
        console.info("STEP 1.1: End setupDriver")

        if (config.MUSE_MODULE_AUDIO_WORKER == "ON") {
            await setupWorker(this.Module);
            console.info("STEP 1.2: End setupWorker")
        }
    },

    loadScoreFile: async function(file, name) {
        if (!file) {
            return
        }

        const buffer = await file.arrayBuffer();
        this.loadScoreData(new Uint8Array(buffer), name || file.name)
    },

    loadScoreData: function(data, name) {
        const ptr = this.Module._malloc(data.length);
        this.Module.HEAPU8.set(data, ptr);
        this.Module.ccall('load', null, ['string', 'number', 'number'], [name || 'score', ptr, data.length]);
        this.Module._free(ptr);
    },

    // Load from raw .mscx XML bytes — C++ writes to a .mscx temp file and opens it.
    loadRawData: function(data, name) {
        const ptr = this.Module._malloc(data.length);
        this.Module.HEAPU8.set(data, ptr);
        this.Module.ccall('loadRaw', null, ['string', 'number', 'number'], [name || 'score', ptr, data.length]);
        this.Module._free(ptr);
    },

    newProject: function() {
        this.Module._newProject();
    },

    startAudioProcessing: async function() {
        if (!this.Module.driver) {
            console.warn("audio driver is disabled")
            return;
        }

        if (this.Module.driver.inited) {
            this.Module._startAudioProcessing()
        } else {
            console.log("driver not inited, start audio will be later")
            this.Module.isNeedStartAudio = true;
        }
    }
}

export default MuImpl;
