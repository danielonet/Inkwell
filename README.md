# Inkwell

A native Haiku text editor for Markdown, RTF and plain text, built around a
sidebar of linked files and folders. Files open in tabs.

**Status:** early skeleton. Currently: open a folder, browse it in the sidebar,
double-click a file to edit it as plain text in a tab, save, close tab.

## Build (on Haiku)

Requires the `haiku_devel` and `makefile_engine` packages.

    make
    ./objects.*/Inkwell

## Layout

- `src/` – C++ sources (Interface Kit, Locale Kit)
- `Inkwell.rdef` – app signature (`application/x-vnd.Inkwell`), version, icon
- `Makefile` – uses Haiku's makefile-engine
- `packaging/inkwell-0.1.0.recipe` – draft HaikuPorts recipe
- `artwork/` – icon PNGs (master 512px + 256/128/64/32/16); HVIF still to be traced
- `locales/` – translation catalogs (`.catkeys`), added later

## Roadmap

- Markdown preview (md4c), RTF via Haiku translators
- Themes (light/dark), starting from system colors
- Node monitoring and lazy loading in the sidebar
- Unsaved-changes prompts, Hebrew translation

## Publishing to HaikuDepot

Tag a release (`v0.1.0`), put the tarball's SHA-256 in the recipe, copy it to
`haiku-apps/inkwell/` in a fork of `haikuports/haikuports`, test with
`haikuporter`, then open a pull request.

## License

MIT, see `LICENSE`.
