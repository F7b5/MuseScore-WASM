import MuImpl from "./muimpl.js"

const DEFAULT_SOUNDFONT = "sound/MS%20Basic.sf3"

// Resolved when C++ StartupScenario fires onAppReady (i.e. notation page is
// open and the dispatcher is wired). Methods that drive the running app
// (save, deleteSelection, ...) await this so a caller invoking them before
// startup completes gets queued instead of silently dropped.
let _appReadyResolve
const _appReady = new Promise(function(resolve) { _appReadyResolve = resolve })

const MuApi = {
    // Resolves once the app is ready to receive commands.
    ready: _appReady,

    // Load score (.mscz binary)
    loadScoreFile: MuImpl.loadScoreFile.bind(MuImpl),
    loadScoreData: MuImpl.loadScoreData.bind(MuImpl),

    // Load score from raw .mscx XML bytes
    loadRawData: MuImpl.loadRawData.bind(MuImpl),

    // Open the new score dialog (name → instrument → opened)
    newProject: MuImpl.newProject.bind(MuImpl),

    // Start audio
    startAudioProcessing: MuImpl.startAudioProcessing.bind(MuImpl),

    // Trigger the app's regular save action — fires onSave callback
    save: async function() {
        await _appReady
        MuApi.Module._save()
    },

    deleteSelection: async function() {
        await _appReady
        MuApi.Module._deleteSelection()
    },

    projectTitle: function() {
        if (!MuApi.Module) {
            return "";
        }

        return MuApi.Module.ccall('projectTitle', 'string', [], []) || "";
    },

    // Serialize current project as .mscs XML — fires onSaveRaw callback
    serializeAsXml: async function() {
        await _appReady
        MuApi.Module._serializeAsXml()
    },
}

async function createMuApi(config) {

    if (!config.soundFont) {
        config.soundFont = window.location.origin + "/wasm/" + DEFAULT_SOUNDFONT
    }

    // language: POSIX-ish code like "fr", "de_DE", "zh_CN", or "system" (default).
    // "system" / unset → resolved from navigator.languages at preload time.
    if (!config.language) {
        config.language = "system"
    }

    MuApi.Module = await MuImpl.loadModule(config)

    // C++ StartupScenario fires onAppReady once the notation page is open
    // and the dispatcher is ready. That's when it's safe to load a score
    // or open the new score dialog — calling these from onLoaded /
    // onRuntimeInitialized is too early (main() hasn't finished setup).
    // C++ also sets Module._appReady=true so we can detect a missed signal.
    let appReadyHandled = false
    const handleAppReady = function() {
        // Both the C++ side (via module_property("onAppReady")) and the
        // _appReady replay below can invoke this — guard against re-entry
        // so we don't double-load the score / re-open the new project dialog.
        if (appReadyHandled) {
            return
        }
        appReadyHandled = true

        if (config.rawScoreData) {
            const data = config.rawScoreData instanceof Uint8Array
                ? config.rawScoreData
                : new Uint8Array(config.rawScoreData)
            MuImpl.loadRawData(data, config.scoreName)
        } else if (config.scoreData) {
            const data = config.scoreData instanceof Uint8Array
                ? config.scoreData
                : new Uint8Array(config.scoreData)
            MuImpl.loadScoreData(data, config.scoreName)
        } else {
            MuImpl.newProject()
        }

        _appReadyResolve()

        if (config.onAppReady) {
            config.onAppReady()
        }
    }

    MuApi.Module.onAppReady = handleAppReady

    // If C++ already fired onAppReady before we got here, replay it now.
    // NOTE: _appReady is set via EM_ASM({ Module['_appReady'] = true }), which
    // targets the correct module instance (not the MODULARIZE factory).
    if (MuApi.Module._appReady) {
        handleAppReady()
    }

    // Wire C++ callbacks straight through to consumer config callbacks.
    MuApi.Module.onSave = function(data) {
        if (config.onSave) {
            config.onSave(data)
        }
    }

    MuApi.Module.onSaveRaw = function(data) {
        if (config.onSaveRaw) {
            config.onSaveRaw(data)
        }
    }

    MuApi.Module.onNeedSave = function(needSave) {
        if (config.onNeedSave) {
            config.onNeedSave(needSave)
        }
    }

    MuApi.Module.onTitleChanged = function(title) {
        if (config.onTitleChanged) {
            config.onTitleChanged(title)
        }
    }

    return MuApi
}

export default createMuApi;
