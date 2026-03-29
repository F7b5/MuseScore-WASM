import MuImpl from "./muimpl.js"

const DEFAULT_SOUNDFONT = "sound/MS%20Basic.sf3"

const MuApi = {

    // Load score
    loadScoreFile: MuImpl.loadScoreFile,
    loadScoreData: MuImpl.loadScoreData,

    // Start audio
    startAudioProcessing: MuImpl.startAudioProcessing.bind(MuImpl),

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

    return MuApi
}

export default createMuApi;

