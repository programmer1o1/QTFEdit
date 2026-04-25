# Changelog

## 3.5.1 — 2026-04-25

### VTFLib

- Fixed the root cause behind the Windows `core` CI failure on builds without Compressonator: high-level VTF creation no longer hard-fails while trying to generate the DXT1 low-res thumbnail. VTFLib now disables thumbnails by default on no-Compressonator builds and auto-drops thumbnail generation in the RGBA create path, so animated / cubemap VTF creation still succeeds.

### Testing + CI

- `mkanimvtf` no longer needs its own local thumbnail/DXT capability workaround because the fallback now lives in VTFLib itself. The Windows `core` matrix entry (`VTFLIB_USE_COMPRESSONATOR=OFF`) passes again.

## 3.5.0 — 2026-04-24

### Qt GUI (QTFEdit)

- **Live Reload From Source** (Tools menu, opt-in): when a VTF was created by importing an image, `QFileSystemWatcher` re-encodes and overwrites the VTF in place whenever the source changes. Captures the original `SVTFCreateOptions`, texture type, and alpha-format choice. Directory watching catches editors that atomic-save (write-tmp + rename). `Tools → Retune Live Source Options…` re-runs the Create dialog mid-session.
- **External VTF change detection** (always on): the open `.vtf` is watched on disk. Clean buffers auto-reload silently (preserving frame/face/slice/mip selection); dirty buffers get a status-bar warning. A 2-second self-save window suppresses feedback loops.
- **Discard Changes & Reload** (Shift+F5).
- **Command palette** (Ctrl+Shift+P) — fuzzy subsequence matching with start-of-string / word-boundary / consecutive-char bonuses; persistent recency + use-count scoring (3-day exp-decay half-life).
- **Lit Normal** channel-view mode — Lambertian shading over RG-as-normal with a fixed directional light.
- **Mip Diff** channel-view mode — heatmap of `|current mip − upscale(next-coarser mip)|`.
- **Mip quality stats** in the VTF Properties dialog — per-mip PSNR column companion to Mip Diff.
- **Non-modal Toast** overlay for info / warn / error status messages, supersedes most `QMessageBox::information` calls; repositions on window resize; supports a clickable variant with pointer-hover pause.
- **Undo / redo** (Ctrl+Z / Ctrl+Shift+Z) backed by `QUndoStack`. Covers VTF flags, minor version, start frame, bumpmap scale, reflectivity, and computed-reflectivity. Properties-dialog multi-field saves grouped as a single macro. Stack cleared on file close / open.
- **Equirectangular HDRI → cubemap** (`Tools → Create Cubemap From HDRI…`) — CPU bilinear sampler into a 6-face Env-Map VTF at 256 / 512 / 1024 / 2048 per face.
- **Export presets** in the Create VTF dialog (preset bar with combo + Save As… + Delete). Persists 20+ fields: format, mipmap filter, version, resize config, sRGB/gamma, full flag mask.
- **Per-file memory**: zoom level, fit-to-window state, and cubemap face are remembered per VTF.
- **Reopen last session on startup** (opt-in).
- **Auto-fit preview on open** (opt-in).
- **Extended image import**: PNG/JPG/BMP/TIFF/WebP/GIF/PPM/PGM/PBM via Qt; **TGA** / Radiance **HDR** / Photoshop **PSD** / Softimage **PIC** via bundled `stb_image`; **OpenEXR** via tinyexr; **QOI** via `qoi.h`. Animated **GIF** / **APNG** auto-expand into Animated-VTF frames.
- **Crash reporting** — async-signal-safe POSIX handler (or `SetUnhandledExceptionFilter` on Windows) writes a minimal report with backtrace to `<AppDataLocation>/crashes/crash-<epochms>.txt`. On next startup, any new reports surface as a **clickable** non-modal toast that opens the folder via `QDesktopServices`.
- **Qt GUI `--version` / `--help`** flags short-circuit before `QApplication` so the binary can be smoke-tested headlessly without a platform plugin.
- **macOS file associations**: `CFBundleDocumentTypes` + `UTExportedTypeDeclarations` in a custom `Info.plist.in`; `main.cpp` installs a `QFileOpenEvent` filter for Finder "Open With".

### VTFLib

- **Kaiser / Gaussian / Point / etc. mipmap filter crash fixed**: `stbir_filter(MipmapFilter)` was a plain C-style cast across two incompatible enums. Out-of-range values past `MIPMAP_FILTER_MITCHELL` tripped an internal `stbir__perform_build` assertion at mipmap-generation time. Replaced with a `VTFFilterToStbirFilter` helper that explicitly maps each VTFLib filter to the closest stbir equivalent (MITCHELL for the windowed-sinc family, TRIANGLE for Gaussian/Bessel, etc.). Applied to all three `stbir_resize` call sites.
- **`MIPMAP_FILTER_NICE`** — new enum value for a Valve-style half-pixel box filter that forces `STBIR_TYPE_UINT8_SRGB` in the mipmap generation path regardless of the caller's `bSRGB` flag. Exposed via `vtfcmd -mfilter nice` and the Qt Create dialog's combo (**"Nice (sRGB-aware box)"**).
- **Clearer errors for unsupported VTF variants** — `CVTFFile::Load` now detects byte-swapped headers (big-endian console VTFs — Xbox 360 / PS3) and future Strata minor versions, and fails with a specific error pointing at `docs/roadmap.md`.
- Fixed `-Wswitch` warning in the resize-method switch by handling the `RESIZE_COUNT` sentinel explicitly.

### VTFCmd

- **`-exportpath <file>`**: single-output path flag for tools (like shell thumbnailers) that need a specific destination file. Only honoured without `-extract-all-*` to prevent overwriting the same file N times.

### Testing + CI

- **CTest round-trip suite**: 14 scenarios — three formats (RGBA8888 / BGRA8888 / RGB888), a `-nomipmaps` variant, eight mipmap-filter variants (box / triangle / catrom / mitchell / kaiser / point / gaussian / nice), an animated round-trip with per-frame diff, and a 6-face cubemap round-trip. Each test validates VTF header fields (magic, width/height, Frames field for animated, `TEXTUREFLAGS_ENVMAP` for cubemap) and pixel-diffs the decoded PNG via the bundled `imgdiff` helper (PSNR ≥ 20 dB gate, fully-transparent pixels ignored).
- New **`imgdiff`** test helper (stb_image-based pixel diff).
- New **`mkanimvtf`** test helper — builds multi-frame or cubemap VTFs via `vlImageCreateMultiple` (vtfcmd's `-file` flag can't do that directly). `--cube` flag for 6-face env maps.
- **CI**: `core` job runs `ctest` on every push (Ubuntu / Windows / macOS). `qt-gui` job runs `vtfeditqt --version` + `--help` as a headless smoke test on all three platforms.

### Shell integration

- **Linux** (freedesktop): `packaging/linux/vtf.thumbnailer` + `vtf-mime.xml`. `cmake --install` deploys them to `share/thumbnailers` and `share/mime/packages`. `.vtf` files get thumbnails in Nautilus / Nemo / Thunar / Caja / PCManFM.
- **Windows**: `packaging/windows/vtf-assoc.reg` + uninstaller. Registers `.vtf` / `.vmt` with QTFEdit ProgIDs under `HKEY_CURRENT_USER` (no admin).
- **macOS**: file associations already shipped via `Info.plist.in`; Finder thumbnails still need a Quick Look app extension (not shipped).

### Cleanup

- Removed unused `applyExposure8bit` and `leadingSpacesCount` functions from `MainWindow.cpp`.

### Docs

- `docs/parity.md` — VTFEdit Reloaded parity checklist now with a "Beyond Parity (Qt-only enhancements)" section covering every item above.
- `docs/roadmap.md` — detailed roadmap covering the big-ticket additions (Strata compressed VTF, console VTF read for Xbox 360 / PS3), shell integration follow-ups, Qt GUI quick / medium / ambitious items, and infrastructure work.
- `README.md` — expanded feature bullets; added Testing, Roadmap (Pending), and packaging sections; points at both `docs/*.md`.
- `packaging/linux/README.md` and `packaging/windows/README.md` — step-by-step install/uninstall.
