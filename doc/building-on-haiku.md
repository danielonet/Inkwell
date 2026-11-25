# Installing, building and running Inkwell on Haiku

Inkwell is a native Haiku application written in C++ with the Interface Kit.
It is built with Haiku's standard `makefile-engine`, so there is nothing to
configure: install a few packages, run `make`, run the program.

> **Status:** version 0.1.0 has not been compiled on a real Haiku system by its
> author yet. If the build stops with an error, please report it (the compiler
> output is what is needed). Some of the Interface Kit calls could not be
> checked against the headers; see [Troubleshooting](#troubleshooting).

## 1. Requirements

- Haiku **R1/beta4 or newer**, 64-bit (x86_64). The code uses C++11, so the
  old 32-bit `x86_gcc2` build of Haiku is not supported.
- About 50 MB of free disk space for the source and build output.
- An internet connection for installing packages and downloading the source.

### Install the build tools

Open **Terminal** and run:

    pkgman install haiku_devel makefile_engine cmd:g++ cmd:make

| Package | Why it is needed |
|---|---|
| `haiku_devel` | Haiku headers and libraries (`libbe`, `libtracker`, `liblocalestub`) |
| `makefile_engine` | The shared Makefile used by `Makefile` |
| `cmd:g++`, `cmd:make` | Compiler and build tool (already installed on most systems) |

You can also install them from **HaikuDepot** (search for the package names).
Check the compiler with:

    g++ --version

To fetch the source with git, install it too: `pkgman install git`. (Or
download the repository as a zip from GitHub and unpack it.)

## 2. Get the source

    cd ~/Desktop            # or any folder you like
    git clone https://github.com/danielonet/Inkwell.git
    cd Inkwell

## 3. Build

    make

The build takes under a minute. The result is the program `Inkwell` inside a
folder named like `objects.x86_64-cc13-release/` (the exact name depends on
your compiler). The `.rdef` file is compiled into resources and the app's
signature (`application/x-vnd.Inkwell`) is registered by the makefile-engine.

Useful variants:

    make clean              # delete the build output
    make DEBUGGER=1         # build with debugging symbols (for gdb)

## 4. Run

From the source folder:

    ./objects.*/Inkwell

or double-click `Inkwell` in the `objects.*` folder in Tracker.

When run from the source folder, Inkwell finds the bundled English dictionary
in `Inkwell/dictionaries/` automatically (it looks next to the program and one
folder above it). If you move the program elsewhere, install the dictionary as
described below, otherwise spell checking has no language.

## 5. Install for everyday use

Inkwell is not packaged yet (HaikuPorts submission comes later), so install it
by hand in the "non-packaged" area:

    mkdir -p ~/config/non-packaged/apps
    mkdir -p ~/config/non-packaged/data/Inkwell/dictionaries
    cp objects.*/Inkwell ~/config/non-packaged/apps/
    cp dictionaries/* ~/config/non-packaged/data/Inkwell/dictionaries/

Optionally add it to the Deskbar's Applications menu:

    mkdir -p ~/config/settings/deskbar/menu/Applications
    ln -s ~/config/non-packaged/apps/Inkwell \
        ~/config/settings/deskbar/menu/Applications/Inkwell

To remove it again, delete those files and the settings listed in
[Where Inkwell keeps its files](#7-where-inkwell-keeps-its-files).

## 6. Using Inkwell

The **Alt** key is the Haiku command key by default (shown as ⌘ in this
project's README).

| Action | How |
|---|---|
| Open a folder | File ▸ Open folder… (Alt+O) |
| Open a file | Double-click it in the sidebar; it opens in a tab |
| Save / close tab / quit | Alt+S / Alt+W / Alt+Q |
| Markdown preview | Open a `.md` file; View ▸ Markdown preview (Alt+P) |
| Follow a link in the preview | Click it. Web and mail links open in your default app; links to other files open in a new tab |
| Rich text | Open an `.rtf` file; use the Format menu: Bold Alt+B, Italic Alt+I, Underline Alt+U, Bigger Alt++, Smaller Alt+- |
| Theme | View ▸ Theme: System colors, Light, Dark, Sepia |
| Spelling | Misspelled words get a red squiggle. Right-click one for suggestions or *Add to dictionary*. Spelling menu: switch on/off, choose language |

Tabs with unsaved changes show a bullet (•) before the name; Inkwell asks
before closing them. The sidebar updates by itself when files change on disk.

### Adding another spelling language

Dictionaries are Hunspell pairs (`xx_YY.aff` and `xx_YY.dic`), for example from
the LibreOffice dictionaries project. Copy both files to

    ~/config/non-packaged/data/Inkwell/dictionaries/

and restart Inkwell; the language appears under Spelling ▸ Language.
Languages that depend on word compounding (German, Finnish, …) will report
valid words as errors, because compounding is not supported yet.

## 7. Where Inkwell keeps its files

| Path | Contents |
|---|---|
| `~/config/settings/Inkwell_settings` | Theme, spelling on/off and language |
| `~/config/settings/Inkwell_dictionary` | Your personal words (one per line, plain text) |
| `~/config/non-packaged/data/Inkwell/dictionaries/` | Extra dictionaries you added |

## 8. Run the tests

The Markdown renderer, RTF reader/writer and spell checker contain no Haiku
code and have unit tests:

    make -C tests test

It should print "All markdown tests passed.", "All rtf tests passed." and "All
spell tests passed.". The user-interface code has no automated tests; try it
by hand using the table in section 6.

## Troubleshooting

**`makefile-engine: No such file` or `findpaths: not found`**
Install the packages from section 1 (`makefile_engine`, `haiku_devel`).

**`cannot find -llocalestub` or `-ltracker`**
`haiku_devel` is missing or incomplete; reinstall it:
`pkgman install haiku_devel`.

**Compile errors mentioning `InsertText`, `DeleteText`, `ExpandOrCollapse`,
`GetFontAndColor` or `PointAt`**
These are `BTextView` / `BOutlineListView` functions whose exact signatures
could not be checked when the code was written. Please send the full compiler
message; the fix is usually a one-line change.

**The window opens but the sidebar is empty**
Choose File ▸ Open folder… first. Inkwell does not remember the last folder yet.

**No red squiggles / the Language menu says "No dictionaries found"**
The dictionary was not found. Run Inkwell from the source folder, or copy the
files as described in section 5. The search order is: the non-packaged data
folder above, `/boot/system/data/Inkwell/dictionaries`, then the `dictionaries`
folder next to the program and one folder above it.

**The app has a generic icon**
The vector icon has not been added yet (see `artwork/` and the note in
`Inkwell.rdef`).

**Starting a second copy just brings the first to the front**
That is intended: Inkwell is a single-launch application.

**Dark theme looks half-dark**
Themes only recolor the editor and the preview, not the sidebar or window
frames. Use *System colors* together with a dark Appearance preference if you
want everything dark.

## Packaging (maintainers)

`packaging/inkwell-0.1.0.recipe` is a draft HaikuPorts recipe. Tag a release,
put the SHA-256 of the release tarball in `CHECKSUM_SHA256`, copy the recipe to
`haiku-apps/inkwell/` in a fork of the HaikuPorts repository, test it with
`haikuporter`, and open a pull request. The recipe installs the program and the
dictionaries to `$appsDir` and `$dataDir/Inkwell/dictionaries`.
