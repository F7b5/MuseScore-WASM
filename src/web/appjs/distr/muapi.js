import MuImpl from "./muimpl.js"

const DEFAULT_SOUNDFONT = "sound/MS%20Basic.sf3"

const MuApi = {
    // Load score
    loadScoreFile: MuImpl.loadScoreFile.bind(MuImpl),
    loadScoreData: MuImpl.loadScoreData.bind(MuImpl),

    // Open the new score dialog (name → instrument → opened)
    newProject: MuImpl.newProject.bind(MuImpl),

    // Start audio
    startAudioProcessing: MuImpl.startAudioProcessing.bind(MuImpl),

    // Trigger the app's regular save action — fires onSave callback
    save: function() {
        if (MuApi.Module) {
            MuApi.Module._save();
        }
    },

    deleteSelection: function() {
        if (MuApi.Module) {
            MuApi.Module._deleteSelection();
        }
    },

    projectTitle: function() {
        if (!MuApi.Module) {
            return "";
        }

        return MuApi.Module.ccall('projectTitle', 'string', [], []) || "";
    },

    // Serialize current project as .mscs XML — fires onSaveRaw callback
    serializeAsXml: function() {
        if (MuApi.Module) {
            MuApi.Module._serializeAsXml();
        }
    },
}

async function createMuApi(config) {

    if (!config.soundFont) {
        config.soundFont = window.location.origin + "/wasm/" + DEFAULT_SOUNDFONT
    }

    MuApi.Module = await MuImpl.loadModule(config)

    // C++ StartupScenario fires onAppReady once the notation page is open
    // and the dispatcher is ready. That's when it's safe to load a score
    // or open the new score dialog — calling these from onLoaded /
    // onRuntimeInitialized is too early (main() hasn't finished setup).
    // C++ also sets Module._appReady=true so we can detect a missed signal.
    const handleAppReady = function() {
        if (config.scoreData) {
            const data = config.scoreData instanceof Uint8Array
                ? config.scoreData
                : new Uint8Array(config.scoreData)
            MuImpl.loadScoreData(data, config.scoreName)
        } else {
            MuImpl.newProject()
        }

        if (config.onAppReady) {
            config.onAppReady()
        }
    }

    MuApi.Module.onAppReady = handleAppReady

    // If C++ already fired onAppReady before we got here, replay it now.
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
