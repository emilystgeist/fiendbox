# FIENDBOX: a demon synth for the M-VAVE FM-1

**FIENDBOX** is firmware for the **M-VAVE FM-1** pocket synthesizer: dark, occult and a little feral, aimed at the
music of old JRPG dungeons (Shin Megami Tensei, King's Field, Shadow Hearts) as much as at dungeon synth. It is a fork
of **GHOULBOX** (Jason Persinger's dungeon-synth firmware), which is itself a fork of **Felucca** by Hügelton
Instruments (1.4.1), with ideas and code adapted from **JIANT FM** by Juan Ignacio Antonucci.

Version 0.4 (HOME held > ABOUT). A hobby project, tested on one FM-1: **install at your own risk.**

**Guide:** [docs/GUIDE.md](docs/GUIDE.md): the scenes, the familiars, the coven and the formulas.

**Try it first:** the web emulator runs the real firmware in your browser, with the mouse, the computer's keys or MIDI:
**<https://emilystgeist.github.io/fiendbox/>**

## What FIENDBOX adds

Everything Felucca and GHOULBOX do still works: four tracks, a 64-step sequencer, songs, the arpeggiator, chord keys,
effects, the web editor and backups, the hall reverb, TAPE and CRSH, the drone latch, the hurdy-gurdy, KEYS layer and
split, DREAM (a page that composes and evolves patterns in key). On top of them:

- **Five power-on scenes** (MENU > SYSTEM > SCENE): **LABYRINTH** (the default: an industrial crawl in A minor, 90 BPM),
  **RUINS** (a King's Field drone, 44 BPM), **SHRINE** (organ, frozen clouds, a bowed psaltery and a flute that
  passes for a mellotron, 48 BPM), **GOTHIC** (virginal, pipe organ and harmonium, 66 BPM) and **BLANK** (LABYRINTH's
  sounds with no notes). Four new patterns for them, CRAWL, DREAD, SIREN and RUSTBEAT (SAVE > PHRASES).
- **The FIEND palette**: near-black blue, icy text, cyan values, crimson for the one active thing. The default on a
  new device (an FM-1 that already has saved settings keeps its palette: MENU > DISPLAY > COLOR).
- **A new boot screen and ABOUT page**: a summoning circle, an imp with a frost mark on its brow, and a blackletter
  wordmark.
- **Familiars.** The EDIT pages of most engines draw a creature instead of a plain waveform: one species per
  engine, its horns opening with the filter, its outline rippling with resonance, its eyes multiplying with detune,
  and the whole body warming from cyan through crimson to bone as the sound gets louder.
- **The coven.** MENU > DISPLAY > HOME > **COVEN** (the default on a new device) turns HOME's stage into four
  familiars, one per track, swimming about: each swells and glows with its own track's level, its eyes shut on a hit, a
  muted track sleeps.
- **BYTE and FLOAT**, two more waves for the LOFI engine: **bytebeat** and **floatbeat**, sound made from a formula of
  time. 32 formulas each (the ALGO control); VAR, MOTION, GRIT or FEEDBK, BEND and LOOP shape them; a key plays the
  formula in that key. Fourteen presets, from BYTEBEAT and FLOAT FM to RITE BEAT, ABYSS, RUNE SONG and BONE ORGAN. The
  old waves play exactly as before.
- **Left out of the device build:** the SLICE engine, already hidden from every menu, to make room. A project that
  used it will not play it.

## Install

You need Chrome or Edge on a computer, an FM-1 on a USB **data** cable, and no other music app open.

The installer runs in the browser: **<https://emilystgeist.github.io/fiendbox/webapp/installer/>**. To install a package you
built yourself instead, serve the installer site on your own computer:

```
python3 tools/ghoulbox_serve.py       # http://localhost:8000/webapp/installer/
```

**First save a backup:** the installer doesn't make one. Open the [web editor](https://emilystgeist.github.io/fiendbox/webapp/editor/), connect, and on its Settings tab press
**Save backup**. If your FM-1 still runs M-VAVE's own firmware there is nothing to back up, but
download the official V15 file from M-VAVE first, so you can return to it.

Then open the installer and press Install. If it stops after "update mode", replug the USB cable and press Install
again: the FM-1 stays in update mode until the install is finished. To go back, install Felucca from its installer
(<https://hugelton.github.io/Felucca/>) and restore your backup in its editor; its "Return to official V15" puts
M-VAVE's firmware back.

If FIENDBOX itself won't start, switch the FM-1 off, then on again while holding **OCT-**: it starts in a USB rescue (no
sound, no menu) where the installer still works. If the FM-1 stays black and a computer sees a USB device named
**WL80UBOOT**, it is in the chip's boot mode: try another cable and run the installer again, or see
[FM-1 Transporter](https://github.com/kurogedelic/FM-1-transporter).

## Build

You need the JieLi toolchain (`tools/get_toolchain.sh`, Linux x86-64, installs to `~/.jieli/toolchain`), the JieLi AC79
SDK at the tag `AC79NN_SDK_V1.2.1_2023-12-13` (see [BUILDING.md](BUILDING.md)), and Python with Pillow, fontTools and
NumPy. In a virtual environment:

```
AC79_SDK=~/fw-AC79-ref ./build_device.sh     # build/felucca.fwsc, the package to install
```

The build prints a warning if the SDK's files differ from the tested version: with none, the checkout is right. The
device build leaves the SLICE engine out; the default `./build.sh` no longer fits the app slot.

## Tests

The firmware's own audio and screen code runs on a computer, so a change can be heard and seen before flashing:

```
./check.sh                         # the device build, then every host test
python3 tools/build.py --gen-only  # the generated data the host tests need (SLICE on)
HOST_ONLY=1 tests/run_tests.sh
```

`tests/run_tests.sh` and `build_device.sh` share `build/gen`: after a device build, run `--gen-only` before the host
tests (`check.sh` does this). See [BUILDING.md](BUILDING.md) for the rest.

The release tooling (`tools/release.py`, `docs/RELEASING.md`) is inherited from GHOULBOX and still points at its
repository and site: don't run it as it is.

## Credits and licence

FIENDBOX changes © 2026 bemily, GPL-3.0-only, like everything it is built on.

- **Felucca** by Leo Kuroshita / Hügelton Instruments (GPL-3.0-only), 1.4.1: the whole firmware this is built on.
  Please support the original ([GitHub Sponsors](https://github.com/sponsors/hugelton),
  [itch.io](https://hugelton.itch.io/felucca)). <https://github.com/hugelton/Felucca>
- **GHOULBOX** by Jason Persinger (GPL-3.0-only): the dungeon sounds, scenes, hurdy-gurdy, reverb and tape, the
  screen's pixel look and much else. <https://github.com/jasonpersinger/ghoulbox-fm1-dungeon-synth>
- **JIANT FM** by Juan Ignacio Antonucci (Juan Jiant) (GPL-3.0-only): the creatures, the coven's swimming, and
  bytebeat and floatbeat are adapted from its code. <https://github.com/juanjiant-bit/JIANT>
- Fonts: VT323 by Peter Hull and UnifrakturMaguntia by j. 'mach' wust and Peter Wiegel (SIL OFL 1.1). Samples:
  Versilian Studios, VSCO-2 Community Edition and VCSL (CC0 1.0).

Every third-party component and its licence is listed in [LICENSING.md](LICENSING.md). M-VAVE and FM-1 are trademarks of
their owners; FIENDBOX is not affiliated with or endorsed by them, by Hügelton Instruments, or by the authors of
GHOULBOX or JIANT FM. The user guide in `docs/guide` is GHOULBOX's, and describes the GHOULBOX 2.1 base.
