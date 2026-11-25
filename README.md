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
- **Spell checking**: red squiggles under misspelled words in prose files
  (`.txt`, `.md`, `.rtf`, ...; not source code). Right-click a word for
  suggestions or "Add to dictionary" (saved in
  `~/config/settings/Inkwell_dictionary`). Spelling menu: switch it on/off and
  pick the language. English (en_US) is bundled. ALL CAPS, MixedCase words,
  numbers, URLs and Markdown code are skipped.
- **Themes** (View ▸ Theme): System colors, Light, Dark, Sepia. Applies to the
  editor and preview, not to the sidebar or window chrome; remembered in
  `~/config/settings/Inkwell_settings`.

## Build (on Haiku)

Requires the `haiku_devel` and `makefile_engine` packages.

    make
    ./objects.*/Inkwell

## Dictionaries

Dictionaries are standard Hunspell pairs (`xx_YY.aff` + `xx_YY.dic`). To add a
language, put the two files in `~/config/non-packaged/data/Inkwell/dictionaries/`
and restart Inkwell; it then shows up under Spelling ▸ Language. Compounding
and two-level affixes are not supported, so languages that rely on them
(German, Finnish, ...) will flag valid words.

The bundled `dictionaries/en_US.*` is from SCOWL (http://wordlist.sourceforge.net),
whose license (permissive; includes Ispell and WordNet notices) is in
`dictionaries/en_US.LICENSE`.

## Tests

The Markdown renderer, the RTF reader/writer and the spell checker have no
Haiku dependencies and are tested anywhere:

    make -C tests test

## Layout

- `src/` – C++ sources (Interface Kit, Locale Kit); `MarkdownRender.*`, `Rtf.*` and `Spell.*` are pure C++
- `Inkwell.rdef` – app signature (`application/x-vnd.Inkwell`), version, icon
- `Makefile` – uses Haiku's makefile-engine
- `packaging/inkwell-0.1.0.recipe` – draft HaikuPorts recipe
- `dictionaries/` – bundled Hunspell dictionary (en_US) and its license
- `artwork/` – icon PNGs (master 512px + 256/128/64/32/16); HVIF still to be traced
- `locales/` – translation catalogs (`.catkeys`), added later

## Roadmap

- Markdown: tables, nested quotes, list continuation lines
- RTF: font families, text color and alignment commands
- Themes: sidebar and window chrome, more palettes

## Publishing to HaikuDepot

Tag a release (`v0.1.0`), put the tarball's SHA-256 in the recipe, copy it to
`haiku-apps/inkwell/` in a fork of `haikuports/haikuports`, test with
`haikuporter`, then open a pull request.

## License

MIT, see `LICENSE`.
