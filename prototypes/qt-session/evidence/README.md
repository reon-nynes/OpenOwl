# Captured architecture evidence

These are bounded runtime observations for a disposable prototype. The verdict is that the requested state model can be exercised through one running Qt session and independent Qt Quick, CLI, and MCP clients. User evaluation of the interaction model and a reviewed production design remain outstanding.

- `session-probe.json`: rendering with no UI, independent value banks, activation with paused playback, cursor-following activation, reversal, playback/activation independence, effect keyframes, rejected folder cycles, and a folder-composite opacity pixel.
- `mcp-probe.json`: initialize, tools/list, and tools/call over the stdio bridge, operating on the same engine session.
- `spout-toggle-probe.json`: two sender enable/disable cycles. A separate receiver process and D3D11 device receives six frames in each cycle. The static top 348 rows match exactly; the last 12 rows contain the moving clock marker and are excluded. Rendering advances while output is off, with no further Spout sends.
- `ui-lifetime-probe.json`: three independent Qt Quick clients open and close while the same session keeps rendering.
- `debug.png`, `studio.png`, `narrow.png`, and their JSON reports: offscreen Qt Quick loading and capture at 1440×1040 and 1100×800. The layer tree creates five delegates rather than rebuilding them for every frame update.
- `premium-audit.json`: the UI skill's static audit, with zero findings. It does not establish interactive keyboard/mouse behavior or production UI compliance.
- `source-hashes.json`: SHA-256 hashes of the experiment's implementation and launcher files at capture.

The runtime checks use simple generated images, scalar interpolation, and one diagnostic opacity effect. They do not establish original Owl shader or compositor equivalence, complete effect behavior, real media/capture handling, production performance, or interaction behavior in an external receiver such as OBS.

Source grounding: `OwlUnity/Assets/Scripts/OwlRuntime/Owl/` supplies `ProjectModel`, `SceneModel`, `LayerModel`, `SourceModel`, `Composition`, `EffectChain`, `LayerMotionClock`, `LayerMotionEval`, `SourcePool`, `SpoutOutput`, and `OwlApp`. `Owl.Mcp/ToolContext.cs` and `McpHost.cs` expose the original GUI coupling and command scheduling. The new folder hierarchy, independent configuration values, and timeline activation are the user's explicit changes to that reference behavior. No original shader implementation is being evaluated by this CPU fixture renderer.

The prototype caught two design concerns: control identity must be separated from frame telemetry, and output sender resources have a lifetime separate from the continuously rendering session. Production design should carry those lessons forward without copying this prototype's all-in-one engine event loop or snapshot transport wholesale.
