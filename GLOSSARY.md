# OpenOwlCLI

Terms confirmed during the design of the independent application. Definitions describe user-facing concepts; unresolved behavior is left out.

## Language

**Session**:
A continuously running application instance that renders while commands operate on it. Rendering continues when external output is disabled.

**Output**:
The publication of rendered frames to an external destination, such as Spout. Output can be enabled or disabled within a running session.

**Scene**:
An independent composition with its own hierarchy of layers and folders, effect stacks, live and timeline configurations, cursor, playback, and timeline activation. A project can contain multiple scenes; each uses its live configuration for output by default.

**Folder**:
An ordered container of layers and nested folders, with an effect stack applied to their combined image. Its contents are resolved before its effects are applied.
_Avoid_: Precomp, precomposition

**Root folder**:
The automatically created top-level folder under which a scene's content is arranged by default. It can have effects like other folders.

**Effect stack**:
The ordered effects attached to a layer or folder.

**Live configuration**:
The saved property values of a scene's layers, folders, and effects for its live setup. An unkeyed property can share its value with the timeline configuration through property sync.

**Timeline configuration**:
The saved static property values and optional keyframes of the same scene's layers, folders, and effects for its timeline setup. An animated property uses its evaluated keyframes; an unanimated property uses its static value, which can be synced with live.

**Timeline cursor**:
The current timeline position used to evaluate layer and effect properties in the timeline configuration.

**Timeline activation**:
The control that smoothly transitions a scene from its live configuration to the timeline configuration evaluated at the cursor. Activation is independent of timeline playback, and its timeline target continues to follow the cursor during playback or scrubbing.

**Timeline playback**:
The advancement of the timeline cursor over time. Starting or pausing playback is independent of timeline activation.

**Editing setup**:
The live or timeline configuration selected for inspection and editing by a client. This selection is independent of the scene's activation and the project's output scene.

**Property sync**:
The sharing of an unkeyed property's value between live and timeline configurations, enabled by default. Keyframes disable sync; removing all keys restores sync using the live value, and manually enabling sync also adopts the live value.

**Program**:
The rendered picture selected for external output, including any scene transition in progress.

**Studio**:
The view and controls for inspecting multiple scenes, managing each scene's playback and activation, and taking a scene to Program through a scene transition.

**Scene transition**:
The change from one scene's rendered output to another in Program. It is independent of each scene's timeline activation and playback.

**Canvas**:
The project's composition dimensions and background, shared by its scenes. External output dimensions are configured separately.
