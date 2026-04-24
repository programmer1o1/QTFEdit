# QTFEdit Parity Checklist (vs. Windows VTFEdit Reloaded)

Last updated: 2026-04-24

This is a living checklist intended to track feature parity between:

- **Windows (legacy)**: `legacy/VTFEdit-WinForms/` (WinForms / C++/CLI, “VTFEdit Reloaded”)
- **Qt**: `VTFEdit/` (cross‑platform, QTFEdit)

It’s based on a source scan of the Windows UI strings/handlers (primarily `legacy/VTFEdit-WinForms/VTFEdit.h`, plus `legacy/VTFEdit-WinForms/VTFOptions.h`, `legacy/VTFEdit-WinForms/VTFResources.h`, `legacy/VTFEdit-WinForms/VMTCreate.h`, `legacy/VTFEdit-WinForms/BatchConvert.h`) and the current Qt implementation. Some items may still need runtime verification.

## Legend

- **[DONE]** Implemented in QTFEdit (Qt) with comparable behavior.
- **[PARTIAL]** Implemented, but missing notable behavior/UI, or workflow differs.
- **[MISSING]** Not implemented in QTFEdit (Qt).
- **[UNKNOWN]** Not confirmed (needs testing or deeper audit).

## File / Project

- **[DONE]** Create new VTF (from image import) + create options (`VTFEdit/CreateVtfDialog.*`).
- **[DONE]** Create new VMT (basic) (`VTFEdit/CreateVmtDialog.*`).
- **[DONE]** Open VTF / open VMT (`VTFEdit/MainWindow.*`).
- **[DONE]** Recent VTF/VMT lists + clear recent (`VTFEdit/MainWindow.*`).
- **[DONE]** Save VTF / Save As VTF (`VTFEdit/MainWindow.*`).
- **[DONE]** Save VMT / Save As VMT (`VTFEdit/MainWindow.*`).
- **[DONE]** Drag & drop `.vtf`, `.vmt`, and images (`VTFEdit/MainWindow.*`).
- **[DONE]** “Recent Files” parity:
  - Qt now exposes a single `Open Recent Files` menu that mixes `.vtf` + `.vmt` in MRU order, with a single clear action and a configurable limit (default **8**, max **32**) (`VTFEdit/MainWindow.*`).

## Import / Export

- **[DONE]** Export current preview to PNG (`Export PNG…`).
- **[DONE]** Export thumbnail PNG (when present).
- **[DONE]** Export image “as” (via Qt image writers).
- **[DONE]** Export All parity (Windows exports **all frames + faces + slices** for a chosen mip in one action):
  - Qt provides: **Export All Frames**, **Export All Faces**, **Export All Slices**, **Export All Mipmaps**.
  - Qt also provides **Export All (Frames×Faces×Slices)** which prompts for the mip level.

## Clipboard

- **[DONE]** Copy image to clipboard (current view).
- **[DONE]** Paste image from clipboard to create a new VTF (`Edit → Paste Image as New VTF…`; Ctrl+V in the viewer).

## Viewer / Preview

- **[DONE]** Channel view (RGBA/RGB/R/G/B/A).
- **[DONE]** “Mask” toggle parity:
  - Qt “Mask” (Ctrl+W) now toggles **alpha compositing** of the current channel view over the selected background, matching Windows semantics (`VTFEdit/MainWindow.*`).
- **[DONE]** “Tile” toggle (Windows `View → Tile`, Ctrl+T).
- **[DONE]** Alpha background controls (Checker/Black/White/Custom color).
- **[DONE]** Zoom in/out/reset + fit‑to‑window.
- **[DONE]** Zoom to cursor (Ctrl+wheel) + panning (middle drag / Alt+drag).
- **[DONE]** Frame/Face/Slice/Mip selection.
- **[DONE]** Animated textures playback (basic).

## HDR / Float Preview

- **[DONE]** Exposure + tonemapped preview for float/HDR formats (ACES/Reinhard/Clamp) + cubemap preview support.
- **[DONE]** Windows parity mode:
  - Qt exposes a `VTFLib (Windows)` HDR preview mode that uses `VTFLib`’s FP16 exposure (`VTFLIB_FP16_HDR_EXPOSURE`) on the same **0.00–80.00** scale as Windows VTFEdit (default **20.00**) (`VTFEdit/MainWindow.*`, `legacy/VTFEdit-WinForms/VTFEdit.h`).
  - Qt still offers additional preview-only tonemap operators (ACES/Reinhard/Clamp) as an optional enhancement.

## VTF Properties / Editing

- **[DONE]** Edit VTF flags.
- **[DONE]** Edit minor version (VTF v7.x minor).
- **[DONE]** Start frame / bumpmap scale / reflectivity editing + “compute reflectivity”.
- **[DONE]** Generate mipmaps (selectable filter) and generate thumbnail.
- **[DONE]** Generate normal map (current/all frames) and generate sphere map.

## VTF Resources

- **[DONE]** Resource list/tree view.
- **[DONE]** View any resource as hex.
- **[DONE]** Export/import/replace/remove any resource as raw bytes.
- **[DONE]** Structured editors for common resources:
  - CRC
  - LOD settings
  - Texture settings (extended)
  - Key/Value data (KVD) viewer + VMT parsing display
- **[DONE]** Sheet resource decode + editor (v0/v1) + apply back into VTF.
- **[DONE]** “Information Resource” UI parity:
  - Qt now exposes an **Information** editor for the KVD resource (reads/writes the Windows `"Information"` schema) while still retaining a raw editor (`VTFEdit/VtfResourcesDialog.*`).

## VMT Editing / Workflow

- **[DONE]** VMT editor dock + save/save as.
- **[DONE]** Validate VMT (loose + strict) + optional live validation status.
- **[DONE]** VMT syntax highlighting (optional).
- **[DONE]** Find / Replace / Go to line.
- **[DONE]** “Open matching VMT” from a VTF (basic `$basetexture` heuristic) and “Open linked VTF” from a VMT.
- **[DONE]** “Create VMT File” parity:
  - Windows has a dedicated, feature-rich “Create VMT File” dialog (`legacy/VTFEdit-WinForms/VMTCreate.*`).
  - Qt mirrors the Windows dialog’s common texture slots/flags (including `$envmapmask`, `$detail`, `$normalmap`, `$dudvmap`, secondary base/bump, `%tooltexture`, `%keywords`, and the common boolean flags) (`VTFEdit/CreateVmtDialog.*`).

## Batch Convert

- **[DONE]** Convert folder (batch convert) workflow exists in Qt.
- **[DONE]** UI parity:
  - Qt batch convert now supports an optional **alpha format** selected per-image based on detected transparency, and can optionally create LOD clamp + Information (KVD) resources during conversion (`VTFEdit/BatchConvertDialog.*`, `VTFEdit/BatchConvertRunDialog.*`).

## App Options / Misc

- **[DONE]** Global “Options” dialog parity:
  - Qt has a unified options dialog (Tools → Options → Global Options…) and uses it as the source of global defaults/templates (`VTFEdit/OptionsDialog.*`).
  - Qt now mirrors the Windows format list (27 entries) and exposes “Texture Type” (Animated / Environment Map / Volume Texture) in both Global Options and the Create VTF Options dialog (`VTFEdit/OptionsDialog.*`, `VTFEdit/CreateVtfDialog.*`, `legacy/VTFEdit-WinForms/VTFOptions.h`).
- **[DONE]** “Warning Popups” toggle.
- **[DONE]** “Notification Sounds” toggle.
- **[DONE]** “Check For Updates”.
- **[DONE]** “Auto Create VMT File”.

## Beyond Parity (Qt-only enhancements)

- **Live Reload From Source** (`Tools → Live Reload From Source`):
  - When a VTF is created by importing image(s), QTFEdit captures the source paths + `SVTFCreateOptions` + texture-type + alpha-format choices.
  - With the toggle on, source files and their parent dirs are watched via `QFileSystemWatcher`. Saving the source in an external editor (including atomic write-tmp + rename) transparently re-encodes and overwrites the VTF in place, preserving current frame/face/slice/mip selection. Status bar shows `Live: N`.
  - `Tools → Retune Live Source Options…` re-runs the Create dialog and rebuilds once with new encoding parameters (useful for trying a different format or mip filter mid-session).
- **External VTF change detection** (always on):
  - The open `.vtf` is watched on disk. If another tool modifies it and your buffer is clean, QTFEdit silently reloads. If your buffer is dirty, a status-bar warning appears instead of a modal popup. A 2-second self-save window suppresses feedback loops from our own `vlImageSave`.
- **Discard Changes & Reload** (`Shift+F5`) — force-reload from disk, discarding any unsaved edits.
- **Command Palette** (`Ctrl+Shift+P`) — fuzzy subsequence matching with start-of-string / word-boundary / consecutive-char bonuses, plus persistent recency + use-count scoring so your frequent commands surface first. Shows shortcut, skips disabled/separator/submenu entries, Up/Down/Enter keyboard navigation.
- **Extended image import**: PNG/JPG/BMP/TIFF/WebP/GIF/PPM/PGM/PBM natively via Qt; **TGA**, **Radiance HDR**, **Photoshop PSD**, **Softimage PIC** via bundled `stb_image`; **OpenEXR** via tinyexr; **QOI** via `qoi.h`. Animated **GIF/APNG** are auto-exploded to frames for Animated-VTF creation. Works for drag-drop, Import dialog, and Live Reload sources.
- **Lit Normal preview** — a new channel-view mode shades RG as tangent-space normal XY under a fixed directional light; catches bad normal maps at a glance.
- **Mip Diff overlay** — channel-view mode that renders a punchy heatmap of `|current mip − upscale(next-coarser mip)|`, making bad mipmap filter choices visible.
- **Export presets** — CreateVtfDialog has a preset bar (combo + Save As… + Delete). Preset names and values are persisted via QSettings under `presets/export/*` and cover format, mipmap filter, version, resize, sRGB/gamma, and the full flag mask.
- **Equirectangular HDRI → cubemap** (`Tools → Create Cubemap From HDRI…`) — bilinear CPU sampler ingests a single equirect image (any supported format, HDR encouraged) and produces a 6-face Environment-Map VTF at 256 / 512 / 1024 / 2048 per face.
- **Non-modal toast notifications** — operational info/warn/error messages fade into the bottom-right corner instead of blocking with a modal popup. Repositions on window resize; status bar still mirrors the text.
- **Undo / redo** (Ctrl+Z / Ctrl+Shift+Z) — `QUndoStack`-backed, covers VTF flags, minor version, start frame, bumpmap scale, and reflectivity edits today. Properties-dialog multi-field saves are grouped as a single macro so one Undo reverts the whole edit.
- **CTest round-trip suite** — `ctest` runs thirteen tests: three formats (RGBA8888 / BGRA8888 / RGB888), a `-nomipmaps` variant, seven mipmap-filter variants (all healthy after `VTFFile.cpp` got a real `VTFMipmapFilter → stbir_filter` mapping), a 2-frame animated round-trip with per-frame pixel diff, and a 6-face cubemap round-trip. The multi-image cases are built via a dedicated `mkanimvtf [--cube]` helper that calls `vlImageCreateMultiple`. Each test validates the produced VTF header (magic, dimensions, Frames field for animated, `TEXTUREFLAGS_ENVMAP` bit for cubemap) and pixel-diffs the decoded PNG via the bundled `imgdiff` helper (PSNR ≥ 20 dB gate, fully-transparent pixels ignored). CI's `core` job runs the suite on every push across three OSes.
- **Crash reporting** — fatal-signal handler (POSIX) / unhandled-exception filter (Windows) writes a minimal report with backtrace into `<AppDataLocation>/crashes/` before default OS handling takes over. POSIX path is async-signal-safe. At next startup, any reports newer than the last-seen timestamp surface as a **clickable** non-modal toast that opens the crashes folder; hovering it pauses auto-hide.
- **Mip quality stats in VTF Properties** — for VTFs with more than one mip, the Properties dialog shows PSNR (in dB) between each mip and the upscaled next-coarser mip, giving a numeric companion to the Mip Diff heatmap in the preview.
- **Headless Qt GUI smoke test in CI** — `vtfeditqt --version` / `--help` short-circuit before `QApplication` so the CI `qt-gui` job can smoke-test the binary without a platform plugin. Runs post-build on Ubuntu / Windows / macOS.
- **Per-file view memory** — zoom level, fit-to-window state, and cubemap face are remembered per VTF and restored on reopen.
- **Reopen last session on startup** (opt-in option).
- **Auto-fit preview on open** (opt-in option).
- **macOS file associations** — `.vtf` and `.vmt` registered via bundle `CFBundleDocumentTypes` + `UTExportedTypeDeclarations`; Finder "Open With" routes through `QFileOpenEvent`.
- **Linux file-manager thumbnails** — `packaging/linux/vtf.thumbnailer` + `vtf-mime.xml` give any freedesktop-compliant file manager (Nautilus / Nemo / Thunar / Caja / PCManFM / …) `.vtf` thumbnails out of the box. The thumbnailer drives `vtfcmd -file %i -exportformat png -exportpath %o`; `-exportpath` is a new VTFCmd flag specifically for single-output shell tools. CMake installs both files under `share/thumbnailers` and `share/mime/packages` on Linux.
- **Windows file associations** — `packaging/windows/vtf-assoc.reg` + `vtf-assoc-uninstall.reg` register `.vtf` / `.vmt` with a QTFEdit ProgID (no admin required — uses HKCU). Thumbnails still need a separate COM DLL; see `docs/roadmap.md`.
- **NICE mipmap filter (sRGB-aware box)** — `MIPMAP_FILTER_NICE` in VTFLib; `vtfcmd -mfilter nice`; "Nice (sRGB-aware box)" in the Qt Create dialog. Forces `STBIR_TYPE_UINT8_SRGB` datatype in mipmap generation so downsampling is gamma-correct even when the caller didn't set `bSRGB`.
- **Clearer errors for unsupported VTF variants** — `CVTFFile::Load` now detects byte-swapped headers (big-endian console VTFs — Xbox 360 / PS3) and future Strata minor versions, and fails with a specific error pointing at `docs/roadmap.md` instead of a generic "corrupt file."
- Cross-platform support (Linux/macOS/Windows) — Windows VTFEdit Reloaded is Windows-only.
- Zoom-to-cursor (Ctrl+wheel) and middle-click / Alt+drag panning.
- Live VMT validation.
- Extra HDR preview tonemappers (ACES / Reinhard / Clamp) alongside the Windows-parity FP16 exposure mode.
- Working WAD2/WAD3 → VTF conversion (disabled in Windows VTFEdit).

## Deprecated / Broken in Windows (Not Targeted)

- **[DONE]** WAD convert:
  - Windows sources contain a WAD conversion dialog but it appears disabled/commented (“conversion is broken”).
  - Qt provides a working WAD2/WAD3 → VTF conversion workflow via **Tools → WAD Convert…** (`VTFEdit/WadConvertDialog.*`, `VTFEdit/WadFile.*`).

## Next Suggested Work (to reach “complete”)

No remaining `[PARTIAL]` / `[MISSING]` items in this checklist.
