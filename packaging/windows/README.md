# Windows packaging bits

## File associations for `.vtf` / `.vmt`

`vtf-assoc.reg` registers both extensions with a ProgID (`QTFEdit.VtfFile`, `QTFEdit.VmtFile`) so that:

- Double-clicking a `.vtf` or `.vmt` opens it in QTFEdit.
- "Open With → QTFEdit" appears for both extensions even if another app is the default handler.
- Content type / PerceivedType are set so Explorer treats `.vtf` as an image and `.vmt` as text.

### Install

1. Edit the `DefaultIcon` and `shell\open\command` values in `vtf-assoc.reg` so the paths match your actual `vtfeditqt.exe` location. The default assumes `C:\Program Files\QTFEdit\vtfeditqt.exe`.
2. Either double-click the file (Windows will prompt for elevation if needed) or run:
   ```
   reg import vtf-assoc.reg
   ```
3. Re-run if you move or reinstall QTFEdit.

### Uninstall

```
reg import vtf-assoc-uninstall.reg
```

or open Settings → Apps → Default Apps and clear the associations manually.

## What's **not** here

- **Thumbnail provider**. File Explorer thumbnails need a separate shell-extension DLL implementing `IThumbnailProvider`, registered with `regsvr32`. That is not shipped here — see `docs/roadmap.md` for the scope of that work.

The provided `.reg` uses `HKEY_CURRENT_USER` so no admin rights are required. To make associations system-wide, copy the keys under `HKEY_LOCAL_MACHINE\Software\Classes` instead (requires elevation).
