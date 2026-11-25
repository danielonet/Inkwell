# Inkwell

A native Haiku text editor for Markdown, RTF and plain text, built around a
sidebar of linked files and folders. Files open in tabs.

**Status:** early, builds untested on real Haiku so far.

- Open a folder; the sidebar loads folders lazily and follows changes on disk.
- Files open in tabs; unsaved changes are marked and prompted for.
- **Markdown** (`.md`): live preview beside the editor (View ▸ Markdown
  preview, ⌘P). Links are clickable: web/mail links open in the preferred
  app, relative links open the file in a new tab.
- **RTF** (`.rtf`): edited as rich text, Format menu for bold (⌘B), italic
  (⌘I), underline (⌘U) and size (⌘+ / ⌘-). Fonts families, paragraph
  formatting, images and tables are not preserved.
- **Themes** (View ▸ Theme): System colors, Light, Dark, Sepia. Applies to the
  editor and preview, not to the sidebar or window chrome; remembered in
  `~/config/settings/Inkwell_settings`.

## Build (on Haiku)

Requires the `haiku_devel` and `makefile_engine` packages.

    make
    ./objects.*/Inkwell

## Tests

The Markdown renderer and the RTF reader/writer have no Haiku dependencies
and are tested anywhere:

    make -C tests test

## Layout

- `src/` – C++ sources (Interface Kit, Locale Kit); `MarkdownRender.*` and `Rtf.*` are pure C++
- `Inkwell.rdef` – app signature (`application/x-vnd.Inkwell`), version, icon
- `Makefile` – uses Haiku's makefile-engine
- `packaging/inkwell-0.1.0.recipe` – draft HaikuPorts recipe
- `artwork/` – icon PNGs (master 512px + 256/128/64/32/16); HVIF still to be traced
- `locales/` – translation catalogs (`.catkeys`), added later

## Roadmap

- Markdown: tables, nested quotes, list continuation lines
- RTF: font families, text color and alignment commands
- Themes: sidebar and window chrome, more palettes
- Hebrew translation

## Publishing to HaikuDepot

Tag a release (`v0.1.0`), put the tarball's SHA-256 in the recipe, copy it to
`haiku-apps/inkwell/` in a fork of `haikuports/haikuports`, test with
`haikuporter`, then open a pull request.

## License

MIT, see `LICENSE`.
