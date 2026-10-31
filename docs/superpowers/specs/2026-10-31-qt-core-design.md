# OpenOwlCLI: Qt session core and client interfaces

Date: 2026-10-31. Status: proposed design; user approval pending.

This is a project-sized design. The disposable experiment at `prototypes/qt-session/` is evidence for selected behaviors, not the implementation to extend into production. This document supersedes that experiment's earlier rule that every unkeyed timeline value is independent: the user has since specified per-property sync, enabled by default.

## 1. Intent, goals, and limits

Build an independent application using C++ and Qt throughout, with a continuously running session that CLI, MCP, a debugging client, and eventually a Qt Quick GUI can all control. One session owns one active project containing multiple independent scenes. Rendering continues when a client disconnects or external output is disabled.

The original, newer C# runtime is the behavioral reference for sources, transforms, effects, composition, clocks, cameras, and output. The user explicitly replaces project compatibility, flat folder semantics, and the live/timeline configuration model. OpenOwlCLI owns its own versioned format and stable identities.

Required core capabilities are layers, nested folders with effects, the source/effect execution needed to render useful projects, Spout output, per-scene timelines, independent live/timeline activation, multiple scenes, and Studio scene transitions. Production GUI authoring and its detailed layout come later. A debug client must expose the same commands and state that other clients use.

Anti-goals: business logic in QML; a renderer driven by GUI repaint; parallel copies of a scene's structure for each setup; exposing mutable engine objects to remote clients; decoding PNG previews in the normal frame path; presenting boot-layer shims as implemented native integrations; claiming shader or application equivalence from the small CPU experiment.

The initial platform proposal is Windows x64, Qt 6.10.2/MSVC, and D3D11. This builds on the installed stack, recovered shader evidence, and Spout experiments. Cross-platform rendering is outside the first implementation. Qt Quick remains the GUI framework; using D3D11 for its native texture integration does not put the engine under GUI ownership.

## 2. Approaches considered and selected

| Approach | Benefit | Cost and outcome |
| --- | --- | --- |
| Framework-independent C++ core with Qt at the edges | A core independent of a GUI framework | More custom lifecycle and adaptation infrastructure. Considered in conversation; the user chose Qt throughout instead. |
| Qt-based application core and Qt Quick clients | Qt supplies process/event infrastructure, threading, networking, metadata, and presentation models | Qt ownership and thread rules become part of implementation discipline. **Selected by the user.** |

Within the selected approach, the proposed process split is a persistent Qt Core session host plus independent clients. The prototype exercised this split successfully. Native renderer resources stay in the host; the GUI consumes GPU previews and document changes. The host can use Qt value types without making every property a QObject.

No implementation plan or production scaffolding is authorized by this proposed document. After approval, the next artifact is a phased implementation plan.

## 3. Confirmed product behavior

- Every scene owns its layers, folders, effects, timeline cursor, playback state, and activation. Scene A's editing, seeking, playback, or activation never changes Scene B's corresponding state.
- Each scene has one root folder. All other nodes descend from it. Folders can contain layers and folders, and have effects on their combined contents. They are ordinary scene containers; they have no separate composition assets, transport, or After Effects collapse/un-precompose semantics.
- Live and timeline setups share node identities, hierarchy, source identities, effect identities, and effect ordering. Their property values can differ. Structural edits affect both setups.
- A client selects an **editing setup**. The inspector shows one value per property for that setup. This selection is independent of output activation. A user may edit or play Timeline setup while the scene remains on live output.
- Unkeyed properties sync by default. Editing either setup writes both values while sync is on. Turning sync on adopts the live value. Keyframing disables and locks sync off. Removing the last key restores sync and overwrites the timeline static value with the live value.
- A keyed property edited in Timeline setup inserts or updates a key at the cursor. An unkeyed property edits its static value, subject to sync.
- Output starts from live configuration. Timeline activation smoothly interpolates property configurations toward the timeline evaluation at the current cursor. The target follows playback and scrubbing throughout activation. Activation is reversible and independent of playback.
- Studio can inspect multiple scenes and independently control each scene's setup activation and playback. A scene transition changes Program between scene renders; it does not restart or activate their timelines.
- Canvas, external output configuration, and eventual recording configuration belong to the project/session level. Spout dimensions are distinct from canvas dimensions.

**Sync consequence:** Timeline editing is independent of the output selector, but synced unkeyed edits deliberately update live values and can therefore change live Program. Keyed and unsynced edits remain separate. The inspector must explain the sync state next to each property.

## 4. Domain data and identities

Use Qt value types and registered typed records internally. Use QVariant/QMetaType at Qt interfaces and JSON at the wire/storage boundaries; an untyped QVariantMap is not the authoritative project model.

| Record | Owns |
| --- | --- |
| `ProjectDocument` | Format version, project ID, global settings, ordered scene IDs, scene records, asset catalog, preferred initial Program scene |
| `SceneDocument` | Scene ID/name, immutable root ID, node records, timeline specification, camera-director specification |
| `FolderNode` | Node ID/name, ordered child IDs, visibility/opacity properties, ordered effect-instance IDs; no coordinate transform |
| `LayerNode` | Node ID/name, source specification, transform/visibility/opacity/blend/role/crop/deformation/mask properties, ordered effect IDs |
| `EffectInstance` | A unique instance ID, effect type ID/version, typed property bindings, auxiliary asset/node references |
| `PropertyDescriptor` | Stable property key, type/dimensions, default/range, keyability, interpolation rule, display metadata, scope/capability |
| `PropertyBinding` | Live value, timeline static value, optional keyframe track, effective unkeyed-sync flag |
| `KeyframeTrack` | Stable track ID, property address, sorted unique keys and interpolation/tangent data |
| `AssetRecord` | Stable asset ID, content identity, original name, media type, managed/external location |

Use UUIDs for document entities. Names are labels, not identity. A property address identifies project, scene, node, optional effect instance, and property key. Effect type IDs are distinct from instance IDs, so repeated instances of one effect have independent values, tracks, and history.

Each scene has a strict tree: one root, one parent for every other node, no cycles, no child in two folders, and no cross-scene parenting. Store child IDs in front-to-back display order and composite them back to front. Folders do not introduce a coordinate system: reparenting leaves authored layer transforms, both setups, sync flags, and all animation tracks unchanged. Moving into a folder can deliberately change the picture through that folder's opacity/effects and stacking context, but does not require pose conversion or animation baking. Structural operations maintain effect/mask/node references atomically and reject unhandled dangling references. Inherited folder transforms would be a separate future design decision.

Asset bytes and immutable capture frames may be shared as resources. Editing or transporting one scene does not mutate another scene's content, media playhead, or effect history. A reused media file does not imply a shared playback clock. Sharing a live input frame is permitted when its source signature matches; independent layer properties still apply.

Source kind and identity are shared structure in the first model: replacing a layer's source changes both setups. Source controls identified as setup-scoped by their descriptors use property bindings. Arbitrary different source identities per setup are not assumed; different content can be put in separate scenes.

### Property state machine

The following applies equally to layer, folder, and effect properties:

| Current state | Operation | Result |
| --- | --- | --- |
| Unkeyed, sync on | Set value in either setup | Both live and timeline static values become the supplied value |
| Unkeyed, sync off | Set value | Only the selected setup changes |
| Unkeyed | Enable sync | Timeline static value becomes live; sync on |
| Unkeyed | Disable sync | Keep both current values; subsequent edits are independent |
| Unkeyed | Add first key | Use the current timeline evaluation unless an explicit value is supplied; disable sync and add key |
| Keyed | Set value in Timeline setup | Upsert at the command/gesture's key time |
| Keyed | Set value in Live setup | Change live value only |
| Keyed | Enable sync | Reject with `PropertyIsKeyed`; UI toggle is disabled |
| Keyed | Remove a key, keys remain | Keep animation and sync off |
| Keyed | Remove last key or clear track | Remove track, set timeline static value to live, enable sync |

Deleting the final key is one undoable document operation including the track removal, sync change, and timeline value replacement. Undo restores the entire previous binding.

A GUI drag is one edit gesture and one undo step. For an animated Timeline property, its key time is captured at gesture start; subsequent drag updates overwrite that key. Playback is not implicitly paused. CLI/MCP may provide an explicit key time; otherwise command acceptance captures the current scene cursor. Invalid non-keyable/type-mismatched edits fail before mutation.

## 5. Evaluation, clocks, and transitions

### Separate document, transport, and view state

`ProjectDocument` is durable and undoable. `SessionTransport` is runtime state: scene cursor anchors, playing flags, activation transitions, Program selection, and scene transition. `ClientViewState` is client-local selection, editing setup, inspector expansion, and Studio layout. A client disconnect changes neither document nor transport.

Use a monotonic session clock. Canonical timeline positions are signed 64-bit microseconds; frame rates are rational numerator/denominator records. Wire clients may use decimal seconds, converted once with a documented rounding rule. Key identity uses canonical time, rather than approximate floating-point equality.

For each scene and render timestamp:

1. Resolve its cursor from that scene's play/pause/seek anchors and loop policy.
2. Resolve timeline properties from tracks, or static values for unkeyed properties.
3. Evaluate its activation weight on session time.
4. Interpolate live and current timeline values into one resolved scene configuration.
5. Prepare sources, role/depth/mask inputs, cameras, and effects; render the resolved tree.

Layer/effect timelines retain the original cycle, ping-pong, and once policies. The camera director is a distinct evaluator with the original manual/timeline/random and seconds/beats concepts, grounded in `CameraRigModel`, `CameraRigClock`, and `CameraRigEval`. Its transport is per scene and is not silently made identical to the layer-property timeline. Camera controls and headless evaluation are core features; the complex camera editor is future GUI work.

Camera and shot IDs are shared scene structure. Camera pose, lens, depth assignment, and configurable director/shot fields use the same live/timeline bindings and sync rules as other properties. Keyability is descriptor-specific; a structural list or identity is not a keyframe value. Property addresses can include a stable camera or shot ID below the root director/effect instance. Editing a camera in Timeline setup therefore changes Live only when that property is unkeyed and synced.

The director has a common per-scene operational clock/manual-take state. Evaluate each setup's rig configuration against that clock, then combine the resulting camera frames using a specialized camera interpolator grounded in `CameraRigMix`/`CameraRigFrame`; do not numerically blend shot IDs or schedules. `camera.take(scene, cameraId)` is an explicit runtime operation independent of inspector selection and property-timeline activation. It changes that scene's manual/override selection using the reference director's transition and scheduling precedence, while poses and lens values still come from the respective configurations. Preview-only camera selection stays client view state and does not issue a camera take.

**Proposed clock default:** intrinsic source playback and time-driven shader/CPU callbacks continue on their declared running clocks, as in the reference runtime. A paused property timeline freezes keyframe evaluation, not every video or time-driven effect. Timeline-bound media playback is an explicit source clock binding, not an accidental consequence of opening the timeline inspector. Deterministic offline/reference evaluation supplies explicit clock overrides and reset policy through the same evaluation interface.

### Activation within a scene

Store current transition origin, target weight, start timestamp, duration, and easing. Reversing evaluates the current weight at command acceptance and starts from it; no jump to an endpoint. Zero-duration activation is an explicit immediate operation.

For continuous values, `resolved = interpolate(live, timeline(cursor(now)), weight(now))`. Activation does not copy values into either saved setup. The default easing proposal is smoothstep, with linear available.

Interpolate ordinary float/vector components directly, angles through the reference shortest-angle rule, and RGBA components according to the reference `SceneMixer` behavior before shader color conversion. Discrete enums, bools, integer effect modes, and references use hold/switch behavior at the midpoint, matching the reference's `late` selection. Integer geometric values use the reference's rounding where applicable. Metadata can specify a specialized interpolator; incompatible data never gets numerically blended.

Enabled/disabled effects use the reference `CanFade`/mix behavior where valid: a one-sided active effect fades its contribution rather than merely switching an enabled bit. Effects that cannot fade use an explicit discrete transition rule. This is not a crossfade between two full scene images.

Live, timeline-edit preview, and resolved Program evaluations have separate stateful-effect contexts. Previewing TV afterglow or particles must not modify the history used by Program. Context identity includes scene, effect instance, evaluation purpose, and resource generation. Size changes, incompatible variants, explicit reset, and backward movement of a bound effect clock have defined invalidation; transient UI selection has none.

### Scene transitions in Studio

Program is the output of a scene mixer, not an alias for whichever scene a client selects. Initial required transitions are **Cut and Fade**, following the user's OBS-style workflow. Both endpoints are the scenes' current resolved renders, so their own playback and activation can continue during the transition. Taking a scene does not seek, play, pause, or activate it.

Use a separate transition record with source, destination, start, duration, and easing. Mid-transition retargeting starts from a retained GPU snapshot of the currently mixed Program picture toward the new destination, avoiding a visual jump. Taking the current destination is idempotent. Cancellation freezes the current mix as the transition origin; an explicit completion or new take resolves it.

The original Blur/Key/Video transition behavior remains a source-grounded extension of the transition module. It is not silently substituted for Cut/Fade. Original linked-layer morphing is not required between these independently owned scenes. The implementation plan must distinguish a working Cut/Fade baseline from completing the additional reference styles.

Off-air scenes retain independent transport and, when playing, continue to advance. **The initial continuity policy renders every scene's resolved context once per engine frame, including off-air scenes.** Its stateful effects therefore see the same frame sequence they would see if that scene were on Program. Studio may present fewer thumbnails, and extra editing-preview contexts can be demand-driven, but presentation budgeting must not silently skip resolved scene history updates. Returning to a scene does not restart it by default. Explicit restart-on-take is a separate per-scene policy/command.

This continuity policy has a real GPU/resource cost. The implementation plan must measure supported scene/effect budgets and report overload; it must not claim unchanged stateful output after silently dropping off-air history updates. Suspending scenes or effect-specific temporal catch-up is future explicit policy, not a hidden optimization.

## 6. Qt modules and ownership

| Module | Small external interface | Hidden implementation |
| --- | --- | --- |
| Session | Connect, inspect capabilities/state, execute commands, subscribe, stop | Active project lifetime, serialization of edits, jobs and runtime anchors |
| Document editing | Execute typed edit transactions; undo/redo | Validation, property sync/key rules, tree/reference invariants, change sets |
| Evaluation | Evaluate a scene at an explicit frame context | Cursor/curve/activation/camera resolution and typed interpolation |
| Rendering | Submit evaluated configuration; request GPU frame streams | D3D11 device/context, compositing, effect execution, masks, history and pools |
| Sources | Acquire/release a source session; obtain timestamped frame/audio leases | Decoders, capture, orientation/alpha metadata, detail requests and errors |
| Output | Configure/enable a sink; report status | Scaling/conversion, Spout sender registration/resources and future encoders |
| Storage | Validate/load/save a project | Version migration, asset resolution, atomic manifest replacement |
| Client models | Apply change sets to Qt models; send commands | QAbstractItemModel/QObject facades and client-local view state |

The host's Qt control thread is the single writer of document state and transport anchors. It processes requests from all clients in one total order. A dedicated Qt render worker owns the D3D11 immediate context, render resources, effect runtime state, and output submission. Decoder/capture workers publish bounded immutable frame leases; they never mutate the document.

Control commits an immutable evaluation snapshot containing document revision and transport version. Rendering adopts it at a frame boundary. Control never waits synchronously for a render-thread callback; jobs/waits complete asynchronously. Render telemetry reports which revision has actually been submitted/completed, distinct from command acceptance.

Qt Quick models live on the client GUI thread. Apply queued document deltas with insert/remove/move/data-change notifications; do not reset the whole model for frame updates. [Qt's model thread-safety contract](https://doc.qt.io/qt-6/qabstractitemmodel.html) requires model APIs to run on their owner thread. GPU work used by Qt Quick runs on its scene-graph render thread.

For the first native backend, renderer algorithms and CPU-driven uniforms remain based on the recovered C# callers and recovered HLSL. Qt supplies the application infrastructure and GUI. There is no Unity runtime dependency in the shipped application.

## 7. GPU composition, preview, and global sizing

### Rendering order and source fidelity

Resolve children inside out. A layer evaluates its source, source-space operations/mask/effects, transform, and composition behavior according to the applicable reference path. A folder composites its ordered children to an intermediate, executes its own ordered image-effect stack top to bottom, and passes its result outward. The root is the final scene-wide image-effect container.

Do not silently sort the authored image-effect stack using Owl's old numeric `Stage` order. That is an explicit change requested by the user. Preserve stage/camera/crop/injection metadata as execution dependencies and eligibility rules. Special effects that modify cameras, inject layer processing, or need role/depth inputs must be represented as such in the render plan, rather than pretending they are simple fullscreen pixel passes.

Folder scope is declared per effect type. Owl's `Both` layer/screen flag does not prove a newly introduced folder scope works. The effect catalog records Layer/Folder/Root eligibility, required auxiliary inputs, padding, passes, no-op behavior, supported mixing, CPU callbacks, and history/reset behavior. Unsupported combinations return a capability error and remain visibly unsupported; they never silently become pass-through.

Preserve the reference's work precision (`ARGBHalf`), sampler/render state, premultiplication, content bounds, pixel scaling, role masks, excluded layers, and stage overscan where the applicable shader path requires them. Color/alpha/orientation are explicit frame metadata. Diagnostic CPU images are not the production renderer.

Reference coordinate mapping comes from `Units.cs`: centered stage coordinates, 100 pixels per world unit, a 1080 reference height, and its fit/fill/pixel-scale calculations. Encode transform units explicitly. Output resizing does not rewrite authored transform values.

### Separate control delivery from frame delivery

Local typed control messages run over Qt local IPC. Change events and transport telemetry carry IDs, revisions, values, and diagnostics—not image bytes. Frame streams publish a latest GPU-frame descriptor: stream ID, frame ID/timestamp, document revision, dimensions, format/color/alpha metadata, adapter identity, resource generation, and a shareable native resource token.

Use a bounded GPU mailbox. The producer publishes completed frames; slow clients skip old frames and retain their last valid picture. A preview client must never block the engine or accumulate a frame queue. Resource reuse, GPU completion, resize generations, and client disconnect are explicit lease/synchronization contracts. An old generation remains valid until its consumer access completes; resize does not replace it with a blank image.

For Windows, open shared resources on the Qt Quick D3D11 device and transfer/copy into a UI-owned texture when needed to keep the producer independent. Wrap the client-owned texture on the scene-graph rendering thread. [Qt's native texture interface](https://doc.qt.io/qt-6/qnativeinterface-qsgd3d11texture.html) does not own the underlying D3D11 texture and requires render-thread use, so wrapper and native-resource lifetimes are managed separately.

The exact shared-handle/fence/keyed-mutex mechanics are an implementation task with an early native interop proof. Required semantics are: no cross-thread immediate-context access, no sampling while a producer overwrites a resource, no unbounded GPU waits, no frame-ID regression, and no empty preview while a new frame is late. Incompatible adapters or device loss produce a clear status and a recoverable new resource generation.

PNG/base64 is reserved for explicit screenshots and diagnostics. The existing spike's 10 Hz PNG path explains a limitation of that experiment and is not evidence against Qt's GPU rendering capability. A GPU preview proof should precede investment in a larger editor.

### Project settings

`CanvasSettings`: width, height, background, scaling policy. Proposed default: 1920×1080, transparent background, matching the reference's initial canvas.

`OutputSettings`: Program frame rate, Spout enabled/name, output width/height or inherit-canvas, fit/fill/stretch policy, and alpha policy. Proposed default: 60/1 frame rate, inherit canvas dimensions, contain scaling, and explicit alpha preservation. Preview output is independent of the external sink's enable switch.

Resizing canvas reconfigures scene render targets, auxiliary buffers, and histories on the render thread. Resizing Spout alone reconfigures its final scaling/conversion surface and sender resources without changing canvas geometry, timelines, or playback. Allocate/validate the replacement before switching generations; failure retains the previous valid configuration and reports the error. Validate adapter limits and memory requirements, rather than silently clamping requested sizes.

`RecordingSettings` is a project-level future module containing destination/container/codec, dimensions/rate, quality and audio policy when implemented. Declare recording capability unavailable until it works. Do not expose working-looking record controls or persist a speculative encoder-specific schema as final. Recording consumes Program through a bounded sink interface with its own backpressure policy.

## 8. Sources, effects, and reference boundaries

`ILayerSource` is the reference for frame availability, size, dynamic/static status, premultiplication, vertical orientation, detail, visibility, and error reporting. Separate `SourceSpec` from a runtime `SourceSession`; input switching has a preparation/result state rather than blocking a render frame.

Read `SourcePool` pooling rules before sharing decoder/input objects. Share immutable decoded frames and appropriate live inputs; keep independently transported media sessions separate. Source lifecycle follows the continuously evaluated scenes and any additional subscribed previews—not which inspector is selected. Render/decoder budgets and timestamped frame reuse are explicit; a resource optimization must preserve the chosen scene continuity contract.

Source capabilities are cataloged from `SourceKind`: None, SolidColor, Image, Video, Spout, Text, Camera, Window, GameCapture, Browser, NDI, and MiniApp. The first usable core needs None/SolidColor/Image/Video/Spout input plus Spout output. Remaining source adapters are staged, and report unavailable until implemented. Local asset/media abstractions in `Owl.Cloud` remain relevant even though online account/shop features are outside this project.

**Initial audio scope proposal:** the first usable video/Spout core is visual-only, with media audio muted and no audible playback/mixer or recording audio path. Scene fades in that capability gate therefore mix pictures only. Audio settings must not look functional before an audio capability exists. Audible media, Program/off-air routing, crossfade curves, and the audio equivalent of transition retarget/cancel require a separate audio design before that capability is implemented; `AudioPlan`/`AudioFader` alone do not supply such a scene mixer. This deferral does not remove the project-level place for future recording settings.

Every effect port includes its registry metadata, CPU callback, required render graph, parameter bindings, assets, and history behavior—not only its pixel shader. Weather, CameraFlash, ImageParticles, StageLights, TransmitLight, and TV demonstrate why this matters. Continuous formulas replace bounded replay fixtures in production.

Import a provenance-tracked snapshot of canonical recovered shader sources and caller-derived metadata into this repository. Generated standalone HLSL adapters are build outputs; shipped DXBC remains test reference only. Production executes locally compiled recovered HLSL. Keep original/recovered bytecode, callback traces, and source hashes in differential evidence; do not assume old Unity audit results cover later source changes.

## 9. Storage, history, and loading

Proposed first format: a project directory with a human-readable `project.openowl.json` manifest and managed `assets/` content-addressed media. External references are explicit asset records; importing can choose managed copy or external reference. A portable packed export is later work, rather than a dependency on a private Qt ZIP implementation.

The manifest includes schema/version, stable IDs, settings, ordered scenes/nodes/effects, typed property bindings, tracks, camera specifications, and asset references. It excludes GUI selection, live GPU/decoder handles, active transitions, frame counters, and intrinsic effect history. Unknown effect payloads are preserved for round trips and marked unavailable. A newer unsupported schema is rejected without overwriting it; recognized older schemas migrate into a new in-memory document before save.

Loading is a job: parse, migrate, validate all tree/reference/property invariants, resolve assets, prepare required resources, and atomically replace the active project at a frame boundary. Failure leaves the current project running. References carry a project generation so delayed responses from the old project cannot mutate the new one.

Save assets first and atomically replace the manifest last using Qt's save facilities. A failed save preserves the prior manifest. Missing media is represented explicitly and reported; the chosen placeholder is visible and deterministic.

Proposed startup policy: load the preferred Program scene with live activation, property timelines paused at zero, and external outputs off until explicitly enabled. Saved configuration is distinct from resuming a running session. Future crash recovery may restore document edits, without auto-resuming outputs.

Document edits, sync/keyframe changes, structure operations, and settings are transactional and undoable. Coalesce GUI gestures by an explicit gesture ID, not a timing guess. Transport commands such as take/play/seek/output-enable are separate from document undo. Undo restores authored data; it does not promise to rewind external captures or GPU history.

## 10. Core interface, CLI, MCP, and events

The session has one command catalog with typed request/result schemas, validation, change semantics, and capability requirements. CLI, MCP, and Qt clients adapt it; none duplicates sync/keyframe/scene logic or calls arbitrary QObject methods by name.

Conceptual interface:

```text
connect(sessionId) -> session identity, protocol version, capabilities
inspect(query) -> document snapshot / runtime snapshot / descriptor catalog
execute(command, expectedDocumentRevision?, gestureId?, idempotencyKey?)
    -> result, documentRevision, transportVersion, optional jobId
subscribe(topics, afterSequence?) -> ordered changes and bounded telemetry
subscribeFrames(sceneId, purpose, budget) -> GPU frame stream
wait(jobId or renderedRevision) -> asynchronous result
```

Queries never depend on a client's selected node or setup. Commands identify scene/node/effect and editing setup explicitly; a GUI supplies those from its local view state. Convenience name lookup must fail on ambiguity and return stable IDs.

| Domain | Required command families |
| --- | --- |
| Session/project | Start/connect/list/status/stop, create/open/save/validate, capabilities and global settings |
| Scenes | List/create/duplicate/rename/remove, inspect, preview/Program take, transition status/cancel |
| Structure/sources | Tree inspect, layer/folder add/move/reorder/remove, source assign/status, input discovery |
| Properties | Describe/get/set, sync enable/disable, begin/update/commit/cancel gesture |
| Animation | Key upsert/remove/clear, curve edit, seek/play/pause/loop, activation target/duration/easing |
| Effects/cameras | Catalog/describe, effect add/remove/reorder/reset, parameter operations; camera/director catalog and transport |
| Output/history/jobs | Spout configure/enable/status, undo/redo, job inspect/cancel, explicit screenshot/diagnostics |

Wire envelope: protocol version, request ID, operation, typed arguments, optional revision/gesture/idempotency fields. Result separates validation, document revision committed, job completion, and frame rendered. A batch validates fully before one atomic commit. Invalid requests cause no partial mutation.

Use stable errors such as `InvalidArgument`, `NotFound`, `AmbiguousName`, `PropertyIsKeyed`, `UnsupportedCapability`, `RevisionConflict`, `ResourceUnavailable`, and `ProjectChanged`. Error details include offending IDs/fields and corrective information.

Events have monotonic sequence numbers and project generation. Document deltas are reliable and revisioned; transport/frame telemetry is coalescible. A subscriber with a gap obtains a fresh snapshot. Client disconnect cancels only client-owned waiting/subscription work, not accepted edits, shared session jobs, rendering, or output. Bounded queues prevent a slow client from blocking the host.

Suggested CLI shape is `openowl <domain> <verb>` with explicit IDs, `--setup live|timeline`, JSON input/output, and nonzero exit status on failure. A separate host process owns the session. The executable naming is left to implementation.

For MCP, use the same catalog to expose purposeful tools and introspection schemas rather than only the prototype's generic `session_command`. Return structured results with an output schema and actionable `isError` tool results; malformed protocol requests remain protocol errors. A stdio bridge can attach to the persistent local session, so its client-controlled lifetime does not own the engine. Protocol negotiation targets versions supported by the actual client; the specification must not be blindly hardcoded to the latest revision. [MCP tool result contract](https://modelcontextprotocol.io/specification/2026-07-28/server/tools).

## 11. Reference map and evidence boundaries

The primary source is `D:/Dev/Owl/OwlUnity/Assets/Scripts/OwlRuntime/`, the newer repaired decompilation. The older installation and boot stubs are not interchangeable with it.

| Concern | Reference files under that root |
| --- | --- |
| Document and sizing | `Owl/ProjectModel.cs`, `SceneModel.cs`, `LayerModel.cs`, `SourceModel.cs`, `CanvasModel.cs`, `OutputModel.cs`, `Units.cs` |
| Editing/storage | `LayerOps.cs`, `SceneOps.cs`, `EffectOps.cs`, `KeyframeOps.cs`, `UndoHistory.cs`, `ProjectSerializer.cs`, `IProjectBackend.cs` |
| Rendering | `Composition.cs`, `LayerManager.cs`, `LayerView.cs`, `StageRig.cs`, `OutputCompositor.cs`, `RoleMask.cs`, `ExemptLayers.cs` |
| Effects | `EffectDef.cs`, `ParamDef.cs`, `EffectRegistry.cs`, `EffectChain.cs`, `EffectRuntime.cs`, `EffectContext.cs` |
| Clocks/cameras | `LayerTimeline.cs`, `LayerMotionClock.cs`, `LayerMotionEval.cs`, `RigTrack.cs`, `RigKey.cs`, `RigCurves.cs`, `CameraRigModel.cs`, `CameraRigClock.cs`, `CameraRigEval.cs` |
| Scene transitions | `SceneMixer.cs`, `TransitionDriver.cs`, `TransitionSettings.cs` |
| Sources/output | `ILayerSource.cs`, `SourcePool.cs`, `LayerSourceFactory.cs`, `InputSources.cs`, source-specific files, `SpoutOutput.cs`, `AudioPlan.cs`, `AudioFader.cs` |
| Original GUI/MCP coupling | `Owl.UI/StudioView.cs`, `InspectorPanel.cs`, `EffectPanel.cs`; `Owl.Mcp/ToolContext.cs`, `McpHost.cs`, `OwlTools.cs` |

Canonical recovered shaders are under `OwlUnity/Assets/Resources/Shaders/`. `OwlUnity/HANDOFF.md`, `SHADER-AUDIT.md`, `SHADERS.md`, and the native shader viewer README define their current evidence and limits.

The Qt session spike verifies selected state semantics and a bounded CPU-to-D3D11-to-Spout path. It does not prove a production GPU compositor or flicker-free GPU preview. Its old sync behavior and paired-value inspector are superseded by this design; no prototype code was changed for the user's latest requirements.

## 12. Delivery boundaries and acceptance

These are capability gates for the later implementation plan, not a schedule:

1. Establish typed document/editing/evaluation contracts, project format, sync/keyframe invariants, history, CLI/MCP parity, and independent scene transports.
2. Prove headless native GPU rendering and the Qt Quick shared-GPU preview handoff. Include resize, client close/reconnect, busy client, and device-generation handling before building a larger editor.
3. Deliver a usable GPU vertical slice with solid/image/video/Spout sources, folder composition, representative recovered effects, independent canvas/output sizing, and a real external Spout receiver.
4. Complete Studio Cut/Fade, independent per-scene controls, camera/director evaluation, remaining effect families and original transition styles with source-grounded evidence. Track unsupported source adapters explicitly rather than implying full parity.
5. Expand the production Qt Quick editor and additional source/output/recording adapters against the established interfaces. Recording and optional native adapters have their own capability gates.

Required acceptance cases include:

- Editing Timeline while Program is live, both with sync on (live deliberately changes) and off/keyed (live remains independent).
- Adding a first key, editing at a different cursor, overwriting an existing key, removing one of several keys, and removing the last key with live-value sync restoration; equivalent cases for effects/folders.
- Independent scene play/pause/seek/activation, including two off-air scenes; scene takes during activation/playback and activation changes during a scene fade.
- Activation with a paused cursor, moving targets, reversal, discrete properties, enabled-effect contributions, and independent effect histories.
- Folder nesting, reorder, reparent, group effects, root effects, mask/reference handling, and rejected cycles.
- Continuous rendering with every UI closed and every external output disabled; output re-enable preserves configured sender identity.
- Canvas resize and Spout-only resize, alpha/orientation, failure preserving a valid prior output, and repeated sender reconnects.
- GPU preview frame IDs never regress, slow consumers retain a valid frame, no image encoding/readback in the normal path, and no control-model resets from frame telemetry.
- Save/load round trip, missing/unknown assets/effects, invalid/newer formats, failed atomic save/load, undo of sync/track operations, multi-client conflicts, and stale project generations.
- Differential shader and callback checks against the existing references, including long timestamps, reset/seek behavior, changing inputs, mask/depth/alpha, and feedback isolation.

The initial performance test proposal is 1920×1080 at 60 Hz on the available reference GPU, using a declared fixed scene and actual GPU timestamps. Measure engine rate, Qt presentation rate, output rate, frame age, and queue depth separately. A target for the simple GPU-preview proof is no visible blanking and p95 frame age within three engine frames; this is an acceptance target to measure, not an established performance claim.

## 13. Calls made in this proposal and builder discretion

Assistant proposals requiring review with this document: Windows/D3D11 first; one active project per session; simple folders without inherited coordinate transforms; shared source identity across setups; continuous resolved rendering of off-air scenes; camera configuration following the same sync rules; intrinsic media/effect clocks not automatically paused by the property timeline; Cut/Fade before the additional original transition styles; a visual-only first usable core with audio designed later; source-coordinate conventions retained; JSON-directory storage first; startup live/paused/output-off; and a managed/external asset choice on import.

The builder may choose concrete Qt record classes, class/file names, the exact local IPC framing, CMake organization, JSON schema tooling, GPU synchronization primitive satisfying the stated contract, pooling policies within declared budgets, and the order of individual adapter/effect ports within the capability gates. Those choices must not change user-facing sync, editing/output separation, independent scene behavior, clock ownership, or evidence claims.

The detailed production GUI appearance is deliberately unapproved. The only current layout requirement is one setup-aware property value per row, property sync next to the property, and effect controls alongside layer controls. Use the debug UI only when an unresolved interaction or native interop question benefits from a runnable experiment.
