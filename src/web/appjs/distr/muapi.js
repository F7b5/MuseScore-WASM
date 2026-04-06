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

    // After Qt is ready, the JS layer is responsible for opening either:
    //   - the provided scoreData (named via scoreName, used as the tab name), or
    //   - the new score dialog when scoreData is absent.
    // Must wrap before loadModule, since loadModule captures config.onLoaded.
    {
        const origOnLoaded = config.onLoaded
        config.onLoaded = function() {
            if (config.scoreData) {
                const data = config.scoreData instanceof Uint8Array
                    ? config.scoreData
                    : new Uint8Array(config.scoreData)
                MuImpl.loadScoreData(data, config.scoreName)
            } else {
                MuImpl.newProject()
            }

            if (origOnLoaded) {
                origOnLoaded()
            }
        }
    }

    MuApi.Module = await MuImpl.loadModule(config)

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

    return MuApi
}

export default createMuApi;
