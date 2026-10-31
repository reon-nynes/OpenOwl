# OpenOwlCLI

Terms confirmed during the design of the independent application. Definitions describe user-facing concepts; unresolved behavior is left out.

## Language

**Session**:
A continuously running application instance that renders while commands operate on it. Rendering continues when external output is disabled.

**Output**:
The publication of rendered frames to an external destination, such as Spout. Output can be enabled or disabled within a running session.

**Scene**:
A composition with a shared hierarchy of layers and folders, shared effect stacks, and distinct live and timeline configurations. Its rendered output uses the live configuration by default.

**Folder**:
An ordered container of layers and nested folders, with an effect stack applied to their combined image. Its contents are resolved before its effects are applied.
_Avoid_: Precomp, precomposition

**Root folder**:
The automatically created top-level folder under which a scene's content is arranged by default. It can have effects like other folders.

**Effect stack**:
The ordered effects attached to a layer or folder.

**Live configuration**:
The saved property values of a scene's layers, folders, and effects for its live setup. Changes to these values do not change the timeline configuration's values.

**Timeline configuration**:
The independent saved static property values and optional keyframes of the same scene's layers, folders, and effects for its timeline setup. At the timeline cursor, an animated property uses its evaluated keyframes and an unanimated property uses its saved timeline value.

**Timeline cursor**:
The current timeline position used to evaluate layer and effect properties in the timeline configuration.

**Timeline activation**:
The control that smoothly transitions a scene from its live configuration to the timeline configuration evaluated at the cursor. Activation is independent of timeline playback, and its timeline target continues to follow the cursor during playback or scrubbing.

**Timeline playback**:
The advancement of the timeline cursor over time. Starting or pausing playback is independent of timeline activation.
