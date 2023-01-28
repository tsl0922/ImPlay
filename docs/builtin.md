# Built in commands

ImPlay has builtin commands that can be called with `script-message-to`.

This is useful for custom input bindings and scripts.

### Syntax

```
script-message-to implay <command> <args>..
```

For example, run `script-message-to implay command-palette playlist` will show command palette with playlist
(You can test it with `Tools - Metrics & Debug`).

### List of commands:

- `open`: show the open files dialog
- `open-folder`: show the open folder dialog
- `open-disk`: show the open DVD/Blu-ray folder dialog
- `open-iso`: show the open DVD/Blu-ray iso dialog
- `open-clipboard`: Open URL From Clipboard
- `open-url`: show the open URL dialog
- `load-sub`: show the load subtitles dialog
- `load-conf <path>`: load mpv.conf, path should be absolute
- `playlist-add-files`: show the playlist add files dialog
- `playlist-add-folder`: show the playlist add folder dialog
- `command-palette <args>..`: show command palette, args can be:
  - `bindings`: show input bindings
  - `playlist`: show playlist
  - `chapters`: show chapters
  - `tracks <audio|video|sub>`: show tracks
- `quickview <tab>`: show quick panel, tab can be:
  - `playlist`
  - `chapters`
  - `video`
  - `audio`
  - `subtitle`
- `metrics`: show Metrics & Debug window
- `settings`: show Settings window
- `about`: show About window
