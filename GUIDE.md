# FIENDBOX guide

This guide covers what FIENDBOX adds. Everything underneath it is Felucca and GHOULBOX, and their manuals still apply:
the GHOULBOX 2.1 user guide in [`docs/guide/`](guide/) (PDF) explains the panel, the sequencer, songs, effects and
saving, and [Felucca's README](https://github.com/hugelton/Felucca#readme) covers the rest. FIENDBOX is a hobby project,
tested on one FM-1.

## 1. Quick start

Install it as the [README](../README.md) describes, **after saving a backup**. A new FM-1 then starts like this:

- The boot screen shows a summoning circle, an imp and the FiendBOX wordmark. The palette is **FIEND**: near-black blue,
  icy text, cyan values, crimson for the one active thing.
- It loads the scene **LABYRINTH**, and HOME shows the **coven**: four little creatures, one per track.
- Press **PLAY**. Four tracks play in A minor at 90 BPM: a square bass, dark strings, a thin wire lead and a drum kit.
- **MASTER** is the volume, **ALGORITHM** picks the track (T1 to T4), **PRESETS** changes that track's sound while it
  plays, **SELECT** is the tempo and **KNOB 1-4** shape the sound. Hold **HOME** for the menu.

An FM-1 that already has saved settings (from Felucca or GHOULBOX) keeps its palette, its scene and its HOME view: choose
them yourself in the menu (section 5).

## 2. The scenes

A scene is the music the FM-1 plays at power-on: four sounds, their patterns, a tempo and an amount of tape. Choose one
in **MENU > SYSTEM > SCENE**; it takes effect at the next power-on, and only when **RESTORE LAST** is off (with it on,
the FM-1 wakes up with your last session instead).

| Scene | Mood | BPM | Key | T1 | T2 | T3 | T4 |
|---|---|---|---|---|---|---|---|
| **LABYRINTH** | an industrial crawl | 90 | A minor | SQR BASS | DARK STRINGS | WIRE | DRUM KIT |
| **RUINS** | a cold, empty drone | 44 | D minor | LOW DRONE | CRYPT ORGAN | BELL | CAVE WIND |
| **SHRINE** | organ, frozen clouds, tape | 48 | D minor | PIPE ORGAN | FROZEN | BOWED PSALT | FLUTE |
| **GOTHIC** | slow and ornate | 66 | D minor | VIRGINAL | PIPE ORGAN | HARMONIUM | LOW DRONE |
| **BLANK** | LABYRINTH's sounds, no notes | 90 | | | | | |

In SHRINE the flute is a real recording, and with a little tape it stands in for a mellotron. Several of these sounds
(SQR BASS, WIRE, DRUM KIT, FLUTE) are left out of the PRESETS list to keep it short, but they play inside a scene.

Four patterns were added for LABYRINTH: **CRAWL** (a grinding bass), **DREAD** (chords), **SIREN** (a thin lead) and
**RUSTBEAT** (a hard kick and snare). Load them from **SAVE > PHRASES** (tap SAVE twice). They are in A minor; the
other nine factory patterns are in D minor, so set a track's ROOT when you mix the two.

## 3. How it looks

**The palette** is FIEND. The older palettes are still there in **MENU > DISPLAY > COLOR**.

**Familiars.** Tap **EDIT** and most engines draw a creature where a plain waveform used to be. Each engine has its own
species, and the creature is made from the sound:

| Control | What it does to the creature |
|---|---|
| CUT | the horns: a closed cell that opens into spikes as the filter opens |
| RES | the outline ripples |
| DTN, SPRD | two eyes, further apart as the detune grows |
| NOIS, RAND, BRTH, CRSH | the outline frays |
| WAVE | the body's lobes |
| anything else | by position: KNOB 1 more horns, 2 longer, 3 wobbling lobes, 4 more eyes |

It also reacts to the track's level: it swells, its lines warm from cyan through crimson to bone, and in silence it
cools and rests. The species are the sigil-sun (ANALOG), the writhing shade (PHASE), the stone idol (LOFI), the hydra
(VOICE), the bristled one (TRIO), the swarm (GRAIN and NOISE), the many-eyed (PHYS) and the crawler (GURDY). FM6 keeps
its algorithm chart, WHEEL its drawbars, and SAMPLE its waveform once a recording is loaded.

**The coven.** **MENU > DISPLAY > HOME** chooses what HOME shows under the knob cards: **SCOPE** (the waveform),
**TRACKS** (the four tracks as rows) or **COVEN**. In the coven each track is a creature of its engine's species,
swimming about the stage. It swells and glows with its own track's level, its eyes shut on a hit, its mouth opens with the
volume, and a muted track sleeps as a dim outline. The selected track is larger, with its number in cyan. With
**MENU > DISPLAY > ANIM** off the coven holds still.

## 4. Formulas: BYTE and FLOAT

The LOFI engine has two more waves, **BYTE** and **FLOAT**, where the sound is a formula of time. BYTE is **bytebeat**: an
8-bit formula (after viznut's 2011 one-liners) whose rhythm, melody and noise all come from a line of arithmetic. FLOAT is
**floatbeat**: the same idea in smooth sines. Each has 32 formulas, **B01** to **B32** and **F01** to **F32**. The old
LOFI waves play exactly as before.

Choose a LOFI sound, tap **EDIT** and turn **WAVE** (KNOB 2 on the first page) to BYTE or FLOAT. A key plays the formula in
that key, and it starts again from the top with each note.

| Page | KNOB 1 | KNOB 2 | KNOB 3 | KNOB 4 |
|---|---|---|---|---|
| EDIT 1 | CHIP (the bit depth) | WAVE | **ALGO** (the formula) | **VAR** |
| EDIT 2 | **MOTION** | **GRIT** (BYTE) or **FEEDBK** (FLOAT) | **BEND** | **LOOP** |

- **ALGO** picks the formula. Sweep it while you hold a key.
- **VAR** below 16 leaves the time as written (it only sets the variable some formulas read). Above that it mutates the
  time in steps: stairs that transpose, xor and or, holes in the rhythm, the phrase reversed, faster and higher,
  scrambled.
- **MOTION** speeds the formula's slow bits (its rhythm and structure) against its pitch. 64 plays it as written; lower is
  slower and more drone-like.
- **GRIT** (BYTE) is a rate crusher. **FEEDBK** (FLOAT) feeds the output back into the formula's time, from a shimmer to
  chaos.
- **BEND** folds the time on itself, which gives new melodies and rhythms from the same formula. **LOOP** holds the time
  in a short window: turn it up and a noise becomes a tuned note.

There are fourteen presets, after RECORDER in LOFI's list. The first seven come from JIANT FM, the last seven were picked
by ear:

| Preset | Formula | What it is |
|---|---|---|
| BYTEBEAT | B02 | the plain bytebeat |
| BYTE VAR | B17 | with VAR turned up, mutated |
| BYTE BENT | B05 | folded by BEND, with GRIT |
| BYTE TONE | B11 | looped into a tuned tone |
| FLOAT FM | F02 | FM whose index walks |
| FLOAT ARP | F03 | a plucked melody |
| FLOAT PAD | F29 | FM breathing at 1 Hz, slow |
| RITE BEAT | B01 | a rhythmic gate pattern, mono |
| ABYSS | B06 | a slow, evolving texture |
| IMP ARP | B07 | a self-playing arpeggio |
| RUNE SONG | B23 | a nibble-table melody, slowed and lo-fi |
| WISP LEAD | F03 | a plucked sine melody |
| VOID STAIRS | F15 | a staircase-sine pad |
| BONE ORGAN | F20 | odd harmonics that build up each bar |

The formulas are the heaviest sounds in the firmware, and FLOAT most of all. FIENDBOX has been tried with a few at once;
if a busy song crackles, use fewer FLOAT voices or switch some to BYTE.

## 5. Where things are

| To do this | Go to |
|---|---|
| Choose the power-on scene | hold HOME, **SYSTEM > SCENE** |
| Have the FM-1 wake up in your last session | hold HOME, **SYSTEM > RESTORE LAST** |
| Change the palette | hold HOME, **DISPLAY > COLOR** |
| Choose SCOPE, TRACKS or COVEN on HOME | hold HOME, **DISPLAY > HOME** |
| Stop the coven's motion | hold HOME, **DISPLAY > ANIM** |
| Load a factory pattern | tap SAVE twice (**PHRASES**) |
| Find the formula sounds | PRESETS, on a LOFI track, after RECORDER |
| Check the version | hold HOME, **SYSTEM > ABOUT** |

## 6. Backups and recovery

The installer does not make a backup. Open the web editor, connect, and on the Settings tab press **Save backup** before
every install. If FIENDBOX won't start, switch the FM-1 off and on again while holding **OCT-**: it starts in a USB rescue,
where the installer still works. To go back, install Felucca from its installer and restore the backup in its editor; its
"Return to official V15" puts M-VAVE's own firmware back.

## 7. Credits

FIENDBOX is built on **GHOULBOX** by Jason Persinger and **Felucca** by Leo Kuroshita / Hügelton Instruments, and adapts
code from **JIANT FM** by Juan Ignacio Antonucci (the creatures, the coven's swimming, bytebeat and floatbeat). The
bytebeat formulas credit Tejeez and xpansive. All of it is GPL-3.0-only. See [LICENSING.md](../LICENSING.md).
