# OpenOwlCLI

Independent Qt / Qt Quick application under design. The recovered Owl C# runtime is the behavioral reference; this application will have its own project format.

The agreed domain vocabulary is in [GLOSSARY.md](GLOSSARY.md). The application uses a continuously rendering session, nested folders with effect stacks, and independent live and timeline property configurations. Timeline activation interpolates toward the moving cursor independently of playback.

Qt is the chosen application foundation, including the future Qt Quick GUI. A disposable architecture experiment is being developed on `prototype/qt-session`. Production architecture and implementation remain subject to design review.

The [proposed Qt core design](docs/superpowers/specs/2026-10-31-qt-core-design.md) records the source-grounded architecture, property-sync rules, independent scenes, Studio transitions, GPU preview requirements, and core interfaces. It has received a builder check and awaits user approval. The latest inspector/sync/Studio requirements are design inputs; they have not been added to the disposable prototype.
