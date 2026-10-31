# Architecture spike debug UI

Disposable diagnostic surface; these are not approved production GUI decisions.

The question is whether one running Qt session can own a shared folder hierarchy, independent live/timeline property configurations, and a moving timeline activation target while independent clients inspect and edit it.

Use a compact desktop instrument layout: shared scene structure at left, output and the two source configurations in the center, side-by-side property editing at right, and independent playback/activation controls below. Two colors carry domain meaning: cyan for live values and amber for timeline values.

Runtime tokens live in the root properties of Main.qml: background #171f2c, panel #222d3e, text #e7ecf4, secondary text #acb8ca, live #63c9cf, timeline #f0ad66. Segoe UI is the application face; Cascadia Mono is used for numeric telemetry. Qt Quick Controls Basic owns buttons, sliders, focus behavior, and scrolling.

The interface is English, intended for desktop debugging. No persistence or destructive file operations are exposed. Generated shapes and a moving engine-clock marker are intentional fixtures. The UI is a client; the session remains running when it closes.
