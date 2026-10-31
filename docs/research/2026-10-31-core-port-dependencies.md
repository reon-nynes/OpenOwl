# First visual core: recovered C# dependencies and reusable evidence

Research for [OpenOwl #4](https://github.com/reon-nynes/OpenOwl/issues/4), part of [map #1](https://github.com/reon-nynes/OpenOwl/issues/1). Captured 2026-10-31. Research only: no implementation, plan edits, PR, merge, rebuild, or new runtime qualification.

## Answer and scope

The first visual core needs a render dependency graph and explicit runtime identities before a broad effect port. The recovered runtime connects source acquisition, pre-effect mattes, layer preparation, role masks, camera placement, screen effects, and output; copying only `EffectRegistry` and recovered HLSL would omit real behavior. Seven representative effects below cover the principal ordinary execution mechanisms without requiring the whole registry. Source pooling and history must be adapted to the new Scene/setup/evaluation-purpose contracts instead of retaining Unity globals.

This note follows representative callers, not every registered effect or native codec. Source authority is the newer repaired C# under **R** = `D:/Dev/Owl/OwlUnity/Assets/Scripts/OwlRuntime/`; canonical shaders are **S** = `D:/Dev/Owl/OwlUnity/Assets/Resources/Shaders/`. **U** = `D:/Dev/Owl/OwlUnity/`; **V** = `U/prototypes/d3d11-shader-viewer/`; **H** = `D:/Dev/Owl/_re2/harness/`; **N** = `D:/Dev/Owl/OwlNativePrototype/`; **P** = `D:/Dev/Owl/OpenOwlCLI/`. Paths and symbol/line locators below refer to these local artifacts; they are not publicly hosted by this research branch. SHA-256 manifests at the end make the read snapshot identifiable. OwlUnity itself is not a Git repository. V HEAD was `6efdcf547c85b3807df3cb47d6df9c9eeeee1848`; N HEAD was `4818bcfe2a9a175a6be0c738032aafe0fed0bd2a`.

The note's commit parent is OpenOwlCLI baseline `0c7253979762db253fe528a9fee9bd59c7da457f`. The design and draft plans were read from the live working tree, including uncommitted content; their hashes identify that snapshot, not approval provenance. At inspection A and B existed; the index linked C/D files that did not yet exist. Their absence is an observation during ongoing authoring, not a finding against later revisions. No HITL question is decided here. Library API claims are intentionally outside this source-only analysis.

## Bounded dependency/evidence matrix

| Concern | Representative real path and source locator | Dependency/ownership consequence | Reusable evidence and remaining gap |
| --- | --- | --- | --- |
| Frame orchestration | `R/Owl/OwlApp.cs:2483–2525`: transition tick, `Live.Sync`, preview sync, `RenderAux`, then `Spout.Sync(...FinalTexture)`; `Composition.cs:79–122`: camera preparation before `LayerManager.Sync`, assign screen effects, then role/exempt auxiliaries | Preserve logical preparation order; implement an engine scheduler and frame context rather than `MonoBehaviour`/camera callbacks. Source update and output enable are separate lifetimes. | N demonstrates a bounded native composition/output path. Neither that scene nor the CPU Qt session spike proves the new multi-scene scheduler, interprocess GPU preview, or overload policy. |
| Coordinates, layer preparation and composition | `Units.cs:9–92`; `LayerManager.Sync:84–207`; `LayerView.Sync:938–1125`; `OutputCompositor.OnRenderImage:115–226` | Original source-space px/unit, crop, flips, deformation, padding/content UV, detail, premultiplication, blend state and final crop form one contract. Work targets use half-float (`EffectRuntime.WorkFormat/GetTemp`). OutputCompositor orders stage crop/background, screen chain, camera blur, exempt overlay, transition blur and final output. | H has layer/blend/output/mask render-state and pixel fixtures; V supplies pass comparisons with fixture vertices. New folder targets and actual native layer geometry/graph remain to qualify. |
| Folder behavior | `LayerFolder.cs` has only id/name; `LayerFolders.Members/Place/Display`; `LayerManager.Sync` iterates `scene.layers` and never executes a folder effect stack | Recovered folders are grouping over a flat stack. The new nested Folder/root Folder composition is an explicit product departure, not recovered parity. Reparenting retains authored layer coordinates by the design. | Use independently derived expected folder pixels, nesting and noncommutative order cases. Existing shader parity cannot establish this new behavior. |
| Referenced layer masks | `LayerManager.Sync` first gathers split/mask hosts and referenced sources, synchronizes non-hosts then hosts, and retains hidden referenced sources. `LayerView.Sync:1019–1049` assigns `Matte` before `ApplyMask`/fog/padding/effects; `ApplyMask:537–613` uses `Lookup(...).Matte` and `LayerMaskMath` | A layer mask consumes the referenced layer's prepared pre-mask/pre-fog/pre-effect matte plus placement/alpha/orientation, not its final effect output. Image masks separately acquire a pooled source under `mask:` + LinkKey. Preserve auto channel (image luma, layer alpha), invert, feather and clipping. Do not infer arbitrary mask DAG semantics from the original two-pass host ordering. | H controlled LayerMask cases qualify shader arithmetic, not all cross-layer ordering, chains, cycles or cross-Folder relationships. New reference validation/render planning must state supported relationships and reject unsupported dependencies explicitly. `MaskModel.IsActive/Normalize` only bounds direct owner/reference validity; it is not a general dependency solver. |
| Roles, screen exemptions and injection | `Composition.RenderAux` → `RoleMask.Render:34` → `LayerManager.SetMaskMode` → `LayerView.SetMaskMode:1156`; `OutputCompositor:158–175` crops role target into screen space and binds `_OwlObjectMask`; `EffectChain.ResolveLayerEffects` calls `InjectIntoLayer` | Role mask is a separate rendered signal: R=non-background, G=character/tracked layer; it uses drawn texture/opacity and camera placement, half-stage resolution (minimum 16), then screen crop. `ExemptLayers` separately handles layers excluded from screen effects. Role masks, ordinary layer masks and effect injection are distinct contracts. | H RoleMask/ImageParticles fixtures and V behind-mask cases can seed tests, but supplied role fixtures do not prove production mask generation, nested-Folder scope, exempt recomposition or injected-effect placement. `RoleMask.NeedsMask` lists Weather/StageLights/particles and other role consumers; preserve conditional activation. |
| Source lifetime, pooling and generation | `LayerView.Sync:955–990` → `SourcePool.Acquire/Current/Update` → `LayerSourceFactory.Create`; `SourcePool.KeyOf:72`, `Reload`, `Release`; `LayerModel.LinkKey:95` | Pooling is not just asset caching: Spout/NDI/camera use source signature, Window adds client-only/fps, Image/Video/Browser/MiniApp use signature + LinkKey; null keys become unique own entries. Once-per-Unity-frame update gating, refs, aliases, shown/audible aggregation and generation replacement are runtime behavior. Independent media clocks cannot be keyed only by file content. | Source is readable, not an executed adapter oracle. Test same bytes with different Scene playheads, shared live capture with distinct transforms, reload in flight, hidden mask-source use and release/generation boundaries. Replace global static/root/frameCount ownership with Session resources and explicit engine tick identity. |
| Intrinsic media clocks and backends | `LayerSourceFactory` dispatches image-with-video MIME to `VideoFileSource`; `VideoFileSource.Begin/UpdateBackend/TogglePlay/SetPlaying/Restart/Seek`; `ILayerSource` carries size/dynamic/error/alpha/flip/detail | Media owns playback beyond property timeline evaluation. VideoFileSource selects among HAP, QtRle CPU/GPU, Webm/FFmpeg, Unity VideoPlayer and browser paths, with fallbacks/recovery and alpha/layout processing. These are not one portable decoder. | N proves only FFmpeg subprocess CPU BGRA pipe/upload for its clip; proposed Qt Multimedia adapter is a new implementation requiring declared video scope and behavior tests. Muted first-core video is an approved boundary, not evidence for all recovered codecs, browser paths, audio or GPU decode. |
| Effect definition and execution | `EffectDef.SkipsChain/PaddingFor`; `EffectChain.ProcessingOrder/Run`; `EffectContext.ApplyParams`; `EffectRegistry` definitions and callbacks | Definition metadata matters: target/family, defaults, explicit shader property names, integer packing, animated predicate, padding, source replacement/on-source/crop/injection behavior, mix and no-op handling. Ordinary chain sorts by Stage, conditionally taps AutoService, runs callbacks or pass 0, and uses `EffectMix` when not plain. ApplyParams only binds named ParamDef properties plus `_PixelScale`, `_OwlTime`, `_OwlContent`. | H/V reflect exact bindings and shader/pass metadata. Standalone source adapters need include, sampler, integer/array, color-conversion, vertex and fixed render-state provenance. One material pass per registry entry is not a valid general port. |
| Effect time and feedback | `EffectChain.Run` sets ctx.Time=`EffectRuntime.Now`; `EffectRuntime.Now` uses override or Time.time; `Persistent/State/PersistentTarget/TransferState/PurgePersistent`; `EffectRegistry.RenderTv:2957`, `RenderFpsLimit:2982`; `LayerStats.MeasureSmoothed:59` | History is globally keyed by EffectInstance object reference + slot, stale after 120 unused frames; temporary evaluated instances can lose or alias intended continuity. TV runs pass 0 → persistent afterglow → pass 1 → writes output back; FPS hold tracks LastUpdate; stats stores time and 4×1 history with cut/reset rules. New context identity must include Scene/effect instance/purpose/resource generation and explicit effect clock/reset policy. | V already executes independent TV histories and blackout/reset checks, plus shader-clock/CPU traces. New LivePreview/TimelinePreview/Resolved isolation, disable/re-enable, instance deletion/reuse, off-air continuity, size/device changes and backward bound-clock motion still need product tests. Do not copy TransferState between scene endpoints for scene-picture Fade. |
| Property and camera clocks | `LayerManager.Sync` → `LayerMotionClock.Time(scene, EffectRuntime.Now)` → `LayerMotionEval.Shown`; `Composition.PrepareCameraRig` → `CameraRigFrame.Prepare` → `CameraRigEval.EvaluateNow/Program`; `CameraRigClock.Seconds/TakeCut/Cut`; `CameraRigMix` | Property clock state is keyed by scene; camera state by rig id, with manual/timeline/random scheduling, seconds/beats, take override precedence, focus/solo/edit and transitions. `CameraRigFrame.Place` then supplies offset/scale/roll/hidden/blur/fade per layer and camera motion to OutputCompositor. Screen `camera_rig` is evaluator behavior, not an ordinary pixel effect. | Source is available; no read evidence here qualifies the new full camera evaluator or setup mixing. Use source-captured numeric fixtures for schedule/precedence, shortest-angle/projection/depth/lens/DOF and frame results. Audio-reactive camera scheduling belongs to later audio capability unless explicitly implemented. |
| Scene transitions | `OwlApp.StartTransition:2700` calls camera/property OnGoLive; `TransitionDriver.Tick` animates both scenes, `SceneMixer.Mix`, optional overlay, history transfer; `SceneMixer.MorphLayer/MixEffects/Blend/CanFade` | Original scene transition is linked-layer morph/fade with remapping and effect/camera mixing, not generic endpoint image Fade. Original LayerMotionClock.OnGoLive always resets the scene cursor; camera restart is conditional. Blend uses float/color interpolation and midpoint choice for discrete/reference values; CanFade excludes skipped/no-op behavior. | Reuse scalar/interpolation/effect fade rules where applicable, not linked-layer topology or automatic cursor reset for new Studio takes. Cut/Fade of current resolved images and retained Program snapshot on retarget need new GPU tests. Blur/Key/Video extensions require their actual graph/source-warmup dependencies, not a style enum alone. |

## Representative effect set and port order

Recommendation, not a new approval: start with five ordinary-chain effects, then add two dependency stress cases before claiming broad execution-mechanism coverage. Each must be an actual supported capability or explicitly unavailable; diagnostic multiply/add passes are useful tests but are not recovered product effects.

| Effect / phase | Mechanism and concrete dependency | Existing oracle to reuse; additional qualification |
| --- | --- | --- |
| `pixelate` | Plain ParamDef → single pass; `_CellSize`, pixel scale and content handling | V demo/H broad fixture establishes bounded pixel arithmetic. Add actual layer/Folder instances, nonneutral values, repeated-instance identity, mix and authored order. |
| `blur` | `RenderBlur:4270` → BlurUtil horizontal/vertical intermediates; stable style and high-quality repeated-pass branch | N has exact shipped blur comparison for one gradient; V/H pass fixtures are useful. Exercise radius 0/12/above-12, both styles, padding/content edges, pixel scale and full callback graph. |
| `film_grain` | `_OwlTime` shader path with quantized time/noise, no feedback history | V animation cases cover eight style/color/fps variants, long time and reset against shipped DXBC. Use corrected source, not earlier Unity build. Test explicit effect clock independently of paused property timeline. |
| `weather` | `RenderWeather:3096` computes double-based wrapped cell/scroll/drift/smoke/sway inputs; behind variant also requires role mask | V Weather oracle is raw-float exact for captured times. `_OwlTime` alone does not animate it. Qualify all kinds, size/speed/wind, long timestamps and actual camera-cropped role mask. |
| `tv` | `RenderTv:2957` retained afterglow, pass 0/pass 1/feedback write and first-use initialization | V moving-input/blackout/0/.5/.95 afterglow tests are a real native graph reference. Add per-purpose/per-Scene isolation, resize/reset, off-air updates and instance lifecycle. |
| `image_particles` / mechanism stress | `RenderImageParticles:1285` → ParticleAt/private depth/angle/Rand → ParticleSprites atlas → explicit quads and original over/additive/behind blend factors | V contains 10 scenarios using Unity-executed callback traces, native quads/blending and fixture atlas. Port asset/sprite assembly and callback math, not trace replay as production behavior; arbitrary assets/controls and complete original vertex equivalence remain unproved. |
| `levels` with auto on / mechanism stress | `RenderLevels:4240` → `LayerStats.MeasureSmoothed` → AutoColor passes 0/1/2/3 → 4×1 stats → Levels; auto off supplies black stats | H Levels and AutoColor controlled fixtures validate supplied statistics and named textures. Real changing-input stats/cut detection/smoothing/clock reset graph is additional work. Auto off alone does not cover this mechanism. |

These seven deliberately do not qualify source-replacing `layer_split`, on-source `layer_fog`, crop modifiers (`shake`/`camera_zoom`), layer injection or the camera director. Their definitions are skipped by the ordinary chain and need explicit render-plan/evaluator support. For the next expansion, use one injection case (e.g. `led_panel` → `led_panel_layer` via ResolveLayerEffects) as a contract test before a family-wide port. Preserve capability rejection until dependencies exist. TransmitLight's Unity-native Perlin trace is reusable evidence, but does not prove a new native Perlin replacement.

A dependency-first sequence consistent with the live A/B draft is:

1. Document identities/bindings, explicit frame/time/purpose/generation records, source identity versus playback-instance identity, typed descriptors/interpolators and capability errors. Keep the property timeline, intrinsic media/effect clocks, director clock and Studio transition clock distinct.
2. Small native GPU proof: half-float work targets, locally compiled canonical HLSL, explicit render state and coordinate/alpha/orientation contract; then bounded native preview lifetime/resize proof. Read V's extraction adapter, do not transplant its viewer ownership.
3. Inside-out Folder composition with source-faithful layer prep and deliberate new Folder expectations; validate references and advertise only supported dependency kinds. Render every resolved Scene once per engine tick under the design's continuity policy.
4. Real Solid/Image/Video/Spout sources and Spout output with asset/playback/resource generations; five ordinary-chain effects above. Output toggles and preview demand cannot reset media or history.
5. Role/dependency support for Weather behind/particles, statistics and retained-history stress cases; source/camera dependencies can be capability-gated while this work lands. A role-dependent variant must not be advertised merely because a mask-free variant works.
6. Director evaluation/placement and setup camera-frame mixing, then Studio image Cut/Fade/retarget and additional reference transition styles. Expand remaining effects one mechanism-aware review at a time, using existing qualified fixtures and adding missing graph oracles.

This is an ordering recommendation; the map's implementation plan remains the place for final file/test-sized tasks.

## Named design departures versus unresolved port risks

The current plan index already names these departures from recovered behavior: own format; shared structure with setup-aware bindings/sync; nested effect-bearing Folders without inherited coordinates; authored effect order instead of Stage sorting; editing/output separation; continuous off-air resolved rendering; explicit startup defaults; and scene-picture Cut/Fade/retarget rather than linked-layer morphing. This research neither reapproves nor replaces them. The design also states intrinsic media/effect clocks continue when the property timeline pauses, a visual-only muted core, and per-scene common director operational clock with setup-specific camera frames. Preserve these as declared contracts, not inferred Unity behavior.

Concrete plan risks to carry to existing contract/effect-port work:

- Stage B defers role/camera/injection dependencies to D3 and requires capability rejection. Stage C's representative set must therefore select mask-free Weather first or pull role support earlier before advertising behind/particles/role effects. Do not claim all variants from a working pass.
- The source matte contract and screen role signal need explicit scope across new Folder boundaries. Source-faithful layer masks consume Matte, while a Folder is a new combined image. This note identifies the missing cross-Folder/reference contract; it does not select a new product rule.
- New authored order cannot blindly use ProcessingOrder. Retain the distinctions between ordinary image effects, source preprocessing, injection, crop modifiers and director evaluation even when their presentation is an ordered stack. A DAG must reject impossible dependencies rather than silently reorder authored image effects.
- The renderer must not use C# EffectInstance reference identity or process-global shader/mask state as production history identity. Context separation must cover auxiliary analysis and camera-derived blur as well as TV. All-scenes continuity has a real measured-budget gate.
- Video backend substitution needs an explicit supported-format/fallback/alpha/orientation/error/seek contract and a justified deviation if behavior differs. Recovered wrapper names do not establish portable native availability. This source-only note makes no Qt/Spout SDK API recommendation.
- Startup, take and scene-transition rules differ: original OnGoLive reset paths must not accidentally restart independent Scene transport. Camera manual take is also distinct from Studio scene take and preview selection.
- The index's hash/provenance ledger is useful, but `implemented` versus `qualified` must remain separate per shader/callback/graph/integration. Fixture-derived expectations must be labeled separately from Unity callback captures and shipped-DXBC comparisons.

## Evidence reuse and gaps

Read first: U/HANDOFF.md, PROJECT.md, SHADER-AUDIT.md, SHADERS.md and V/README.md. The existing work should be reused, not rerun as a prerequisite for this research.

- **Earlier Unity audit:** H/audit_summary.json plus focused_manifest, audit inventory and reports record 86 shaders, 584 exact broad trials, 4,638 controlled trials across 59 shaders, matching serialized state and 148 vertex uniform-name interfaces, with D3D11/D3D12 player probes. This is bounded fixture evidence. The other 27 shaders lack expanded controlled cases; original vertex arithmetic, arbitrary HDR/input/control combinations and BC SRV upload integration remain outside those claims. HANDOFF explicitly states this player/build predates the later RadialBlur/FilmGrain corrections.
- **Current native shader comparison:** V/evidence/verification.json records 146 image passes, 18 Weather samples and 12 additional RadialBlur cases with the stated one-byte bound. V/evidence/animation/summary.json records 3,375 comparisons across 25 scenarios; corrected FilmGrain/Weather samples are raw-float exact. These compile current recovered HLSL rather than using shipped DXBC as implementation. Supplied fixture vertices and inputs do not establish a complete compositor graph.
- **Runtime callback/graph evidence:** V/evidence/runtime/{provenance,summary,verification,combined-coverage}.json records 21 scenarios / 2,835 additional comparisons for CameraFlash, ImageParticles, StageLights, TransmitLight and TV; combined animation/runtime is 46 / 6,210 across 19 animated passes. `uncovered_runtime_paths: []` is bounded to those enumerated paths, not the whole registry. TV feedback and particles execute natively; several CPU inputs are captured traces looping two seconds. StageLights is separately continuous.
- **StageLights continuity:** V/evidence/stage-continuity/summary.json records 1,460 live-path checks and 270 Unity oracle samples with at most one channel byte difference, including the former 2/4/6-second seams. Replaying the old two-second sway trace had discontinuities; use the continuous native formula as bounded reference evidence.
- **Native composition feasibility:** N/README.md and evidence/{probe,smoke}.json cover its two-layer/alpha/mask/multipass/video/Spout scene. Its blur matched the shipped DXBC on one gradient; Spout receiver was same process/device; preview used one process and basic GUI loop. These cannot replace the new separate-process GPU preview gate or a real external receiver test.
- **Qt session evidence:** P/prototypes/qt-session/evidence/README.md covers state/session/CLI/MCP and a separate-process Spout toggle receiver. Its image renderer/PNG transport and diagnostic opacity do not qualify native GPU composition, shader parity or the superseding property-sync model.

Read-only freshness checks performed for this note: current EffectRegistry SHA-256 equals the runtime provenance hash; current exporter equals its recorded hash; generated runtime-oracle bytes equal the recorded trace hash (all three exact). Current V catalog/source and report checks are recorded below. No Unity/native test was rerun, and this note does not claim renewed runtime qualification.

## Boot and externally supplied components

Recovered C# source is available for the wrappers and many managed decoder/control algorithms, but external/native implementations are separate authority. `SpoutSource` creates/configures Klak.Spout receiver, `SpoutOutput` configures sender, and InputSources delegates discovery. Current `U/Assets/Scripts/BootLayer/Klak.Spout.Runtime.cs` actually contains throwing GetSourceNames/receivedTexture/sourceName accessors and inert sender configuration; SetResources only stores a field. PROJECT's generalized boot summary is therefore not a working-integration guarantee. The observed source bodies take precedence over prose. `Owl.Boot.Media.ChildProcess.Start` in BootLayer/Owl.Boot.cs throws; GameCapture/recording/window APIs have further stubs. `FfmpegVideoPlayer` depends on Owl.Boot.Media. Shipping plugin files or compiling their wrappers is not evidence these paths operate in reconstructed Unity or in C++.

N provides independent SDK feasibility evidence only within its measured scene. New Spout input/output must prove discovery, actual frame arrival, dimensions, alpha/orientation, sender/receiver loss and reconnect on the chosen native integration. Decode, capture, audio, recording and virtual camera need their own capability gates. Online/shop/account services remain outside the map's scope.

## SHA-256 snapshot and verification record

Full hashes below identify source/report bytes read; they are not a claim that every file has runtime conformance coverage. Prefixes are defined above. Live plan hashes identify only this research snapshot. The only new repository file in the research commit is this note.

Read-only digest check: all 148 current catalog entries match their current ShaderLab source hash; all 146 image-pass records in the retained demo report match the current catalog source identity. This supports source freshness for those captures, while the earlier Unity audit remains subject to HANDOFF's explicit stale-build warning. This check compared recorded identities; it did not rerun GPU work.

| Artifact | SHA-256 |
| --- | --- |
| `R/Owl/OwlApp.cs` | `d7fc400aac13c8071281e9d7d47039ed6080cd0bec6b95c6f84652d73da7f6c2` |
| `R/Owl/Composition.cs` | `6cd84671d766a7cf4ba54a6df610c9dbc3e90833ef7d9e165ad916adc38f9cdf` |
| `R/Owl/Units.cs` | `d5bede21b0368514cce07e595c65ac2ac1f9710c5fceed99b8d0f6cdf72dfd35` |
| `R/Owl/LayerManager.cs` | `518e537dd0d86c26cf6af9cb6651e870a23cc4eead8d2a1db3b510c3973d0daa` |
| `R/Owl/LayerView.cs` | `ca5e904c0a4905cc2e082bbc9bd2b8f961e47d5e4007da85f72ea6b27ae7ecff` |
| `R/Owl/OutputCompositor.cs` | `d6d7b012326bcd95328e2b1a05cdccf9f6a0f806901000411396fb7ac4b5545c` |
| `R/Owl/LayerFolder.cs` | `8c83c96bc9a66bfc2571dbf754f8e2ba0600d8fa603afd55994e066e5a7c36c3` |
| `R/Owl/LayerFolders.cs` | `3c955c60d79d480e8ad9b06ac123b339d1cfca68090a64bafd0bf6e691dfbd95` |
| `R/Owl/MaskModel.cs` | `da1b6fc70b9181ce3ee824f5a61bb462913583759b6dc5fe96aecbcf894ed69b` |
| `R/Owl/LayerMaskMath.cs` | `63819172d45b8543e0bd90ae82b3a12ac3ebded28613baa194fa7fb87eb2066f` |
| `R/Owl/RoleMask.cs` | `d3e8422889a7cef8ecbc16c7361fd99d417e7c4b6e0c693b1ebbf56bf479ac47` |
| `R/Owl/ExemptLayers.cs` | `11953709f3a471da395c3c3984bbc8f5110fec1a34d8b50f7c8188b0155477a0` |
| `R/Owl/SourcePool.cs` | `ebd2f51668cf462e1113286252f24aea8a9372f0c5ed83d787dd9f790f51146f` |
| `R/Owl/ILayerSource.cs` | `6e7ec4b02784d11ceab9ee22a95c8c976e8eafa775f9e293956d75876469dcf1` |
| `R/Owl/LayerSourceFactory.cs` | `1eb9e6e97ed942ed394b8c18169b8398680635070b79433822cf0419f0c95a4c` |
| `R/Owl/InputSources.cs` | `0fb00dddc97016241d9f6a4fb15aaeb8a049788e7be41c23f59a96e623a404d6` |
| `R/Owl/LayerModel.cs` | `2446b6c7b8e9155405df1a49f9df9a0820f24e162c1197f99e61dc598777025b` |
| `R/Owl/SourceModel.cs` | `96b67e0c755260f74bc15ef6b0a07ec1e2a0c39cd416f07f7a7e0cbe97fb8540` |
| `R/Owl/VideoFileSource.cs` | `1a71c64f112411696373306856bea00cf76a2e0d7f15c2087472b960551bfeab` |
| `R/Owl/SpoutSource.cs` | `fa600768a81cf4f366f00196fb70133916389f867fe07f5108f868d7cb8b8ac2` |
| `R/Owl/SpoutOutput.cs` | `14528c5381d1f89396f73b65d2ec27093b36f7c0c604b41569c248525158a19e` |
| `R/Owl/FfmpegVideoPlayer.cs` | `e2c2012e165d3300201b65cd56f58208a8fa2ebc920bee8258718350efcbabea` |
| `R/Owl/EffectDef.cs` | `e98e1cd06c4f8b4f0613ce6b123686dee2e585b63490546442947f442b96df92` |
| `R/Owl/EffectChain.cs` | `2b8ca017b884848c533f80f1bb8fbddd6898aa1c8cd8215278cb79b498ec2e4f` |
| `R/Owl/EffectContext.cs` | `437fc2dfe5a436aaded2e5e8228d45a75f350bca452768bab505509b6312e2b3` |
| `R/Owl/EffectRuntime.cs` | `11a661713c204f64a71540c5afbd8d695e96ff968638a8556531bb93995271d6` |
| `R/Owl/EffectMix.cs` | `2e0d618a6d239fb0aba0b46f0f665a101daf7d9bff7322db5b0da5791a2efaa5` |
| `R/Owl/EffectRegistry.cs` | `b380861e07878a78e6d51132623ede475552f26e88dd399334ea8353e8c13ad3` |
| `R/Owl/BlurUtil.cs` | `9c6dfb9d0cd7b21775f6e0c24df706c281bef8ef5c86b2119b58f65abe100e12` |
| `R/Owl/LayerStats.cs` | `97ae0af251661032f09dddd67f01a5c46628168682bcc3bda8fb4fcd83764be7` |
| `R/Owl/LayerMotionClock.cs` | `c23aabb8aabbaa4e3d0761bb281744317d5781f96672ff54c26e611eff254c6a` |
| `R/Owl/LayerMotionEval.cs` | `987cd786b6d8ce7d5efc865110de6e0b2a6814f1d38b3eafb998e0f497e45857` |
| `R/Owl/CameraRigModel.cs` | `f6c12fcc4b49e621df8ce1378a2d4608529e4efd832b0828255c36da696cd0c1` |
| `R/Owl/CameraRigClock.cs` | `4c7a94e303c2b6bec2e612c98dab207651d2d83c3dd1798af704901f8175fd1f` |
| `R/Owl/CameraRigEval.cs` | `ef8b5db363a9f78fa7dd757b9ff071ac889f81c0a9ec26cb4349c4e5dc7a9e2b` |
| `R/Owl/CameraRigFrame.cs` | `f5dfd93fd8a8854edde020fe9687f2e96b45689fc4b66afcbe9c2d8d09dd591b` |
| `R/Owl/CameraRigMix.cs` | `6db2868ff83320c3404286d983dbfe375785d97169f78ae1bb7dba3c5d0995c4` |
| `R/Owl/SceneMixer.cs` | `3a94e8126db6e6d64c347ebd4735ec8ec43e6a04f3ca6bd7b45666d12f90a3c1` |
| `R/Owl/TransitionDriver.cs` | `ad4fc1209838a1867ae7d0a0902d41c7f4f4ffee39b7ce4a135c9048cc0ab096` |
| `S/FX/Pixelate.shader` | `4760b2a15ca19ba32cd42fe6b0cff20961b0bf5f97f22f01c552749955f25985` |
| `S/FX/Blur.shader` | `58d63e7109ecf20aaa8d627d1118cd389dab3b7bb1f65ab82f306c50d31787e8` |
| `S/FX/FilmGrain.shader` | `e2b6a0bea196c220424687dcc0d48f4276f75699c58afa22132bf158d84ecd3f` |
| `S/FX/Weather.shader` | `b525c1766fde81969193f3c75ba21ae3dcd9921c56c73acea12c0434edd0eda1` |
| `S/FX/TV.shader` | `da325614e27799f0eb538fbf3b2f4d94baadda9741ae987428d55cd7dd8b1c4b` |
| `S/FX/ImageParticles.shader` | `36e42a9c81452321b9c444e4d35873582fe32c4cb5e8e11af7c69b6bf71d6816` |
| `S/FX/Levels.shader` | `d4ef2dc9cb9ec3eafa2277f72151f72a59b8789092dd5cd09073ff3ff80a4c6f` |
| `S/FX/AutoColor.shader` | `c4596d17a732b9e25e967c1c5695623b95e0170ead425e4711bf005a7f5fbe31` |
| `S/FX/RadialBlur.shader` | `4cd54e131fbf05b3724341af90f881193e4254b9ef90cdbe4a5a5bd27852fa97` |
| `S/Owl_Layer.shader` | `577da730388c753a1abb2aa65db070bdc33370a0f782719ce91cf35219090222` |
| `S/Owl_Output.shader` | `4aa2ab684a0f95ece4e3a6d3a587237a624ca5cb6ed544e2f785c1b4e11e4beb` |
| `S/Owl_EffectMix.shader` | `0194ba3b3ad56e4a657abce3530b623f471f8db29fe2b6003fec9ff703d3c590` |
| `S/LayerMask.shader` | `ab5b7bb52a13ecfe77138b711a7b2aed173b08969d98cea781e98fa6af4b7072` |
| `S/Owl_RoleMask.shader` | `7e7512cb2f30af8d37f48d7916210bbda3fe5044843ef0f59079977900d5cafe` |
| `U/HANDOFF.md` | `d984215322047acb4e329b04c8f06f20d83636d806f4c11bf3c061fb4227f25a` |
| `U/PROJECT.md` | `c16d29af8382638a13169f288404913e653f160177fa64dae23a43f46feff802` |
| `U/SHADER-AUDIT.md` | `679ac0c4a02f98d1660613f58ac61cf31e2d41bda5b670abaf5c8c70d6679b44` |
| `U/SHADERS.md` | `98dcdccbf4eb07fd3fbdf9b4e4f802922bea2aef6eaaa6a76092292a66b51577` |
| `U/Assets/Editor/OwlAnimationInputs.cs` | `83e1acb257654d0755ca5292fd637a55539e3d54fdcc02c35c03b3ac1e7a07ba` |
| `U/Assets/Scripts/BootLayer/Klak.Spout.Runtime.cs` | `2c7b85d6b5250f338e9f4de34d30cffb88b947329654f7ebc54dec8073eccefb` |
| `U/Assets/Scripts/BootLayer/Owl.Boot.cs` | `6294928c4e5a20f7838ddf47002cd4579053715e2fd8ee7271ae1fe94586b0ed` |
| `V/README.md` | `85d0a4154750d187d13f5bd27b3d0b55b26c60d349ec0f3d6fffddfec932782c` |
| `V/prepare.py` | `51839b76a1d557088d0b8b46db89a2181eeef13a7d0743d49ee60ea43e857c69` |
| `V/generated/catalog.json` | `1cb19ac4c051735b9a9bb1302e9ceebdb634410ea89b356040759b30f3dc4f0f` |
| `V/generated/runtime-oracle.json` | `94c990c27324695b5869f79a689e9b920a4ec863ed8141f9dc9519c9b5702910` |
| `V/evidence/verification.json` | `94a8d20474ba112fbbfa7952b078b901bc72fccfdcb6e5da74acd9e87eb7e79d` |
| `V/evidence/animation/summary.json` | `0a94fbfc98ba141b73dab8c7bbbd1a45fd7dde4b347328db03fc7aaaa9ef11c6` |
| `V/evidence/animation/verification.json` | `5166ff26eb89319c246171f0cab959269efd44554e6ed56d37b74a1f80e2ea1a` |
| `V/evidence/runtime/provenance.json` | `8a6348d48e5317b6fcc5eab5d922006827ac1d32db321d400736151e46de3147` |
| `V/evidence/runtime/summary.json` | `711778f9974db81d646d8b52062b5700ec03f6cb2954de455ff7bc7419756145` |
| `V/evidence/runtime/verification.json` | `31a91564334c5c0e516b2aafdae367c5bb3d3549b1ba1f72d5ccb5c1c1372780` |
| `V/evidence/runtime/combined-coverage.json` | `9eb5502fc0ef71c8f17e8ee18d6716eb75d6b47b962a2c3df8dc7d5acf0610d1` |
| `V/evidence/stage-continuity/summary.json` | `92187cb707bd5536783afa520ff0347cb66c57098d11b813ee6da396fe42cbb4` |
| `H/audit_summary.json` | `5b862920b07ea64856e3ad29e9d61885db768afcf0e9144b926b06e3bb28dc39` |
| `H/audit_inventory.json` | `a7bd83ad5a89fe02a28b4aeb64b06fb643b3d64613fadad7aebb8f8c64585a12` |
| `H/focused_manifest.json` | `d3037384ffdadaa3aa9870a328d72d10f5c6be5093b66a0df64917a0848cac69` |
| `N/README.md` | `93672523ee8d540a21b76a57da011913cf9fb8cb039c72c5a44d1c7fe3d609e3` |
| `N/evidence/probe.json` | `be9af45733081ecc1368c121300340d3d1a7c9b57ec315256390520efd5acaed` |
| `N/evidence/smoke.json` | `0320158685c2381d3a2fcfd332a6fb534add8c980214c0bf2679618e2282929f` |
| `P/GLOSSARY.md` | `a1e36d0d0afadeeda812994e9e82137ff51c02a51a178bf89aeead8bbdc39bca` |
| `P/docs/superpowers/specs/2026-10-31-qt-core-design.md` | `3b807487954e68cb94685cd901110d35b55b4c9f96ade9c53812a58fc3f8ba68` |
| `P/docs/superpowers/plans/2026-10-31-qt-core.md` | `340bc6d59ccc995ed1924c87195c31da56758dab34361c545d3bc7b0949476ba` |
| `P/docs/superpowers/plans/2026-10-31-qt-core-a-session.md` | `7ab1c5a0cee2d3b7f9dbb172e70bb800cdf80d1525429a1950d91fda0a8b4230` |
| `P/docs/superpowers/plans/2026-10-31-qt-core-b-gpu.md` | `c0dea36eff983ce166f779dcbc83dc82dc2656b0d7fdabed5aecbf8a03879a70` |
| `P/prototypes/qt-session/evidence/README.md` | `8e2c86b28865201ff2319313fd3ce203e568f269123529d792552b22ee2fdea3` |
