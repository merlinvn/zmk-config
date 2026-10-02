# Neo's ZMK configuration

This repository builds ZMK firmware for Neo's keyboards. The Merlinvn-based 34-key layout is shared
across the Ferris Sweep/Cradio split, Planck Rev6, Neo Kinesis, Zaphod, and Technikable.

## Build targets

| Keyboard | ZMK target | Keymap |
| --- | --- | --- |
| Ferris Sweep / Cradio left | `nice_nano@2.0.0//zmk` + `cradio_left` | `config/cradio.keymap` |
| Ferris Sweep / Cradio right | `nice_nano@2.0.0//zmk` + `cradio_right` | `config/cradio.keymap` |
| Planck Rev6 | `planck@6.0.0//zmk` | `config/planck.keymap` |
| Neo Kinesis | `nice_nano` + `neo_kinesis` | `config/boards/shields/neo_kinesis/neo_kinesis.keymap` |
| Zaphod (reference) | `zaphod` | `config/boards/petejohanson/zaphod/zaphod.keymap` |
| Technikable (reference) | `technikable` | `config/boards/petejohanson/technikable/technikable.keymap` |

Cradio uses the 34-key positions directly. Planck maps the same positions onto its 4×12 matrix;
the other 14 physical keys are left unused. Board and shield build targets are listed in
[`build.yaml`](build.yaml).

## Keymap files

- `config/merlinvn/` contains the shared layer bindings, behaviors, and combos.
- Each board entry keymap maps those layers onto its physical layout. Cradio uses 34 positions;
  Planck uses the same layout on a 4×12 matrix.
- Active build targets use the shared Merlinvn layout; board entry keymaps map it to each board.
- `config/boards/` contains board and shield definitions used by this config.
- `AGENTS.md` explains the build workflow, layout conventions, and troubleshooting steps.

## Build firmware

With the local Nix environment active, build a target with `just build <target>`:

```sh
just build cradio   # builds left and right UF2 files
just build planck   # builds Planck Rev6
just build all      # builds every entry in build.yaml
```

Build artifacts are written to `firmware/`. The local environment requires Nix and direnv; run
`just init` once to initialize the pinned ZMK/Zephyr west workspace. See
[`AGENTS.md`](AGENTS.md) for the dev-container setup and direct `west build` commands.

## Cradio USB and Bluetooth

Cradio's left half is the split central. It sends keyboard input to the Mac over USB or Bluetooth.
The right half is a BLE split peripheral: it pairs with the left half and does not appear as a
separate keyboard on the Mac. Keep the right half powered when using the split.

Build both halves with the `nice_nano@2.0.0//zmk` target. The `//zmk` variant enables flash/NVS
storage so Bluetooth bonds survive resets and power cycles. The plain `nice_nano` target can
build with `CONFIG_SETTINGS_NONE=y`, losing pairing on reboot. Verify normal builds have
`CONFIG_SETTINGS_NVS=y` and `CONFIG_FLASH=y` in their generated `zephyr/.config`.

BLE is enabled in `config/cradio.conf`. When USB and Bluetooth are both available, ZMK selects USB
by default. To switch host output to Bluetooth, hold the left thumb's Nav key and the first right
thumb key to momentarily activate Fun, then press `OUT_BLE` on the right top row. Pair **Cradio**
in the Mac's Bluetooth settings. The USB cable can remain connected for power.

On Cradio, the `LT1` + `LT0` combo and the reset key in Fun use `&reset_all`: it resets the right
peripheral first, then the left central after a short delay. This requires a live split connection;
use the physical reset buttons if the link is down.

If the split link stops reconnecting, reset both controllers close together. A normal reset does not
erase pairing. Clearing the split pairing requires settings-reset firmware on both halves, then
restoring the normal left/right images; see the full recovery notes in `AGENTS.md`.
