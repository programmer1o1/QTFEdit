# Linux packaging bits

## File-manager thumbnails for `.vtf`

The freedesktop thumbnail spec requires two pieces:

1. A MIME type for `.vtf` so the file manager recognises the format.
2. A `.thumbnailer` entry that tells the file manager how to render a preview.

### Install (per-user)

```sh
# 1. MIME type — so .vtf files get classified as image/x-vtf.
mkdir -p ~/.local/share/mime/packages
install -m 0644 packaging/linux/vtf-mime.xml ~/.local/share/mime/packages/
update-mime-database ~/.local/share/mime

# 2. Thumbnailer — needs vtfcmd on PATH (or edit TryExec/Exec in the .thumbnailer).
mkdir -p ~/.local/share/thumbnailers
install -m 0644 packaging/linux/vtf.thumbnailer ~/.local/share/thumbnailers/
```

Then restart your file manager (or log out / back in) so the thumbnailer daemon picks up the new entries. Nautilus, Nemo, Caja, Thunar, PCManFM, and most other freedesktop-compliant file managers honour this spec.

### Install (system-wide)

Replace `~/.local/share` with `/usr/share` in the commands above. Requires root.

### How it works

The `.thumbnailer` invokes:

```
vtfcmd -file %i -exportformat png -exportpath %o -silent
```

`%i` is the input `.vtf` path and `%o` is the PNG path the file manager expects. `-exportpath` is a VTFCmd flag specifically for single-file tools like shell thumbnailers — without it, vtfcmd derives the output name from the input and drops it in `-output`, which does not match freedesktop's convention.
