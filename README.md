# Inkwell

A native Haiku text editor for Markdown, RTF and plain text, built around a
sidebar of linked files and folders. Files open in tabs.

**Status:** early skeleton. Currently: open a folder, browse it in the sidebar,
double-click a file to edit it in a tab (Markdown files get a live preview
beside the editor, View ▸ Markdown preview, ⌘P), save, close tab. Unsaved changes are marked on the tab
and prompted for on close/quit; the sidebar follows changes on disk.

## Build (on Haiku)

Requires the `haiku_devel` and `makefile_engine` packages.

    make
    ./objects.*/Inkwell

## Tests

The Markdown renderer has no Haiku dependencies and is tested anywhere:

    make -C tests test

## Layout

- `src/` – C++ sources (Interface Kit, Locale Kit); `MarkdownRender.*` is pure C++
- `Inkwell.rdef` – app signature (`application/x-vnd.Inkwell`), version, icon
- `Makefile` – uses Haiku's makefile-engine
- `packaging/inkwell-0.1.0.recipe` – draft HaikuPorts recipe
- `artwork/` – icon PNGs (master 512px + 256/128/64/32/16); HVIF still to be traced
- `locales/` – translation catalogs (`.catkeys`), added later

## Roadmap

- Markdown: clickable links, tables, nested quotes, list continuation lines
- RTF via Haiku translators
- Themes (light/dark), starting from system colors
- Hebrew translation

## Publishing to HaikuDepot

Tag a release (`v0.1.0`), put the tarball's SHA-256 in the recipe, copy it to
`haiku-apps/inkwell/` in a fork of `haikuports/haikuports`, test with
`haikuporter`, then open a pull request.

## License

MIT, see `LICENSE`.
