# ZMK Custom Layouts

[![ZMK version](https://img.shields.io/badge/ZMK-upstream-blue)](https://zmk.dev/)
[![Firmware build](https://img.shields.io/github/actions/workflow/status/reybits/zmk-config/build.yml?label=firmware%20build)](https://github.com/reybits/zmk-config/actions/workflows/build.yml)

> [!NOTE]
> **Fork of [Townk/zmk-config](https://github.com/Townk/zmk-config).** The
> base layout and the master-layout-plus-adapter idea come from there. The
> fork has diverged enough that it's worth listing the substantive changes:
>
> - **Built against upstream ZMK on Zephyr 4.1.** The original pinned a
>   personal fork (`Townk/zmk@mousemove-molock`) on Zephyr 3.5. CI now
>   builds against `zmkfirmware/zmk@main` for `nice_nano//zmk`.
> - **Standalone `&molock` behavior module** at the repo root, riding on
>   upstream's first-class layer-locking API (PR #2717) instead of carrying
>   a patch inside the ZMK source tree.
> - **Vendored shields.** `boards/shields/{rolio,vista508}/` live in-repo,
>   no longer pulled from `Townk/zmk-rolio-shield`.
> - **Vista508 widgets ported to LVGL 9.** Includes the load-bearing
>   `CONFIG_ZMK_DISPLAY_DEDICATED_THREAD_STACK_SIZE=8192` — without it the
>   display thread overflows its stack inside LVGL 9's layer/draw chain
>   and the keyboard never advertises over BLE or enumerates over USB.
> - **Lily58 support removed** (unowned by this maintainer).
> - **Rolio's Buttons layer** moved to the dedicated inner-thumb keys
>   (Corne keeps the Z/`/` hold-tap variant since it lacks the extra
>   thumb positions).
> - **Layout deltas vs. upstream prose**: `` ` `` ↔ `⇥` swap on the base
>   alpha row; homerow mod order is `⌘ ⌥ ^ ⇧` pinky-to-index on both
>   hands; Numbers layer swaps `⏎` and `⇥`; Navigation and Mouse right-hand
>   bindings shifted one column inward; new innermost-homerow combo emits `CAPS_LOCK`
>   (used for the macOS PL ↔ EN input-source toggle).
> - **Base-layer thumb cluster reshuffled.** Enter and Backspace are on
>   opposite hands (Enter on the left thumb, Backspace on the right
>   thumb with the Shift → ⌦ morph following it). The Symbols hold
>   moved off the innermost left thumb onto the position next to it,
>   so a sustained hold no longer needs the thumb to curl all the way
>   inward. Mouse and Navigation activation swapped within the left
>   thumb — Navigation via a plain momentary key at the middle-left
>   thumb, Mouse via a hold-tap on the innermost left thumb (Esc is the
>   tap face).

This repository contains the ZMK user configuration for all my keyboards that
use the [ZMK](https://zmk.dev/) firmware.

The way I organize the keyboards' layouts is to have a single _master layout_,
with more keys defined than any keyboard I have. Then, each individual Keyboard
configuration would define a _“layer adapter”_ mapping the keys in the layout
that it uses to its physical position.

Every keyboard layout has its layout documentation (located under the `docs`
directory), but since they all share the majority of my layout, you should
expect plenty of repeating texts between them.

My suggestion is to read the layout documentation for the keyboard you're
looking for, then, when you understand it, go check out the other docs. This
way, you won't have to read the same text twice, or explanations for things you
aren't concerned with in the first place.

Currently, in this repository, I have layout for:

- [Corne (6 columns)](docs/corne.md)
- [Rolio46.2](docs/rolio.md)

If you did not click on any of the layout links yet, there is some general
information I would like you to have to help you understand them better.

## Symbols Glossary

Inside the keymap files and on the layout map images, you'll see a series of
symbols that are not well-used in ZMK configurations out there, so to understand
the layout easily, here is a table with all the symbols I use:

| Symbol | Description | Symbol | Description | Symbol | Description |
|:------:|-------------|:------:|-------------|:------:|-------------|
| ![Command](./docs/glyphs/apple-keyboard-command.svg) | Command (Super) | ![Control](./docs/glyphs/apple-keyboard-control.svg) | Control (Ctrl) | ![Option](./docs/glyphs/apple-keyboard-option.svg) | Option (Alt / Meta) |
| ⇪ | Caps Word | ![Shift](./docs/glyphs/apple-keyboard-shift.svg) | Shift | ![Globe](./docs/glyphs/web.svg) | Globe |
| ![Caps-Word](./docs/glyphs/apple-keyboard-caps.svg) | Turn On Caps-Word | ![Space](./docs/glyphs/keyboard-space.svg) | Space | ![Return](./docs/glyphs/keyboard-return.svg) | Enter (Return / Ret) |
| ![Backspace](./docs/glyphs/backspace.svg) | Delete backwards (Backspace / Bksp) | ![Forward Delete](./docs/glyphs/backspace-reverse.svg) | Delete forward (Del) | ↖ | Home |
| ⇞ | Page Up | ⇟ | Page Down | ↘ | End |
| ![Backtab](./docs/glyphs/keyboard-tab-reverse.svg) | Backtab | ![Tab](./docs/glyphs/keyboard-tab.svg) | Tab | ![Volume up](./docs/glyphs/volume-plus.svg) | Volume up |
| ![Volume down](./docs/glyphs/volume-minus.svg) | Volume down | ![Volume off](./docs/glyphs/volume-off.svg) | Mute | ![Brightness up](./docs/glyphs/brightness-7.svg) | Screen brightness up |
| ![Brightness down](./docs/glyphs/brightness-5.svg) | Screen brightness down | ![Backlight off](./docs/glyphs/lightbulb-off-outline.svg) | Keyboard backlight off | ![Backlight down](./docs/glyphs/lightbulb-on-20.svg) | Keyboard backlight decrease |
| ![Backlight up](./docs/glyphs/lightbulb-on-90.svg) | Keyboard backlight increase | ![Skip back](./docs/glyphs/skip-backward.svg) | Previous song | ![Play or pause](./docs/glyphs/play-pause.svg) | Play / pause |
| ![Skip forward](./docs/glyphs/skip-forward.svg) | Next song | ![Stop](./docs/glyphs/stop.svg) | Stop media | ![Launchpad](./docs/glyphs/apps.svg) | Launchpad |
| ![Mission Control](./docs/glyphs/view-dashboard-outline.svg) | Mission Control (`⌘ ⌃ ↑`) | ![Show all windows](./docs/glyphs/dock-window.svg) | Show application windows (`⌘ ⌃ ↓`) | ![Spotlight](./docs/glyphs/folder-search-outline.svg) | Spotlight |
| ![Cut](./docs/glyphs/content-cut.svg) | Cut (`⌘ X`) | ![Copy](./docs/glyphs/content-copy.svg) | Copy (`⌘ C`) | ![Paste](./docs/glyphs/content-paste.svg) | Paste (`⌘ V`) |
| ![Undo](./docs/glyphs/undo.svg) | Undo (`⌘ Z`) | ![Redo](./docs/glyphs/redo.svg) | Redo (`⇧ ⌘ Z`) | ![Search](./docs/glyphs/magnify.svg) → | Find Next (`⌘ G`) |
| ← ![Search](./docs/glyphs/magnify.svg) | Find Previous (`⇧ ⌘ G`) | ![Previous word](./docs/glyphs/format-letter-starts-with.svg) | Previous word (`⌥ ←`) | ![Next word](./docs/glyphs/format-letter-ends-with.svg) | Next word (`⌥ →`) |
| ![Line start](./docs/glyphs/format-pilcrow-arrow-left.svg) | Beginning of line (`⌘ ←`) | ![Line end](./docs/glyphs/format-pilcrow-arrow-right.svg) | End of line (`⌘ →`) | ![Virtual desktop](./docs/glyphs/window-maximize.svg) → | Go to Virtual Desktop on the Right (`⌘ ⌃ →`) |
| ← ![Virtual desktop](./docs/glyphs/window-maximize.svg) | Go to Virtual Desktop on the Left (`⌘ ⌃ ←`) | ![Window](./docs/glyphs/window-restore.svg) → | Next window (`` ⌘ ` ``) | ← ![Window](./docs/glyphs/window-restore.svg) | Previous window (`⌘ ~`) |
| ![Keyboard](./docs/glyphs/keyboard-variant.svg) | Alternate layout (COLEMAK) | ![Bluetooth profile](./docs/glyphs/bluetooth-connect.svg) | Bluetooth profile | ![Bluetooth clear](./docs/glyphs/bluetooth-off.svg) | Bluetooth profile clear |
| ![Keyboard power](./docs/glyphs/power-plug-outline.svg) | Toggle OLED display | ![LED underglow actions](./docs/glyphs/palette-outline.svg) | LED RGB underglow actions | ![USB](./docs/glyphs/usb.svg) | Output mode (USB / BLE) |
| ![Computer power](./docs/glyphs/power.svg) | Turn off host computer | ![Keyboard reset](./docs/glyphs/restart.svg) | Reset firmware | ![Bootload](./docs/glyphs/code-block-tags.svg) | Bootload mode |
| ![Navigation layer](./docs/glyphs/arrow-all.svg) | Navigation Layer | ![Numbers layer](./docs/glyphs/numeric.svg) | Numbers Layer | ![Symbols layer](./docs/glyphs/code-braces.svg) | Symbols Layer |
| ![Media layer](./docs/glyphs/music.svg) | Media Layer | ![Mouse layer](./docs/glyphs/mouse-variant.svg) | Mouse Layer | ![Functions layer](./docs/glyphs/keyboard-f1.svg) | Functions Layer |
| ![Buttons layer](./docs/glyphs/monitor.svg) | Buttons Layer | ![System layer](./docs/glyphs/cog-outline.svg) | System Layer | ![Layer lock](./docs/glyphs/lock-outline.svg) | Lock layer in place |

## Key Representation

When looking at the layout map images, you'll see that some keys have their
background highlighted with different colors than the rest; many of them, also
have more than one symbol as their labels.

On each layer map, the key you hold to reach that layer is drawn in green and
carries the layer icon.

To understand what all those symbols mean, let's first, look at how a key that
has a single purpose is represented:

![Simple key representation](docs/images/simple-key.svg)

This key has nothing special about it. When I tap it, it will output `a`, and if
I hold it down, it will output `a` repeatedly, until I release it.

My layouts, like virtually every other keyboard layout in the world (including
the standard ones that most people use), have keys that are used exclusively to
alter what other keys output when held-down. When that happens, we say that the
key caused the keyboard to _“switch layers”_, for instance:

![Shift key representation](docs/images/shift-key.svg)

This is the _Shift_ key, which forces other keys to output the uppercase version
of itself, or in certain cases, it can force the key to output an entirely
different symbol than the normal one (e.g., the number keys in most standard
keyboards).

When a key outputs a different symbol than the uppercase version of the normal
output while holding down the _Shift_ key, it has the extra symbol drawn above
the normal one, for instance:

![Number 2 key representation](docs/images/number-2-key.svg)

When tapped while no other hey is held-down, it will output `2`, but if the
_Shift_ key is held-down while tapping it, it will output `@`.

Now, there is a special type of key that you don't find in standard keyboards
easily. Those keys output their normal value when tapped, and a different one
when held-down. These keys are called _“Hold-Tap”_ keys. Most custom layouts
use them, especially layouts for keyboards with less than 50 keys.

When those keys are shown in my layout maps, they have the _“hold-down”_ value
displayed below the normal symbol. For instance:

![Tap-hold key “F” and “Shift”](docs/images/f-shift-key.svg)

This key is a normal `F` key that output `f` when tapped, and `F` when tapped
while holding down the _“Shift”_ key. However, when I hold this key down, it
behaves like the _“Shift”_ key, meaning that if I hold it down, and then press
the key for number `2`, the keyboard will output a `@` character.

> [!NOTE]
> Unfortunately, Keymap Drawer does not support representations of
> [Mod-Morph](https://zmk.dev/docs/behaviors/mod-morph) or
> [Tap-Dance](https://zmk.dev/docs/behaviors/tap-dance), so I had to come up
> with a syntax to represent them:
>
> ![Special labels on key representation](docs/images/labels-syntax.svg)
>
> When you see a symbol followed by `:`, followed by another symbol on either,
> the _shifted symbol_ or _held symbol_ positions, it is indicating a _morph_
> behavior, meaning: `<morph trigger>: <output symbol>`. In the example above,
> when the `A` key is tapped while holding down the _Command_ key, the result
> output will be the `` ` `` key.
>
> When you see a counter in the form of `<N>x`, followed by `:`, followed by a
> symbol on either the _held symbol_ or _shifted symbol_ positions, it indicates
> a _tap-dance_ behavior, meaning: `<tap count>x: <output symbol>`. In the
> example above, when the `A` key is tapped twice, it will output a `CAPS_LOCK`.

When we put all this together, and we see the following key in a layout map:

![Tap-hold key “Backspace” and “Forward Delete”](docs/images/backspace-del-key.svg)

We know that this key is a normal _“Delete Backwards”_ when tapped by itself,
and a _“Forward Delete”_ when tapped while holding down _“Shift”_. But when I
hold it down, it will be like I'm holding down the _“Command”_ key on my macOS.

## Tap-Hold Behavior

Usually, a tap and hold key goes into the “hold” mode when the key is held-down,
which prevents the user from holding the key down and having its output repeated
until the key is released.

To solve this issue, I used a (relatively new) feature of ZMK called
[`quick-tap-ms`](https://zmk.dev/docs/behaviors/hold-tap#quick-tap-ms), which
always produces the “tap” behavior when the key is pressed twice in quick
succession. This allows me to have repeatable keys, even when they're a
“tap-hold” type of key.

For instance, let's assume the `F` tap-hold key shown in the previous section.
The sequence of taps and their output would be:

- `F` down, `F` up :: it will output a single `f`;
- `F` down :: it will output a _“Shift”_ key;
- `F` down, `F` up, `F` down :: it outputs `ffffffffff…` until the key is
  released.

## Globe Key

Around 2021, Apple made the `(FN)/Globe` key a standard key on all its
keyboards, so as an Apple user, I wanted to have the same ability to access some
features using my keyboard as I can using a standard Apple keyboard.

ZMK has partial support for the _Globe_ key (check issue
[#947](https://github.com/zmkfirmware/zmk/issues/947) for details on the matter),
it can only be used as a _“holding”_ key.

## Caps Lock & Caps Word

The layout exposes two related behaviors on combos rather than dedicated keys:

- **`CAPS_WORD`** — pressing both _Shift_ keys together. ZMK's
  [`CAPS_WORD`](https://zmk.dev/docs/behaviors/caps-word) capitalizes only the
  next word typed and then automatically returns to normal typing — useful for
  the occasional fully-capitalized word without holding _Shift_.

- **`CAPS_LOCK`** — pressing `G` + `H` together. This sends the real
  `CAPS_LOCK` HID keycode. macOS can be configured under
  _System Settings → Keyboard → Input Sources → Use Caps Lock to switch
  between input sources_ to route this toggle to switch between input
  languages (e.g. PL ↔ EN) instead of flipping the caps light, which is why
  it's kept available even though it's a rarely-needed key otherwise.

## My methodology

You may be asking yourself (probably not, but I'll pretend you are) what is my
process to choose where keys go in all these layouts?

In all honesty, I don't have one. I usually use the keyboard and try to note
repetitive mistakes or things that require a bigger effort to accomplish. When
they become somewhat highly noticeable, I think about where the keys causing
this issue should be for me to have less of these mistakes happening. Then, I
change the layout to match my new hypothesis, flash the keyboard, and use the
layout as if nothing happened.

Over time, the fixes that worked will fade in the background while new issues
that deserve my attention, as well as the fixes that did not work, will show up
again and again. When that happens… Rinse and repeat.

## Local build

If you want to build this configuration locally, first, follow [ZMK's
instructions](https://zmk.dev/docs/development/setup/native) to be able to build
the firmware itself. The build runs against upstream ZMK on Zephyr 4.1, so make
sure your Zephyr SDK is current.

Run all subsequent commands from the repository root. The build needs
`-DZMK_EXTRA_MODULES="$PWD"` so Zephyr discovers the locally-vendored shields
(`boards/shields/{rolio,vista508}/`) and the in-repo `&molock` behavior driver
(`src/behaviors/`). The board name is the Zephyr-4.1 form: `nice_nano//zmk`
(short for `nice_nano@2.0.0//zmk`).

```sh
$ west build -p -d build/rolio-left -b 'nice_nano//zmk' -- \
    -DSHIELD='rolio_left vista508' \
    -DZMK_EXTRA_MODULES="$PWD" \
    -DZMK_CONFIG="$PWD/config" \
  && west build -p -d build/rolio-right -b 'nice_nano//zmk' -- \
    -DSHIELD='rolio_right vista508' \
    -DZMK_EXTRA_MODULES="$PWD" \
    -DZMK_CONFIG="$PWD/config" \
  && west build -p -d build/corne-left -b 'nice_nano//zmk' -- \
    -DSHIELD='corne_left nice_view_adapter nice_view' \
    -DEXTRA_CONF_FILE="$PWD/config/corne_niceview.conf" \
    -DZMK_EXTRA_MODULES="$PWD" \
    -DZMK_CONFIG="$PWD/config" \
  && west build -p -d build/corne-right -b 'nice_nano//zmk' -- \
    -DSHIELD='corne_right nice_view_adapter nice_view' \
    -DEXTRA_CONF_FILE="$PWD/config/corne_niceview.conf" \
    -DZMK_EXTRA_MODULES="$PWD" \
    -DZMK_CONFIG="$PWD/config" \
  && west build -p -d build/settings-reset -b 'nice_nano//zmk' -- \
    -DSHIELD=settings_reset \
    -DZMK_EXTRA_MODULES="$PWD" \
    -DZMK_CONFIG="$PWD/config"
```

After the first build, subsequent builds only need the build dir:

```sh
$ west build -d build/rolio-left \
  && west build -d build/rolio-right \
  && west build -d build/corne-left \
  && west build -d build/corne-right \
  && west build -d build/settings-reset
```

If you make any modifications to the keymap, regenerate the layout SVGs used in
the documentation. The script passes `-DUSE_MOLOCK=1` to `keymap-drawer` so the
universal `&molock` key shows up annotated in the rendered images (it has no
effect on the firmware build itself — `&molock` is always available there).
Install `keymap-drawer` first (one-time):

```sh
$ pipx install keymap-drawer==0.23.0
```

Use this version: CI regenerates the maps with it and fails when they differ
from the committed ones.

Then run the layout regenerator:

```sh
$ ./support/update-layout-maps.sh
```

## Disclaimers

Layout map images are all generated with
[Keymap-Drawer](https://keymap-drawer.streamlit.app/) from Cem Aksoylar. If you
have your custom ZMK or QMK configuration, I highly recommend you to check it
out.

My homerow mod configuration is based on the fantastic _“timeless homerow mods”_
from [Robert U (@urob)'s ZMK Configuration](https://github.com/urob/zmk-config).
You should absolutely check this configuration for examples.

The pattern I used to share my layout across multiple keyboards was a suggestion
from Rafael Romão ([@rafaelromao](https://github.com/rafaelromao/keyboards)) and
Cem Aksoylar ([@caksoylar](https://github.com/caksoylar)) on the [ZMK Discord
server](https://discord.com/channels/719497620560543766/813882537436905552/1253152742910984282),
so I spent some time digging through their ZMK configurations to learn, and copy
the pattern into my layout.

Unfortunately, due to the nature of my layout, you cannot use this configuration
with Nick Coutsos [Keymap-Editor](http://nickcoutsos.github.io/keymap-editor).
