# QTFEdit Roadmap

This tracks work beyond the current VTFEdit Reloaded parity (which is complete — see [`parity.md`](parity.md)). Items are rough difficulty buckets. The README has a short highlights summary pointing here for the full detail.

## Big-ticket additions

### Strata Source compressed VTF read/write

Strata extends v7.4 / v7.5 with a compressed-body variant where the image data is deflate- or zstd-packed after the header. Currently QTFEdit **detects** VTF minor versions beyond the supported max and fails with a specific error instead of a generic corruption message. Full support still requires:

- A new flag / version field in `SVTFHeader_75+` indicating the compression used.
- Header-size / offset adjustments to read the compressed region and decompress on Load.
- Symmetric compression on Save, gated behind a `bCompress` option in `SVTFCreateOptions`.
- Library dependencies: `zlib` (already pulled in via tinyexr) and optionally `zstd`.
- Round-trip test harness.

Not attempted until a format spec is either documented or received.

### Console VTF read (Xbox 360, PS3)

VTFs produced for Xbox 360 and PS3 targets are big-endian and use platform-specific texture tiling / swizzle. QTFEdit now **detects** byte-swapped headers (Version field reads > 100 on a little-endian host) and fails with a specific error. Full support still requires:

- **Endian abstraction** across VTFLib's header / resource-record readers. VTFLib's current `Float16.h` only has endian constants; everything else assumes little-endian structs.
- **Xbox 360 texture swizzle / untiling**: the 360 stores textures in a tiled layout that must be reversed on load and re-applied on save.
- **PS3 swizzle**: similar story with Cell-specific swizzle.
- **Format variants** that differ from PC DXT (some consoles premultiply alpha, use different block ordering, etc.).

Even minimum viable read support is weeks of work and needs sample files plus spec/SDK access that isn't public.

## Shell integration

- **Windows `IThumbnailProvider` COM DLL** — Linux freedesktop thumbnailer ships (`packaging/linux/`); Windows has file associations (`packaging/windows/`) but no Explorer thumbnails. A separate COM/ATL project — likely a new Windows-only CMake target — would close that.
- **macOS Finder thumbnails** — file associations ship in the bundle plist; Quick Look / Thumbnail Provider app extension is a separate Xcode target.

## Qt GUI — quick wins (hours–days)

- **Split-pane A/B compare** — extract the preview widget, drop two into a `QSplitter` with synced zoom/pan.
- **Per-mip diff: extra stats** — the Properties dialog already shows numeric PSNR per mip alongside the Mip Diff heatmap. Follow-ups: RMSE rows, per-channel breakdown, line chart over mip level.
- **Route more popups through toast** — several `showWarningPopup` / `showErrorPopup` sites could be demoted to toasts for non-blocking operational messages (see `MainWindow.cpp` resource import/export paths).
- **DXT round-trip tests** — gated on Compressonator availability across CI runners; covered at runtime, not in CTest yet.
- **Non-square / NPOT round-trip test inputs** — exercise the resize path with a 48×32 or 33×17 asset.

## Qt GUI — medium (weeks)

- **Extend undo/redo coverage** — the stack is in place (flags, minor version, start frame, bumpmap scale, reflectivity, compute-reflectivity already covered). Remaining sites: mipmap regeneration, thumbnail regeneration, resource add/remove/replace, sheet edits.
- **Sheet editor visual timeline** — replace the table UI in `VtfSheetDialog` with a `QGraphicsView` thumbnail strip; drag-to-reorder frames.
- **Command palette: actionable help pages** — add non-action entries (e.g. "Open Parity Checklist") so the palette doubles as discovery.
- **Cubemap-from-HDRI: per-face preview + seam filter** — current sampler is bilinear only; add optional seamless cube filter and a 6-face preview grid before committing.
- **Headless GUI end-to-end smoke** — current smoke is just `--version` / `--help`. Extend to `QT_QPA_PLATFORM=offscreen vtfeditqt path/to/sample.vtf --exit-after-load` (flag doesn't exist yet) to exercise the real load/render path under a platform plugin.

## Qt GUI — ambitious (months)

- **GPU preview path (`QOpenGLWidget` + shaders)** — unlocks real mip LOD selection, anisotropic sampling, and a live lit normal-map gizmo. Prerequisite for any GPU-accelerated preview features.
- **Plugin API** — `QPluginLoader`-based interface with optional scripting bridge (Lua / `QJSEngine`) so custom resource types can ship editors without patching the tree.
- **Non-destructive `.qtfproj` edit graph** — rearchitect mutation sites to emit serializable ops; the `.vtf` becomes derived output.

## Infrastructure & quality

- **Breakpad / crashpad full pipeline** — a native signal/SEH handler ships today (writes to `<AppDataLocation>/crashes/` with backtrace), with a clickable toast on next startup that opens the folder. A full minidump library with symbol-server upload would be a real follow-up.
- **Accessibility pass** — tab order, screen-reader labels, keyboard-only navigation.
- **Localization scaffolding** — `tr()` wrapping so translations are possible later.
- **Extract `PreviewView` + `LiveReloadManager` from `MainWindow.cpp`** — the monolith is now ~5k lines; splitting it would halve that and make preview logic unit-testable.
