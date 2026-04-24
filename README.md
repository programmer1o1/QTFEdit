# QTFEdit v3.5 (Qt GUI for VTFLib)

QTFEdit is the primary GUI frontend for VTFLib, implemented with Qt (Qt 6/5 Widgets) and intended to be cross-platform (Linux/macOS/Windows).

Release notes and the cumulative feature history live in [`CHANGELOG.md`](CHANGELOG.md). The VTFEdit Reloaded parity checklist is [`docs/parity.md`](docs/parity.md); the full forward roadmap is [`docs/roadmap.md`](docs/roadmap.md).

This repository contains:

- **VTFLib**: LGPL C/C++ library for reading/writing `.vtf`/`.vmt`.
- **VTFCmd**: GPL command-line frontend for VTFLib.
- **QTFEdit** (`VTFEdit/`): Qt GUI frontend for VTFLib.
- **Legacy WinForms GUI** (`legacy/`): frozen Windows-only VTFEdit Reloaded + Visual Studio solutions.

## Library/Author Information

* **Title**: VTFLib
* **Written In**: C/C++
* **Date**: July 25th, 2011
* **Authors**: [IAmKnotMax](https://github.com/WereTech), [Skyrym](https://github.com/Sky-rym), [misyltoad](https://github.com/misyltoad), [Neil 'Jed' Jedrzejewski](https://github.com/NeilJed) & [Ryan Gregg](https://nemstools.github.io/)

## Project Structure

* `VTFLib/` - VTFLib source (library).
* `VTFCmd/` - VTFCmd source (CLI).
* `VTFEdit/` - QTFEdit source (Qt GUI).
* `legacy/` - frozen WinForms GUI + Visual Studio solutions.
* `docs/` - documentation (including the parity checklist).

## How to Build

### Windows (legacy WinForms GUI)
The legacy WinForms GUI is frozen, but still buildable on Windows via the Visual Studio solutions under `legacy/sln/`.

### Linux (new, CLI + library)
Prerequisites: `cmake`, a C/C++ compiler (gcc/clang), and optionally AMD Compressonator (`libcompressonator`) for DXT compression. VTFCmd can use DevIL (OpenIL) if installed, or a bundled stb-based loader/writer backend.

Build steps:
```
mkdir -p build && cd build
cmake .. -G Ninja -DVTFLIB_USE_COMPRESSONATOR=ON   # OFF to build without Compressonator
cmake --build . --config Release
```
Outputs:
- `build/VTFLib/libvtflib.a` – static VTFLib suitable for Linux.
- `build/VTFCmd/vtfcmd` – command-line converter runnable on Linux.

Notes:
- If Compressonator is not found on your system, DXT compression/decompression will be disabled at runtime with a clear error message.
- VTFCmd image I/O backend is controlled by `-DVTFCMD_IMAGE_BACKEND=auto|devil|stb` (default `auto`). With `auto`, CMake will use DevIL if found, otherwise it will fall back to the bundled stb backend.
- The stb backend uses `thirdparty/include/stb_image.h` and `thirdparty/include/stb_image_write.h`.
- If you only want the library, configure with `-DBUILD_VTFCMD=OFF`.
- If you want the DevIL backend but CMake can’t find it, you can point it at your paths with `-DIL_INCLUDE_DIR=...` and `-DIL_LIBRARY=...` (for example `/usr/include` and `/usr/lib64/libIL.so` or `/usr/lib64/libIL.so.1`).
- The legacy WinForms GUI is in `legacy/` and remains Windows-only.

### Linux/macOS/Windows (new, Qt GUI)
Prerequisites: Qt 6 (or Qt 5) Widgets development packages and a C++ compiler.

Build steps (GUI only, no DevIL required):
```
cmake -S . -B build -G Ninja -DBUILD_VTFCMD=OFF -DBUILD_VTFEDIT_QT=ON -DVTFLIB_USE_COMPRESSONATOR=ON
cmake --build build --config Release
```
Outputs:
- `build/VTFEdit/vtfeditqt` – Qt-based VTF/VMT tool (Linux/Windows).
- `build/VTFEdit/vtfeditqt.app` – Qt-based VTF/VMT tool (macOS).

Notes:
- If you previously configured this repo when the Qt GUI lived under `VTFEditQt/`, delete your build directory and reconfigure.
- For a portable Windows folder (fixes missing `Qt6Core.dll` / MinGW runtime DLL errors), stage an install and run `windeployqt`:
  - `cmake --install build --config Release --prefix dist`
  - `windeployqt --release --no-translations --compiler-runtime dist/vtfeditqt.exe`
- For a portable macOS app bundle, stage an install and run `macdeployqt`:
  - `cmake --install build --config Release --prefix dist`
  - `macdeployqt dist/vtfeditqt.app -always-overwrite`
- Batch convert supports presets, preserves input folder structure under the output root, and can generate `.vmt` with `$basetexture` computed relative to a configurable `materials/` root.
- Create options include common VTF header flags (e.g. normal map, clamp, filtering) via the Qt GUI.
- Opened `.vtf` files can have header flags edited via the GUI and then saved back to disk.
- VTF Properties can edit start frame, bump scale, reflectivity, and can regenerate mipmaps/thumbnail.
- VMT creation wizard can generate a starter `.vmt` (and can derive `$basetexture` when the VTF path contains `materials/`).
- DXT formats (DXT1/3/5/ATI) require a compressor for import/export, and require Compressonator for viewing/decompression on non-Windows builds; without it only uncompressed formats will work reliably.
- Import supports PNG, JPG/JPEG, BMP, TIFF, WebP, GIF, PPM/PGM/PBM natively via Qt; TGA, Radiance HDR, Photoshop PSD (basic), and Softimage PIC via bundled `stb_image`; OpenEXR via tinyexr; and QOI via `qoi.h`. HDR sources are converted to 8-bit sRGB on import (clamp + 1/2.2 gamma) — use an HDR VTF format target if you need the full range.
- On Linux, install `packaging/linux/vtf.thumbnailer` + `packaging/linux/vtf-mime.xml` (or run `cmake --install` on a Linux build — the files ship to `share/thumbnailers` and `share/mime/packages`) to get `.vtf` thumbnails in Nautilus / Nemo / Thunar / Caja / PCManFM / any freedesktop-compliant file manager. See `packaging/linux/README.md` for step-by-step instructions.
- On Windows, `reg import packaging/windows/vtf-assoc.reg` (after editing the path inside) registers `.vtf` / `.vmt` with QTFEdit. See `packaging/windows/README.md`.
- `vtfcmd -exportpath <file>` — single-output path flag, used by shell thumbnailers that pass an exact destination via `%o`.
- `vtfcmd -mfilter nice` / Qt dialog "Nice (sRGB-aware box)" — Valve-style NICE mipmap filter that forces gamma-correct downsampling regardless of the `-srgb` flag.
- Opening a console VTF (Xbox 360 / PS3) or a Strata Source compressed VTF fails with a specific error pointing at `docs/roadmap.md` instead of a generic "corrupt file" — those formats are recognised but not yet supported.
- Animated GIF / APNG import auto-expands into Animated-VTF frames.
- A **Lit Normal** channel mode previews the RG components as a tangent-space normal map under a fixed directional light.
- A **Mip Diff** channel mode overlays a heatmap of how much a mip level differs from the next coarser one — useful for catching bad filter choices.
- `Tools → Create Cubemap From HDRI…` samples an equirectangular HDRI into a 6-face Environment-Map VTF (bilinear CPU sampler; 256/512/1024/2048 per face).
- The Create VTF dialog has a **preset bar** (combo + Save As… + Delete). Presets persist in QSettings and cover format, filter, resize, sRGB/gamma, and flags.
- Non-modal **toast notifications** for operational info/warn/error messages, replacing most `QMessageBox::information` popups.
- **Undo/redo** (Ctrl+Z / Ctrl+Shift+Z) — VTF flags, minor version, start frame, bumpmap scale, and reflectivity edits route through a `QUndoStack`; Properties-dialog multi-field edits are grouped into a single undo macro. The stack is cleared when a different VTF is opened.
- **Crash reporting** — on fatal signal / unhandled exception the app writes a minimal report (signal name + backtrace on POSIX, exception code + stack frame pointers on Windows) to `<AppDataLocation>/crashes/crash-<epochms>.txt` before the OS takes over. The handler is async-signal-safe on POSIX (plain `open`/`write`/`backtrace_symbols_fd`, no Qt or allocator calls). At next startup, any reports newer than the last-seen timestamp (stored in `QSettings`) surface as a **clickable** non-modal toast — clicking opens the crashes folder via `QDesktopServices`. Hovering the toast pauses auto-hide.
- Live Reload From Source (Tools menu, opt-in): drop an image to create a VTF, then enable live reload — saving the source in your editor automatically re-encodes the VTF in place. `Retune Live Source Options…` lets you change encoding settings mid-session.
- Command Palette (Ctrl+Shift+P) — fuzzy-find and run any menu action, with recency and use-count weighting so frequently-used commands surface first.

## Roadmap

VTFEdit Reloaded parity is complete (see [`docs/parity.md`](docs/parity.md)). The headline items still on the table:

- **Strata Source compressed VTF (Deflate / Zstd)** — Strata extends v7.4 / v7.5 with compressed image bodies. QTFEdit currently detects them and fails with a specific error; full read/write support is pending a published format spec.
- **Console VTF read (Xbox 360, PS3)** — big-endian VTFs with platform-specific tiling / swizzle. QTFEdit detects byte-swapped headers today and fails with a specific error; full support needs an endian abstraction across VTFLib plus 360 / Cell swizzle implementations.

Everything else — split-pane A/B compare, sheet-editor visual timeline, GPU preview path, Windows / macOS shell thumbnail providers, plugin API, extended undo coverage, accessibility & localization passes, and the `MainWindow.cpp` monolith breakup — is catalogued in [`docs/roadmap.md`](docs/roadmap.md).

## Testing

CTest-driven round-trip smoke tests live under `VTFCmd/tests/`. Configure and build, then:

```
cd build
ctest --output-on-failure
```

The suite currently covers:

- `vtfcmd_roundtrip_rgba8888` — RGBA8888, mipmaps on
- `vtfcmd_roundtrip_bgra8888` — BGRA8888, mipmaps on
- `vtfcmd_roundtrip_rgb888`   — RGB888 (no alpha), mipmaps on
- `vtfcmd_roundtrip_rgba8888_nomips` — RGBA8888 with `-nomipmaps`
- `vtfcmd_roundtrip_mipfilter_box` / `_triangle` / `_catrom` / `_mitchell` / `_kaiser` / `_point` / `_gaussian` / `_nice` — round-trip with each mipmap filter swapped in. All eight filters succeed thanks to the `VTFFilterToStbirFilter` mapping in `VTFLib/VTFFile.cpp` (prior to this fix, passing any filter past `MITCHELL` directly to `stbir_filter()` tripped an stbir assertion).
- `vtfcmd_roundtrip_animated` — builds a 2-frame animated VTF via the `mkanimvtf` helper (which calls `vlImageCreateMultiple` directly — `vtfcmd -file a -file b` produces separate single-frame VTFs, not a multi-frame one), validates the header has Frames=2, decodes back through `vtfcmd -extract-all-frames`, and pixel-diffs **every frame** against its reference input.
- `vtfcmd_roundtrip_cubemap` — builds a 6-face environment-map VTF (`mkanimvtf --cube`), validates the VTF header's `TEXTUREFLAGS_ENVMAP` bit is set, decodes back via vtfcmd, and pixel-diffs face 0.

Each test converts `resources/rgb.png` → `.vtf` → `.png` via `vtfcmd`, validates both outputs exist, parses the VTF header bytes to assert the `"VTF\0"` magic and the reported dimensions, and then runs the bundled `imgdiff` helper (stb_image-based) to compare the decoded PNG against a reference copy of the input. Pixels where both sides are fully transparent are ignored (RGB is visually undefined there). The quality gate is a minimum PSNR of 20 dB; the per-channel max-abs-diff gate is intentionally permissive because VTFLib's round-trip reshuffles colour in some alpha-edge pixels. DXT formats are intentionally not in the suite because Compressonator availability varies across platforms.

In addition to the VTFCmd suite, the CI `qt-gui` job runs a headless smoke step (`QT_QPA_PLATFORM=offscreen vtfeditqt --version && --help`) on all three OSes. The Qt GUI binary responds to `--version` / `-v` and `--help` / `-h` before `QApplication` is even instantiated, so the smoke step needs no platform plugin.

The suite totals **14 ctest scenarios**. Add further ones by calling `_add_roundtrip_test(<suffix> <format> "<extra>")` in `VTFCmd/CMakeLists.txt`. The CI `core` job runs `ctest` on every push.

## CI

GitHub Actions builds the core targets (`vtflib`, `vtfcmd`) on Ubuntu/Windows/macOS and the Qt GUI (`vtfeditqt`) on Ubuntu/Windows/macOS (Qt 6). The `core` job runs `ctest` (14 round-trip scenarios, PSNR-gated). The `qt-gui` job runs `vtfeditqt --version` / `--help` as a headless smoke test on all three OSes.

### Releases

Pushing a `v*` tag (e.g. `git tag -a v3.5.0 -m "…" && git push origin v3.5.0`) triggers a `release` job that downloads every platform artifact, re-packages each with a versioned name (`QTFEdit-<ver>-linux-x86_64.zip`, `QTFEdit-<ver>-windows-x86_64.zip`, `QTFEdit-<ver>-macos.zip`, plus `VTFLib-VTFCmd-<ver>-*` and `VTFEdit-Reloaded-<ver>-winforms-windows-x86_64.zip`), and opens a **draft** GitHub Release on the Releases tab with `CHANGELOG.md` as the body. Review and click Publish.

## Optional Dependencies (legacy/Windows)

- HLLib (optional): https://web.archive.org/web/20171114194253/http://nemesis.thewavelength.net/files/files/hllib246.zip
- DevIL (OpenIL, optional): https://sourceforge.net/projects/openil/files/DevIL%20Win32%20and%20Win64/DevIL-EndUser-x64-1.8.0.zip/download?use_mirror=phoenixnap

## VTFCmd Usage
Parameters:
```
 -file <path>             (Input file path.)
 -folder <path>           (Input directory search string.)
 -output <path>           (Output directory.)
 -exportpath <file>       (Exact output file path for single-image export; used by shell thumbnailers.)
 -prefix <string>         (Output file prefix.)
 -postfix <string>        (Output file postfix.)
 -version <string>         (Output version.)
 -format <string>          (Output format to use on non-alpha textures.)
 -alphaformat <string>     (Output format to use on alpha textures.)
 -flag <string>           (Output flags to set.)
 -resize                  (Resize the input to a power of 2.)
 -rmethod <string>        (Resize method to use.)
 -rfilter <string>        (Resize filter to use.)
 -rwidth <integer>        (Resize to specific width.)
 -rheight <integer>       (Resize to specific height.)
 -rclampwidth <integer>   (Maximum width to resize to.)
 -rclampheight <integer>  (Maximum height to resize to.)
 -alphathreshold <integer> (Alpha threshold for One Bit Alpha. Pixel alpha below this value is set to 0)
 -gamma                   (Gamma correct image.)
 -gcorrection <single>    (Gamma correction to use.)
 -nomipmaps               (Don't generate mipmaps.)
 -mfilter <string>        (Mipmap filter to use.)
 -nwrap                   (Wrap the normal map for tiled textures.)
 -bumpscale <single>      (Engine bump mapping scale to use.)
 -nothumbnail             (Don't generate thumbnail image.)
 -noreflectivity          (Don't calculate reflectivity.)
 -shader <string>         (Create a material for the texture.)
 -param <string> <string> (Add a parameter to the material.)
 -recurse                 (Process directories recursively.)
 -exportformat <string>   (Convert VTF files to the format of this extension.)
 -extract-all-frames      (Export all frames from VTF files with a _frameN suffix.)
 -extract-all-faces       (Export all cubemap faces with a _faceN suffix.)
 -extract-all-slices      (Export all volume-texture slices with a _sliceN suffix.)
 -extract-all-mips        (Export all mipmap levels with a _mipN suffix.)
 -extract-all             (Shorthand for all four -extract-all-* flags.)
 -silent                  (Silent mode.)
 -pause                   (Pause when done.)
 -help                    (Display vtfcmd help.)
```
Example vtfcmd usage:
```
vtfcmd.exe -file "C:\texture1.bmp" -file "C:\texture2.bmp" -format "dxt1"
vtfcmd.exe -folder "C:\input\*.tga" -output "C:\output" -recurse -pause
vtfcmd.exe -folder "C:\output\*.vtf" -output "C:\input" -exportformat "jpg"
```
## VTFEdit Reloaded Changelog
<details>
	
  v2.1.7
  - Warning and Notification Sounds are now enabled by default.
  - Added button to Help menu to pull up latest GitHub version in browser.
  - Fixed ordering of resize algorithms and exposed more options to user.
  - Upgraded [stb_image_resize.h](https://github.com/nothings/stb/blob/master/deprecated/stb_image_resize.h) to [stb_image_resize2.h](https://github.com/nothings/stb/blob/master/stb_image_resize2.h), ~~VTF creation is much faster now.~~ I mistook something else changing import times, times are pretty much the same lmao.
  - Mipmaps are now generated with STB2.
  - Upgraded Compressonator to fork [Yellow-Dog-Man/compressonator](https://github.com/Yellow-Dog-Man/compressonator).
	
  v2.1.6
  - Fixed multi-line comments in VMTs not being allowed to save or load.
	
  v2.1.5
  - Fixed reflectivity not being set correctly when saving a VTF.
	
  v2.1.4
  - Fixed mipmap generation still attempting to make mipmaps when it was disabled.
	
  v2.1.3
  - Disables mipmap generation with DXT formats that use Multiple of Four resizing as they do not work as expected.
	
  v2.1.2
  - Fix alpha detection method for batch converting.
	
  v2.1.1
  - Added an option to set a custom alpha background for images that have alpha. Both settings are in the Options tab at the top.
  - Fix DXT1 with One Bit Alpha format not being imported correctly or being functional in source games.
    - Instead, this format will be turned into DXT1 with the One Bit Alpha flag being enabled for it.
  - Added a new Advanced option to set the alpha threshold when creating one bit textures.
    - This will set pixels that have a lower numeric alpha value lower than this number to be 0, and any equal to or higher to be 255.
    - This option is only relevant to DXT1 with One Bit Alpha.
  - Added a parameter for VTFCmd for the alpha threshold when creating one bit textures.
  - Removed HLLib as this is unused after the removal of WAD convert. This removes the requirement of needing msvcr100.dll to run the program.
	
  v2.1.0
  - Fix batch create not setting the shader parameter to what you set for all vmts.
  - Fix an image format placement in code.
  - Fix One Bit Alpha flag not being kept on save.
  - Moved all system sounds to only being heard when both warning popups and notification sounds are enabled.
  - Changed the method that is used to determine whether an image should use Color or Alpha format.
  - It will now iterate through all pixels in the image and check their opacity. Previously this only looked at certain attributes of the image.

  v2.0.9
  - Releases now include VTFCmd.exe (not yet verified).
  - Added a saved parameter for the zoom level of the VMT editor (Ctrl+MWheel changes the zoom level for this).
  - Added a color panel that is used to view the representation of the reflectivity value and can be used to change that value when clicking on it.
  - Added option to enable/disable notification sounds.
  - Added option to enable/disable pop-up warnings when an error occurs or closing an opened file.
  - Added a total percentage label to the Batch Convert window.
  - Changed the default text when creating a new VMT to include a line for $basetexture.
  - Changed the image position for VTF files to be centered on the middle panel and scaled to a more appropriate size by default.
  - Optimized some functions.
  - Removed the tabs in favor of adding a panel to the right, as a lot of redundant information was removed.
  - Removed the file system ui elements.
  - Removed WAD convert due to known issues.
  - Fixed some conditional statements.
	
  v2.0.8
  - Fixed various issues with UI positioning and other functions not working.
  - Added a few more check boxes for VMT Create dialog.
	
  v2.0.7
  - Adjusted the Convert Folder/Convert WAD dialogs to allow for adjusting the created VMT file instead of defaulting to a bare-minimum file. There is a new button for this on the bottom-left next to the button that was used for only changing VTF options.
  - Forgot to add VTF flags functionality to WAD convert, this is added now.
	
  v2.0.6
  - Added an Edit Resources button for VTF files that are version 7.3 and above.
  - Added a Flags tab to the VTF import dialog. This also works when using the convert folder option and setting them within the Options button on the bottom-left of the dialog.
	
  v2.0.5
  - While importing an image, you can now select "Nearest Multiple of 4", which is the suggested bare-minimum for mipmaps to be generated properly.
  - If you have an image opened, you can now change the version by selecting the new one in the dropdown box in the Info tab, then Save.
  	- Keep in mind this will change the VTF size. So if you are using it for a spray decal, it could possibly go outside of the 512kb limitation when increasing the version.
  - The program's settings file is now contained in the same location that the program is in, instead of your Roaming folder.
  - The default version has also been changed to 7.4 as mostly commonly that will be the version used by source engine modders.
  - The context menu's options for VTF files has been slightly changed for more clarity.
  - The File menu has been slightly changed for more clarity and better positioning. Creating either VTF or VMT is now under the first option "Create New..."
  - Pulled request by goodusername123, which excludes the sRGB flag from VTFs (temporary measure).
  - Reverted the syntax highlighter due to performance and functional issues; related code remains commented out.
  - Added a way to change the framerate for animated playback (in milliseconds).
  - Fixed the import's resize filtering to use what you chose instead of being forced to use only one type of filter.
  - Adjusted the About dialog to include @WereTech and @Sky-rym as authors.

  v2.0.4
  - Changed the UI to use a better version of the tool bar and added more buttons for ease of access.
  - Adjusted several UI elements to stop text wrapping, causing the rest of the text to not be visible at all.
  - Adjusted some windows to now use a minimum window size, as this was not set before and you were able to resize these windows to be extremely small which also hid controls.
  - Added a Close option to both the File dropdown, and the new tool bar. The shortcut for this is Ctrl + Q. This will close the current file, which has a confirmation prompt so you don't close your file accidentally.
  - Changed the right-click context menu for VTFs to additionally show when clicking within the image window instead of needing to right-click on the image itself.
  - Changed the functionality of the Frame, Face, Slice, and Mipmap numeric bars to only be interactable if any of these properties have more than one to them. (in this case 0 = 1 because C++ starts at 0)
  - Adjusted some VTF flag names.

  v2.0.3
  - Updated the Shaders and Surface Properties list in VMT Create in accordance to the Valve Developer Wiki's current list
  - Added Drag and Drop functionality for images that can be imported normally through the File > Import dialog
	  - Note that if any window is above VTFEdit-Reloaded, or any part of the window is outside monitor bounds, the Import settings will default to the top-left of the screen. I do not have the knowledge to change this behavior myself unfortunately.
  - Changed the ReadOnly property in the Convert Folder and Convert WAD dialog, so you can manually insert or change the path to your folder instead of needing to use the Folder Selector button on the right.
  - Changed the resource files to use "winres.h" and "winver.h" instead of "afxres.h". Supposedly afxres.h is outdated, and has been replaced with the two other headers. So far I haven't noticed any issues with this.

  v2.0.2
  - Updated Compressonator to latest version as of 8/8/22.
  - Added 32-bit version.

  v2.0.1
  - Updated to HLLib v2.4.6.
  
  v2.0.0
  - Migrated to using AMD Compressonator for DXT compression.
  - Fix the green tinge when exporting textures
  - Respect sRGB-ness when generating mipmaps and resizing.
  - Ported to using the latest CLR and VS 2019.
  - Removed crufty sharpening filters (no longer needed with proper sRGB)
  - Use monospace font for VMT editor
  - High DPI support
  - Syntax highlighting support for VMTs
  - Fixed cubemap previews
    - Previously they were doing integer maths on what should be floats
    - Replaced overly complex controls with a simple exposure slider
    - Very simple Reinhard tonemapper for previewing exposure
</details>

## Program Copyright-Permissions

See the lgpl.txt (VTFLib) and gpl.txt (VTFCmd & VTFEdit) files contained in the distribution.
