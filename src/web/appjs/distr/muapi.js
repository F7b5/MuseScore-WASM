import MuImpl from "./muimpl.js"

const DEFAULT_SOUNDFONT = "sound/MS%20Basic.sf3"

const MuApi = {
    _onSave: null,

    // Load score
    loadScoreFile: MuImpl.loadScoreFile,
    loadScoreData: MuImpl.loadScoreData,

    // Start audio
    startAudioProcessing: MuImpl.startAudioProcessing.bind(MuImpl),

    // Trigger the app's regular save action
    save: function() {
        if (MuApi.Module) {
            MuApi.Module._save();
        }
    },

    // Register a handler that receives the saved .mscz project bytes
    registerOnSave: function(handler) {
        MuApi._onSave = handler;
    },

    // Serialize current project as .mscs XML — calls onProjectSerialized callback
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

    MuApi.Module.onProjectSaved = function(data) {
        console.log("[js muapi internal] onProjectSaved len: ", data.length)
        if (config.onProjectSaved) {
            config.onProjectSaved(data)
        }
    }

    MuApi.Module.onSave = function(data) {
        if (MuApi._onSave) {
            MuApi._onSave(data)
        }
        if (config.onSave) {
            config.onSave(data)
        }
    }

    MuApi.Module.onProjectSerialized = function(data) {
        if (config.onProjectSerialized) {
            config.onProjectSerialized(data)
        }
    }

    MuApi.Module.onNeedSave = function(needSave) {
        if (config.onNeedSave) {
            config.onNeedSave(needSave)
        }
    }

    if (config.onSave) {
        MuApi.registerOnSave(config.onSave)
    }

    return MuApi
}

export default createMuApi;
