# Separate-process D3D11 to Qt Quick preview: supported paths and evidence gate

Research for [OpenOwl #3](https://github.com/reon-nynes/OpenOwl/issues/3), part of [map #1](https://github.com/reon-nynes/OpenOwl/issues/1). Investigated 2026-10-31. Target: Windows, Qt **6.10.2 MSVC x64**, independent persistent Session renderer and Qt Quick presentation process. This is AFK research, not approval of a transport contract or a production implementation plan.

## Finding

A GPU-only preview has a supported route: share an ordinary D3D11 2D texture, open it on the preview's D3D11 device, synchronize access, and present it through `QNativeInterface::QSGD3D11Texture::fromNative`. Both direct sampling of the opened shared texture and copying into a preview-owned texture are plausible. **Neither separate-process Qt Quick path has been validated by the existing experiments.** A bounded two-process experiment must establish resource reuse, completion, overload and recovery before the coding plan selects one.

For the first experiment, a small per-client keyed-mutex ring with a preview-local RGBA8 copy is a useful conservative candidate. It keeps the producer's shared slot out of Qt's retained sampling lifetime. That is an engineering inference to test, not a product decision or a demonstrated latency advantage. A direct-import variant can then measure the copy saving against its longer ownership interval.

“GPU-only” here permits GPU texture copies and a small IPC descriptor. It excludes routine frame readback, CPU image encoding and reupload. Test-only pixel readbacks must be labelled separately.

## Evidence classes and version boundary

- **Documented fact** means a primary Microsoft or Qt API contract, linked next to the claim.
- **Source observation** means the pinned implementation or local header/evidence file says exactly that; it is weaker than a platform guarantee.
- **Inference/proposal** means a design consequence or test criterion developed here. It has not run.

Context7 resolution preceded API research. Selected `/websites/doc_qt_io_qt-6`, `/websites/learn_microsoft_en-us_windows_win32_direct3d11`, and `/leadedge/spout2`, all High reputation. Three narrowly scoped queries covered native D3D11 import, shared-texture synchronization and Spout receive access. Qt 6.10.2 was not offered as a version-specific Context7 collection. The rolling Qt documentation currently identifies itself as 6.12.0; the `/qt-6.10/` pages identify 6.10.3. Neither is silently treated as 6.10.2. Installed 6.10.2 headers and upstream **v6.10.2** source provide the version checks below.

Upstream Qt Declarative v6.10.2 resolves through annotated tag `858423070a1a4f7d8d5c8c525bf056f824b50430` to commit **`09c70541c76659bcd8c49f05841b0e778c9ffd4c`**. [Tag metadata](https://api.github.com/repos/qt/qtdeclarative/git/tags/858423070a1a4f7d8d5c8c525bf056f824b50430). The local kit is not claimed to be a reproducible build of that source commit merely because its version matches.

## What is already demonstrated locally

| Evidence | What it supports | What it does not support |
| --- | --- | --- |
| `OpenOwlCLI` baseline `0c7253979762db253fe528a9fee9bd59c7da457f`, `prototypes/qt-session/README.md` and `evidence/README.md` | Separate persistent engine, independent Quick/CLI/MCP clients, CPU QImage/QPainter composition, PNG preview at about 10 Hz; Spout upload can be toggled independently | A native GPU Quick preview, thread isolation, frame completion or a production compositor |
| `prototypes/qt-session/evidence/spout-toggle-probe.json` | Two output cycles; separate receiver PID/device, six 640×360 frames per cycle, exact static top 348 rows; Session advances while output disabled | The moving last 12 rows, Qt Quick receiving shared textures, stress, crash/resize/device-loss recovery or external OBS behavior |
| Sibling `D:/Dev/Owl/OwlNativePrototype`, commit `4818bcfe2a9a175a6be0c738032aafe0fed0bd2a`, `src/compositor.cpp:121`, `:226`, `evidence/manifest.json`, `evidence/smoke.json` | RGBA8 native texture wrapped on Qt's own D3D11 device; one process and basic render loop serialize context use. Recorded 960×540 smoke: 630 frames, 21 exact same-process/device Spout receives, previewReadback=false | Independent engine/Qt devices, a threaded Quick consumer or cross-process preview synchronization. Recorded FPS/GPU timings are specific to that scene and RTX 5090; they are not a target budget |

The native manifest's compositor SHA-256 matches the file currently present. These are readings of existing recorded reports and sources, not reruns. No executables were launched, stopped or rebuilt during this research.

`OwlUnity/HANDOFF.md`, `PROJECT.md`, `SHADER-AUDIT.md` and `SHADERS.md` were read to bound extrapolation. Their shader reconstruction evidence does not establish this transport. Newer recovered C# under `Assets/Scripts/OwlRuntime/` and canonical shaders under `Assets/Resources/Shaders/` remain behavioral inputs, not a GPU handoff proof. Native stubs and earlier binaries add no such proof.

## Qt integration choices

### 1. Native scene graph texture import: smallest presentation surface

**Documented facts.** `QSGD3D11Texture::fromNative(texture, window, size, options)` wraps rather than owns the native object; the caller deletes the returned `QSGTexture` independently. It must execute on the scene graph rendering thread after initialization. The supported scope is 2D RGBA textures, and only alpha/mipmap options are honored. [Qt API](https://doc.qt.io/qt-6/qnativeinterface-qsgd3d11texture.html); [same contract in v6.10.2 source, lines 733–762](https://github.com/qt/qtdeclarative/blob/09c70541c76659bcd8c49f05841b0e778c9ffd4c/src/quick/scenegraph/coreapi/qsgtexture.cpp#L733).

**Source observation.** In v6.10.2, this D3D11 entry point delegates through the native-texture helper without an explicit native format. `QSGPlainTexture` defaults the imported wrapper to `QRhiTexture::RGBA8` and sample count 1. A non-null QSG wrapper alone is therefore not proof of successful compatible rendering. [Import implementation](https://github.com/qt/qtdeclarative/blob/09c70541c76659bcd8c49f05841b0e778c9ffd4c/src/quick/scenegraph/util/qsgplaintexture.cpp#L69); [window helper](https://github.com/qt/qtdeclarative/blob/09c70541c76659bcd8c49f05841b0e778c9ffd4c/src/quick/items/qquickwindow.cpp#L3280).

**Inference.** Start with **typed `DXGI_FORMAT_R8G8B8A8_UNORM`**, mip levels=1, array size=1, sample count=1. Do not assume BGRA8, sRGB, NV12, an HDR working texture, a typeless resource, or an MSAA render target can be handed directly to this convenience API. A GPU conversion/resolve into the preview format is a separate stage; a raw `CopyResource` does not perform arbitrary pixel-format/color conversion. Declare transfer function, alpha representation, orientation and pixel dimensions in the experiment. Test premultiplied alpha and asymmetric corners explicitly. `TextureHasAlphaChannel` is not an alpha conversion.

Open the shared resource **on the device used by the Qt scene graph**, then pass that local `ID3D11Texture2D*`. COM pointers and immediate contexts are process-local, not IPC payloads. Qt's QRhi import documentation requires the texture to belong to the same device/sharing context and requires matching metadata; it does not make an arbitrary foreign-device pointer usable. [QRhiTexture::createFrom](https://doc.qt.io/qt-6.10/qrhitexture.html#createFrom).

Two device arrangements are supported building blocks:

- Let Qt create its device; obtain `DeviceResource` and `DeviceContextResource` from `QSGRendererInterface` at valid rendering stages, then open/copy the producer texture there. Resource availability is stage-sensitive. [Renderer interface](https://doc.qt.io/qt-6.10/qsgrendererinterface.html).
- Before scene graph initialization, select the producer's adapter with `QQuickGraphicsDevice::fromAdapter(LUID low, high)`, or create a **preview-process** device/context and supply it using `fromDeviceAndContext` and `setGraphicsDevice`. Caller-supplied objects must outlive Qt use; adoption does not transfer ownership. This is not adoption of the separate engine's raw pointer. [Graphics device factories](https://doc.qt.io/qt-6.10/qquickgraphicsdevice.html), [window device setup](https://doc.qt.io/qt-6.10/qquickwindow.html#setGraphicsDevice).

### 2. Preview-local GPU copy followed by native import

**Proposal.** Acquire a shared slot, enqueue a GPU copy into a non-shared RGBA8 texture on the Qt device, establish completion of the shared-source read, then return the slot. Qt samples only preview-owned storage. Keep the last complete preview image when nothing new is available.

This adds a texture copy and storage, but separates shared-slot retirement from how often Qt redraws a retained node. If the copy and sampling use Qt's immediate context in render-thread order, command ordering is simpler than a worker feeding Qt through another device. Local textures still need correct last-use ordering/retirement; a second device does not get free synchronization merely because both devices are in one process. Measure copy cost and lease duration instead of asserting “zero-copy” or an acceptable frame rate.

### 3. QQuickRhiItem / QRhi import / custom rendering

`QQuickRhiItem` (since 6.7) provides a public item/renderer split for offscreen rendering that Qt composites. QRhi-backed implementations require `Qt::GuiPrivate`; the QRhi family has limited compatibility guarantees, and software Quick is unsupported. This is a viable version-pinned path when explicit format selection or conversion justifies the extra rendering work, not a required wrapper for a simple RGBA8 preview. [QQuickRhiItem](https://doc.qt.io/qt-6.10/qquickrhiitem.html). `QRhiTexture::createFrom` plus `QQuickWindow::createTextureFromRhiTexture` is another version-pinned building block; native-resource ownership and synchronization remain external responsibilities. [QRhi import](https://doc.qt.io/qt-6.10/qrhitexture.html#createFrom).

`QSGRenderNode` or native under/over-QML rendering is also possible, but requires explicit graphics-state discipline and is a larger presentation integration. The official D3D11-under-QML example establishes native drawing, not separate-process IPC. [Qt example](https://doc.qt.io/qt-6.10/qtquick-scenegraph-d3d11underqml-example.html).

`QQuickRenderTarget::fromD3D11Texture` redirects **Qt's output** to a texture; it is not the import operation for displaying the producer's image. `QQuickFramebufferObject` is OpenGL-specific. Neither is a reason to introduce a second graphics API into this D3D11 question. [Qt render-target source](https://github.com/qt/qtdeclarative/blob/09c70541c76659bcd8c49f05841b0e778c9ffd4c/src/quick/items/qquickrendertarget.cpp), [RhiItem comparison](https://doc.qt.io/qt-6.10/qquickrhiitem.html).

## Shared resources and synchronization

### Resource and handle contract

**Documented facts.** D3D11.1 guarantees sharing for a bounded set of ordinary 2D texture descriptions including RGBA8. Begin with DEFAULT usage, shader-resource plus render-target bind flags, one mip/array element and no MSAA. Extended sharing support does not make staging/dynamic/immutable, depth-stencil or MSAA resources generally shareable. [D3D11.1 sharing guarantees](https://learn.microsoft.com/en-us/windows/win32/direct3d11/direct3d-11-1-features).

Use coherent handle families:

| Family | Creation/opening | Lifetime obligation |
| --- | --- | --- |
| NT shared resource | `SHARED_NTHANDLE | SHARED_KEYEDMUTEX`, `IDXGIResource1::CreateSharedHandle`, `ID3D11Device1::OpenSharedResource1` | Transfer a valid target-process handle by duplication/inheritance, or use a deliberately scoped name. Close each owned NT handle. A numeric handle copied over a socket is not automatically valid in another process |
| Legacy DXGI resource | Appropriate shared resource, `IDXGIResource::GetSharedHandle`, `ID3D11Device::OpenSharedResource` | Returned legacy token is not an NT handle: do not CloseHandle/DuplicateHandle it; preserve the resource lifetime while another process opens it |

Microsoft recommends NT handles from D3D11.1 onward. `CreateSharedHandle` has access/security attributes and is called once per resource; duplicate for additional owned handles. Never combine `D3D11_RESOURCE_MISC_SHARED` with `SHARED_KEYEDMUTEX`; those flags are mutually exclusive despite loose wording in some older examples. [NT handles](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle), [OpenSharedResource1](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_1/nf-d3d11_1-id3d11device1-opensharedresource1), [legacy handles](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiresource-getsharedhandle), [misc flags](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_resource_misc_flag).

**Proposal.** Publish producer adapter LUID, generation, dimensions, format and slot identities together. Compare the actual preview device's adapter LUID rather than adapter index/display name; `DXGI_ADAPTER_DESC` exposes that identifier and Qt accepts its two halves. The initial experiment should require the same LUID and fail gracefully on mismatch. Cross-adapter sharing/copy, automatic device migration and CPU fallback are separate unproven capabilities, not implied by successful same-adapter sharing. [DXGI adapter descriptor](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc), [Qt adapter selection](https://doc.qt.io/qt-6.10/qquickgraphicsdevice.html#fromAdapter). Pinned Spout source also explicitly handles sender-adapter matching; see source pointers below.

### Viable synchronization choices

| Choice | Supported mechanism | What the experiment must still prove |
| --- | --- | --- |
| Keyed mutex | Per-resource `IDXGIKeyedMutex::AcquireSync` / `ReleaseSync` provide synchronized exclusive access across devices | Correct keys, finite/no-wait acquisition, full write/read lifetime, abandoned-owner recovery and a bounded slot protocol |
| Shared texture + explicit completion + IPC lease | Producer/consumer each use `D3D11_QUERY_EVENT` on their own context; send readiness/return only after `GetData` confirms completion | CPU metadata cannot race ownership; no slot is reused while queued GPU access survives; query polling cannot block the renderer |
| Shared fences, capability gated | `ID3D11Device5::CreateFence` / `OpenSharedFence`, `ID3D11Fence::CreateSharedHandle`, immediate `ID3D11DeviceContext4::Signal` / `Wait` | Runtime/device support, shared resource flag recipe, monotonic values, both ready and consumed directions, bounded recovery when a peer disappears |
| Spout SDK receive-copy | Pinned SDK supplies sender discovery, shared textures, frame count/access and receiver texture copies | Its contention and disconnect behavior meets preview budget; version-specific SDK behavior is not mistaken for a completion proof or a nonblocking API |

For keyed mutexes, a newly created unowned resource starts at key 0. Later acquisition must match the last release key. Acquisition can stall the CPU; timeout 0 tests immediately. Compare return values explicitly: `WAIT_TIMEOUT` and `WAIT_ABANDONED` must not be accepted through `SUCCEEDED(hr)`. Abandonment requires recreating both shared surface and mutex. No recursive acquisition; wait ordering among multiple devices is undefined. [AcquireSync](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgikeyedmutex-acquiresync), [ReleaseSync](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgikeyedmutex-releasesync).

**Critical documented distinction:** `Flush()` submits queued work and may return before GPU execution finishes. Microsoft specifically identifies event queries plus `GetData` as a way to establish completion. Polling is not completion unless it actually reports ready. [Flush](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush), [event query](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/ne-d3d11-d3d11_query), [GetData](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-getdata).

Shared fences permit a GPU dependency rather than CPU busy-waiting: Signal updates the fence after preceding work completes; Wait prevents future work starting before the target value. That does not make a wait bounded if a dead producer can never signal. Gate support through interface queries and successful creation/opening; fence declarations in the installed SDK do not prove driver support. Keep any not-yet-satisfied peer wait off the critical Session render queue, or choose only already-completed candidates. The keyed transport can initially remain in place while fence completion is evaluated; eliminating it would be a separate tested recipe. [CreateFence](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_4/nf-d3d11_4-id3d11device5-createfence), [shared fence](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_3/nf-d3d11_3-id3d11fence-createsharedhandle), [open](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_4/nf-d3d11_4-id3d11device5-opensharedfence), [Signal](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_3/nf-d3d11_3-id3d11devicecontext4-signal), [Wait](https://learn.microsoft.com/en-us/windows/win32/api/d3d11_3/nf-d3d11_3-id3d11devicecontext4-wait).

### Qt threading, retained images and completion

The scene graph may render on a dedicated thread; custom node updates synchronize with item state. Do not couple correct native access to the basic loop used by the earlier spike. [Scene graph threading](https://doc.qt.io/qt-6.10/qtquick-visualcanvas-scenegraph.html).

Qt's `beforeRendering` is an integration point for copy commands; use direct render-thread connections and the external-command state boundaries when mixing D3D11 calls with Qt. `afterFrameEnd` means submitted, and `frameSwapped` means queued for presentation; neither is a GPU-completion acknowledgment. Invalidation requires render-resource cleanup. [QQuickWindow stages and lifecycle](https://doc.qt.io/qt-6.10/qquickwindow.html). D3D11 devices support concurrent resource creation, but a device context must be used by only one thread at a time; serialize the immediate context, including Qt/Spout access. [Microsoft threading](https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-render-multi-thread-intro).

**Inference/proposal.** Direct import holds a shared lease through the last Qt GPU sample. If the retained node may redraw the old texture on later frames, returning it merely because one frame was submitted is unsafe. Detach/replace the node's reference, then establish completion of all prior uses before returning that slot. Repeatedly displaying a preview-local copy avoids this retained shared-lease issue. End a completion query at the intended last-use boundary, submit it and poll without spinning; alternatively prove fence ordering on that exact context. Hook names alone do not establish ordering of the commands actually using the resource. Hiding/occluding a Quick window can suspend useful callbacks, so cleanup/lease reclamation cannot require the next visible paint.

## Spout: useful option, bounded claims

Pinned source is Spout **2.007.018**, commit **`6b68a5b4fdeca6130b851e5f9366467a844860e5`**, dated 2026-10-10; local vendor checkout is clean. Source observations:

- `spoutDX::SendTexture` takes texture access, copies to the SDK shared texture, calls Flush, updates frame count and releases access. It returns true even when access was unavailable; that boolean alone is not a “new frame delivered” acknowledgment.
- `ReceiveTexture` copies a new frame to receiving storage under access protection, Flushes and releases access. Connection success is distinct from a new image. Follow `IsUpdated` and recreate matching receiver storage on sender/size changes.
- `spoutFrameCount` chooses keyed access when present and otherwise a named access mutex. Both relevant access paths use a **67 ms** timeout. Frame-sync event signaling/waiting is a separate pacing mechanism, not a fence proving the GPU completed texture access.

[Pinned SpoutDX implementation](https://github.com/leadedge/Spout2/blob/6b68a5b4fdeca6130b851e5f9366467a844860e5/SPOUTSDK/SpoutDirectX/SpoutDX/SpoutDX.cpp#L491), [frame count/access implementation](https://github.com/leadedge/Spout2/blob/6b68a5b4fdeca6130b851e5f9366467a844860e5/SPOUTSDK/SpoutGL/SpoutFrameCount.cpp#L761), [shared texture flags](https://github.com/leadedge/Spout2/blob/6b68a5b4fdeca6130b851e5f9366467a844860e5/SPOUTSDK/SpoutGL/SpoutDirectX.cpp#L550).

**Inference.** Spout is a viable receive-copy experiment and remains relevant for external Output. Its 67 ms access window is unsuitable as an assumed hard bound for a 60 Hz critical renderer/Quick callback. Do not assume SDK Flush under a named CPU mutex supplies the stronger explicit two-sided completion contract proposed above. Test that behavior, or use a custom synchronized preview transport. Putting Spout on a worker is not sufficient unless the worker's device/context and delivery into Qt are also synchronized. Passing Qt's device to `OpenDirectX11` reduces device mismatch but does not permit concurrent context access. Spout input/output and internal preview need not share one transport contract; no final choice is made here.

## Proposed bounded lifetime and overload model

This is a candidate for testing, not a serialized production schema:

1. One producer and one consumer begin with **three slots per preview connection**, each with generation and slot identity. Maintain only fixed-capacity metadata; IPC notification is coalesced. A separate latest state/telemetry channel carries no image payload.
2. Candidate ownership sequence: Free → Writing → Ready → Reading → Free. A key-0 producer acquires only a free slot without waiting; key-1 consumer acquires only a published ready slot. Producer completion precedes readiness; consumer shared-source completion precedes return. The initial conservative version may use event queries on both sides before ReleaseSync/publication/return, avoiding an undocumented assumption about a callback's completion semantics.
3. If no free slot exists, skip that preview export and continue rendering Program/Output. Do not allocate a fourth slot, wait for the client or enqueue per-frame IPC. On resume, discard stale Ready frames through an explicit safe acquire/release path and consume the newest eligible one; the producer can then publish a fresh frame. A slot cannot be reclaimed just because its descriptor was overwritten.
4. A consumer may keep its last complete local image while the producer progresses. Report frame sequence, source render timestamp, receipt and completion times separately; repeated display does not mean a new frame arrived.
5. For multiple previews, begin with independent leases/rings per connection or a proven broker-copy arrangement. A two-party keyed mutex is not a broadcast acknowledgment system. Bound total clients/pools; do not let one stalled client retain a slot needed by all others.

Use generation changes for dimensions/format, producer restart, connection replacement and device recreation. Pair descriptors with actual `GetDesc` results and LUID. Retire old wrappers/resources only after their last GPU use; discard stale notifications and acknowledgments by generation. Producer/client death must revoke the connection without reusing its in-flight surfaces. With an alive but hung client, stop publishing to that pool rather than freeing its leases blindly. Bound retired generations too: suspend that preview until retirement is safe instead of retaining a new pool on every resize forever.

On `WAIT_ABANDONED`, recreate that resource family. On `DXGI_ERROR_DEVICE_REMOVED`/`RESET`, gather the removal reason and rebuild device-dependent resources, not just the IPC handle. Qt scene graph invalidation and window recreation invalidate the preview's wrappers/device assumptions. A changed adapter requires renegotiation. These are separate events from ordinary sender absence. Microsoft's device-removal walkthrough supports rebuilding the device/DXGI chain; it does not automatically provide a Qt application recovery policy. [Device removed handling](https://learn.microsoft.com/en-us/windows/uwp/gaming/handling-device-lost-scenarios).

## Minimal experiment and pass/fail evidence

Do not expand either old prototype automatically. A later, explicitly authorized throwaway experiment needs only: a D3D11 synthetic producer executable, a Qt 6.10.2 Quick receiver with one image item, fixed-capacity descriptor IPC, timestamp/counter logging and failure controls. No project/editor/media/effects work is needed. Force and verify Direct3D11; start with matching adapter LUIDs. Proposed provisional thresholds below establish feasibility; product acceptance and final performance budgets remain HITL.

First run keyed NT-handle transport plus local copy and explicit query retirement. Only after that passes, compare direct import under the same fixtures. Fence or Spout variants are optional follow-up questions, not prerequisites or reasons to broaden a failed test.

| Case | Procedure | Proposed pass/fail evidence |
| --- | --- | --- |
| Independent devices/processes | Log PIDs, device identity, actual LUID, texture desc and negotiated generation; open each slot on Qt device | Distinct processes/devices, matching LUID, actual RGBA8 desc, correct image; no CPU frame payload. A same-device shortcut fails the question |
| Content/ownership | Render 10,000 changing 1080p60 frames with encoded sequence, asymmetric RGB corners, transparency and dense frame-dependent checksums. Test-only readback checks at least 100 received frames | Zero mixed-frame/tearing checksum failures; no channel swap, flip or alpha error; clear GPU debug output. Tag readbacks and exclude them from steady-state timing |
| Completion/reuse | Record writing/copy/last-sample completion per slot, lease and generation. Delay GPU work and consumer polling, exhaust all three slots and detach retained direct-import nodes | Zero overwrite before the preceding shared read is complete, zero return before completion, zero read before producer completion; counters identify dropped rather than corrupted frames. afterFrameEnd/Flush timestamps alone fail |
| Slow or absent client | Pause receiver consumption for 5 s, hide/minimize for 5 s, disconnect/reconnect, run 0/1/2 previews; compare a no-preview baseline | Producer remains advancing near baseline (provisional ≥95% completed-frame rate); producer waits for a consumer = 0. Three slots/client, fixed metadata, no growth beyond declared pools. Resume fresh output within 2 s; stale queued playback fails |
| Steady-state cost | After warmup, 60 s at 1920×1080 and 60 Hz; record producer CPU/GPU frame intervals, preview copy/import cost, waits, drops, frame age and handle/memory counts | Publish p50/p95/p99/max and baseline delta. Provisional 95% throughput rule above; no claim of glass-to-glass latency. Hard presentation-age budget remains an explicit later decision |
| Resize/generation | Alternate 640×360, 1920×1080 and 1280×720 100 times; delay old messages/acks; hold an old consumer lease across changes | Zero stale generation accepted and zero use-after-free; correct new desc/image; fixed active/retired generation cap; pause preview safely when cap reached rather than grow forever |
| Disconnect/crash | Kill only experiment-owned producer/receiver at each lease state, including ownership of keyed mutex; restart 20 cycles | Session independent of receiver survives; receiver reports unavailable on producer loss; abandoned resources recreated; no unsafe lease reclaim; reconnect within 2 s after healthy peer ready; process handle/resource counts settle |
| Adapter/backend mismatch | Deliberately select wrong LUID if hardware exists; reject unsupported backend in a separate test | Useful explicit unavailable/error state, no hang or accidental CPU fallback; record skipped hardware case as untested, never as passed |
| Device/scene graph loss | Inject device-error handling and scene graph teardown/recreation; test real removal/reset only in a separate authorized environment | No old-device wrappers reused, fresh generation/handshake, bounded resources and valid first new frame. Injection proves control flow only; without real reset, report real driver recovery untested |

Keep test-owned termination separate from the currently running Session; this note authorizes no process stops. Preserve exact source/build hashes, Qt DLL version, SDK/toolchain, adapter LUID/driver, flags, render loop, viewport/scale, fixture seed and JSON timelines. GPU debug warnings, failed HRESULTs, abandoned/timeouts and all skipped cases belong in the report. A pretty capture or a new native pointer is insufficient evidence.

## Local version/hash inventory

All hashes below are SHA-256 of the observed local files on 2026-10-31; Git revisions are listed separately. These pointers make the evidence inspectable even though sibling repositories and installed kits are not shipped by this note.

Qt root `C:/Qt/6.10.2/msvc2022_64/`:

| Relative file | SHA-256 |
| --- | --- |
| `include/QtCore/qconfig.h` (`QT_VERSION_STR` = 6.10.2) | `27033b0f666d01a4b0cac00e39c915ff292606f85dada301eb5251098ef45fbf` |
| `include/QtQuick/qsgtexture_platform.h` (D3D11 interface lines 49–57) | `f012609e4d53cff0795053283bee094f032b140ea2afa3148ee90c7647c1df14` |
| `include/QtQuick/qquickgraphicsdevice.h` | `9596614a3f91cdcea2f1485a4c8ba748c28af065176ec40ef2ad4c1e25119440` |
| `include/QtQuick/qquickwindow.h` | `f5fb4654c8ee11476aa66d484e98122cc1faba7aff03bb48595b92b2deb1f550` |
| `include/QtQuick/qsgrendererinterface.h` | `0cab1f81dfd12dbbab51d9267d0dcd0d1dfdfe03e20963249ee9ba217f5da27a` |
| `include/QtQuick/qquickrhiitem.h` | `cd71f167a13bc28c4605c6f337137227413f2af6b1c84791b3774a6d923efceb` |
| `include/QtGui/6.10.2/QtGui/rhi/qrhi.h` (native import lines 988, 1032) | `ab047f23b43d250f2ff10f4d71075c2a38802992df7c301f0183b685b68d3e37` |

Other evidence:

| File rooted as stated | SHA-256 |
| --- | --- |
| `D:/Dev/Owl/OwlNativePrototype/src/compositor.cpp` | `e095d4162d50d1afe36647ceaeadcefb983a5b224b5f959e536ff7381c0e8594` |
| `D:/Dev/Owl/OwlNativePrototype/evidence/manifest.json` | `45c9875f2476a3624d53153e91a22afcf0d390186cc595b12c742e1676d66095` |
| `D:/Dev/Owl/OpenOwlCLI/prototypes/qt-session/evidence/spout-toggle-probe.json` | `6adeb5a48f548a2bb2097d66252385c0fbd229ee81941abdbd0a4cb15c8bb332` |
| `D:/Dev/Owl/OpenOwlCLI/prototypes/qt-session/evidence/source-hashes.json` | `5df516f44413c98f45a179757c0dd7c4ae08810c0090d85712d71c211d367909` |
| Spout root `D:/Dev/Owl/OwlNativePrototype/vendor/spout/`, `SPOUTSDK/SpoutGL/SpoutFrameCount.cpp` | `eb6537597d011e8773ead7fa2c8a3f9c1a351c016095737d956238bbfbb5da91` |
| Same Spout root, `SPOUTSDK/SpoutDirectX/SpoutDX/SpoutDX.cpp` | `39cfe6df6f9927198cd72d207d368f37d2281965e1a1d2ddfea7781f5a5d97b7` |
| Windows SDK root `C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0/um/`, `d3d11_4.h` (Device5 fence declarations) | `25179b6474ecadbaa6edd7ce4ff14e9f698a2d66e42c8c50d153d7609b898e0e` |
| Same SDK root, `d3d11_3.h` (Context4 / fence declarations) | `40b7580a901962b6af806df66c69ec9349ff018f47537431e323b6b132f30b21` |

Installed headers demonstrate declarations, not a compiled working interop combination. No runtime capability probes, performance tests or recovery tests were run for this note. Event queries, keyed handoffs, fence flags, Qt callback ordering and bounded generation retirement must be demonstrated together in the proposed experiment. Choosing latency/quality targets, supported adapter arrangements, crash/reconnect UI and the eventual frame ABI remains HITL.

## Publication boundary

This single note is published on throwaway `research/wayfinder-gpu-preview` from committed baseline `0c7253979762db253fe528a9fee9bd59c7da457f`. A temporary alternate Git index is used so concurrent README/spec/plan/agent-doc edits and the real index are preserved. No production/prototype code, PR, merge, main branch or repository-default change is part of this research. The issue resolution comment is the canonical bounded answer; the parent map is updated separately by its driving agent.
