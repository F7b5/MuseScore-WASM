# MuseScore WASM Build

## Overview

This documents the work done to build MuseScore Studio as a WebAssembly module that runs the Qt application in the browser.

## What Was Done

### Toolchain Setup

- **Emscripten SDK 4.0.7** — pinned version matching Qt 6.10 WASM ABI
- **Qt 6.10.2 wasm_singlethread** — installed via `aqt` with `qt5compat` and `qtshadertools` modules
- **Qt 6.10.2 gcc_64** — used as `QT_HOST_PATH` for host tools (moc, rcc, etc.)

### Source Code Fixes (28 files modified)

The existing `app-web` build configuration was designed for an older version of the codebase. The following categories of fixes were applied to make it compile and link with the current code:

1. **Interface migration (Global → Context IoC)**
   - `IInteractive`, `IActionsDispatcher`, `IGlobalContext`, `IStartAudioController`, `ISoundFontController` changed from `MODULE_GLOBAL_INTERFACE` to `MODULE_CONTEXT_INTERFACE`
   - Converted `GlobalInject` → `ContextInject` and added `Contextable` base class where needed
   - Split modules into `IModuleSetup` (global) + `IContextSetup` (context-scoped) pattern:
     - `appshellmodule.h/cpp` → `AppShellModule` + `AppShellContext`
     - `appjsmodule.h/cpp` → `AppJsModule` + `AppJsContext`
   - `WebApi` converted to use `ContextInject` with `Contextable` base

2. **API changes**
   - `WebAudioDriver` rewritten to match simplified `IAudioDriver` interface (removed ~10 methods)
   - `WebSoundFontController` updated with `Contextable` base and context constructor
   - `resetOnReceive()` calls removed (API no longer exists)
   - `INotationConfiguration` → `INotationSceneConfiguration` (module renamed)

3. **WASM platform guards (`#ifndef Q_OS_WASM`)**
   - `QProcess` / `MsProcess` class (not available in WASM Qt)
   - `<private/qkeymapper_p.h>` (Qt private header unavailable)
   - `EngineGlobalSetup` (not compiled in WASM audio mode)

4. **Build system fixes**
   - Added `muse_module_add_qrc()` for appshell QRC resources
   - Added `Q_OS_WASM` case in `guiapp.cpp` for QML main file path (`qrc:/qml/Main.qml`)
   - Fixed `MuseAudio` CMakeLists.txt link order

5. **QML simplification**
   - `Main.qml` simplified to: `AppWindow` + `InteractiveProvider` + `NotationFrame`
   - Removed `AppMenuBar` (caused cascading QML dependency errors)

## Build Commands

### Main WASM Module
```bash
source ~/build_tools/emsdk/emsdk_env.sh
export QT_ROOT_DIR=~/Qt/6.10.2/wasm_singlethread
export QT_HOST_PATH=~/Qt/6.10.2/gcc_64
export CMAKE_TOOLCHAIN_FILE=${QT_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake

MUSE_APP_BUILD_MODE=dev \
MUSESCORE_BUILD_CONFIGURATION="app-web" \
MUSESCORE_BUILD_NUMBER=12345678 \
MUSESCORE_REVISION=$(git rev-parse --short=7 HEAD) \
bash ./ninja_build.sh -t release
```

### Serve Locally
```bash
cd build.artifacts
npx http-server --cors -c-1 -p 8082 ./
```
Then open `http://localhost:8082/viewer.html` in browser.

## What Works

- **Compilation**: All 28 modified source files compile successfully with Emscripten
- **Linking**: WASM module links and produces output artifacts:
  - `build.artifacts/MuseScoreStudio.wasm` (~77 MB)
  - `build.artifacts/MuseScoreStudio.js` (~258 KB)
  - `build.artifacts/viewer.html` (entry point)
  - `build.artifacts/distr/` (JS API bridge: muapi.js, qtloader.js, etc.)
- **Loading**: Browser fetches and instantiates the WASM module

## What Doesn't Work (Yet)

- **Runtime crash**: `RuntimeError: function signature mismatch` during `callMain`
  - The app crashes at startup before any UI renders
  - Currently debugging with `-s ASSERTIONS=2` enabled for better error messages
  - Likely caused by a virtual function override with mismatched signature somewhere in the startup path
- **Audio**: `MuseAudio` standalone WASM module build is deferred (not needed for UI rendering MVP)
- **SoundFont**: `MS Basic.sf3` not bundled (file not present in working tree)
- **Menu bar**: Removed from QML to avoid dependency issues — UI will be minimal

## Key Files

| File | Role |
|------|------|
| `buildscripts/ci/wasm/build.sh` | CI build script (reference) |
| `src/app/CMakeLists.txt` | Main WASM executable config |
| `src/app/exported_functions.cmake` | Exported C functions for JS bridge |
| `src/web/appshell/` | Web-specific app shell module |
| `src/web/appjs/` | JS API bridge (WebApi, muapi.js) |
| `src/web/appjs/viewer/viewer.html` | Browser entry point |
| `buildscripts/cmake/SetupConfigure.cmake` | `app-web` build configuration |
