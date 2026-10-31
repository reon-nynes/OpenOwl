# Qt session architecture spike

**Disposable C++20 / Qt 6 / Qt Quick experiment.** This is an architecture question made runnable, not the production application or an approved final GUI.

Question: can one continuously rendering Qt session own shared layer/folder identities and effect stacks, independent live/timeline property values, and activation toward a moving cursor, while several independent clients inspect and edit it?

## Run

From the repository root:

```powershell
.\prototypes\qt-session\run.cmd -Launch
```

The script builds and opens the Qt Quick debug client. It connects to an existing spike session or starts the separate engine automatically. **Closing the window leaves the engine running.** Reopening the client reconnects to the same session.

```powershell
.\prototypes\qt-session\run.cmd -BuildOnly
```

The local experiment uses Qt 6.10.2 MSVC x64 at `C:/Qt/6.10.2/msvc2022_64` (override `OWL_QT_ROOT`), MSVC 2022 Build Tools, CMake, and the already downloaded Spout checkout in `../../../OwlNativePrototype/vendor/spout/SPOUTSDK`. CMake accepts `-DSPOUT_DIR=...` to use another checkout. No dependency installation or fetch is performed. The source reference checkout was pinned to `6b68a5b4fdeca6130b851e5f9366467a844860e5` by the earlier experiment.

The engine executable can be started without a UI. Before rebuilding modified engine sources, stop the running session, because Windows locks its executable.

## Try the behavior

1. Keep the scene on live output. Scrub or play the timeline; the live output stays unchanged while the timeline preview changes.
2. Pause playback, put the cursor somewhere else, and choose **Activate timeline**. Activation completes while the cursor remains paused.
3. Choose a longer activation duration. During activation, move the cursor or start playback: the target follows it. **Return to live** can reverse an in-progress activation.
4. Select **Subject folder** and change its effect opacity. The effect applies to the combined amber/cyan layers. The two effect values belong to their respective setups.
5. For an animated property, the timeline slider edits/inserts a key at the current cursor. **+ Key** starts animation for an unanimated property; **Clear** removes its animation and keeps the current value as its static timeline value. This editing convention is a prototype proposal awaiting feedback.
6. Add nested folders/layers. Select a child and move it into the root. The command interface also accepts arbitrary reparenting and rejects cycles.
7. Switch on **Studio view** to inspect both continuously rendered scenes. Inspecting a scene and taking it to output are separate actions. Scene take is a hard cut in this experiment.
8. Enable **Spout** to publish `OpenOwl Architecture Spike`. Disable it and watch the engine frame counter continue advancing.
9. Close and reopen the debug window, or use the CLI/MCP bridge while it is closed.

## CLI and MCP

With the Qt DLL directory on `PATH`, from this folder:

```powershell
$env:PATH = 'C:/Qt/6.10.2/msvc2022_64/bin;' + $env:PATH
.\build\Release\owl-control-spike.exe
.\build\Release\owl-control-spike.exe --json '{"op":"timeline.seek","scene":"stage","time":3}'
.\build\Release\owl-control-spike.exe --json '{"op":"timeline.activate","scene":"stage","enabled":true,"duration":2}'
.\build\Release\owl-control-spike.exe --json '{"op":"timeline.play","scene":"stage","playing":true}'
.\build\Release\owl-control-spike.exe --json '{"op":"stop"}'
```

Use `--file <command.json>` if the calling shell rewrites JSON quotation marks.

The same executable with `--mcp` is a small MCP stdio bridge exposing `session_state` and `session_command`. The bridge attaches to the existing session; its exit does not stop rendering. Tool names and the generic command shape are provisional diagnostic interfaces, not a final MCP interface. Inspect state for stable node and effect instance IDs. Supported operations are in `Session::dispatch` in `engine.cpp`.

## What is being exercised

- Qt Core/Network owns the session, timer, local socket connections, and command dispatch. Qt value types hold in-memory scene data. The engine process is the sole owner of state and rendering.
- Qt Quick is a separate presentation client. It owns selection and layout, sends commands, and receives state/frame snapshots.
- Live values, static timeline values, keyframe tracks, evaluated timeline values, and values resolved for rendering are distinct.
- Activation has its own duration and weight. Its running target is evaluated at the current cursor each frame. Reversing starts from the current weight.
- Folder traversal evaluates children before folder effects. The stored child order is back to front; the debug tree is displayed front to back.
- Layer/effect control identities are updated separately from frame telemetry. An early all-snapshot implementation rebuilt controls every frame; the spike now keeps their delegates stable.

## Evidence and limits

Runtime probe reports and offscreen Qt Quick captures are recorded under `evidence/`. Fresh local captures and logs go under ignored `artifacts/`.

The compositor uses **Qt QImage/QPainter on the CPU with generated fixtures at 640×360**. Only a diagnostic opacity effect is implemented. Spout uploads that CPU result to D3D11; the output toggle controls the real sender. This does not validate the production GPU compositor, recovered HLSL, all Owl effects, video/audio, cameras, recording, or production threading/performance.

The independent receiver diagnostic uses a different process and D3D11 device. It compares six received frames against a static CPU fixture, excluding the last 12 rows containing the moving session-clock marker. This is a bounded SDK check, not an OBS integration test.

The session runs at a nominal 30 Hz; previews are PNG snapshots at about 10 Hz. Commands and CPU rendering share one engine event loop. A slow operation can delay both; production worker ownership, scheduling, change delivery, undo, persistence, project format, and GPU preview handoff remain design work. All session state is in memory. Closing the engine loses it.

The prototype is isolated on `prototype/qt-session`. It must not be promoted wholesale into production.
