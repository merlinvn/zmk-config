# Customization guide

Instructions for working on this ZMK config — written for coding agents, but equally usable by
humans adapting this repo as a starting point for their own keymap. For background on the keymap
and the build pipeline, see the [README](README.md).

## Ground rules

- **Building needs no local setup.** Pushing to a fork builds every target in `build.yaml` through
  GitHub Actions (`.github/workflows/build-nix.yml`, triggered by pushes and PRs touching
  `config/**` or `build.yaml`); the firmware is downloaded from the Actions tab. A fresh fork has
  Actions disabled — enable them once from the fork's Actions tab.
- **Building locally** requires the nix-based dev environment (nix + direnv, see the
  [README](README.md#local-build-environment)) plus a one-time `just init`, which turns the
  repository into a west workspace by pulling in ZMK, Zephyr and the modules. Then verify keymap
  changes with `just build <target>` (`just list` shows valid targets, `just build all` builds
  everything); compiled firmware lands in `firmware/`. If `just` or `west` are missing, the
  environment isn't active — run commands through `nix develop --command <cmd>` or activate with
  `direnv allow`.
- **Building in the ZMK dev container** is also supported. The checkout at
  `/Users/neo/Projects/keyboard/zmk` has `.devcontainer/devcontainer.json`; reopen that folder in
  the container, then work from `/workspaces/zmk-config` (mounted from this repository). If the
  dev-container CLI or an active container is unavailable, the same image can be run directly:
  from the config repository root:

  ```sh
  docker run --rm -it --security-opt label=disable \
    -v "$PWD":/workspaces/zmk-config \
    -w /workspaces/zmk-config \
    zmkfirmware/zmk-dev-arm:4.1-branch /bin/bash
  ```

  On first use in this config checkout, run the pinned West setup from
  `/workspaces/zmk-config`:

  ```sh
  west init -l config
  west update --fetch-opt=--filter=blob:none
  west zephyr-export
  ```

  These fetch ZMK, Zephyr, and the modules pinned by `config/west.yml` into ignored workspace
  directories under this repo. Reuse them on later runs. The dev image does not include `just`, so
  build with West directly:

  ```sh
  west build -s zmk/app -d .build/cradio_left-nice_nano -b nice_nano -- \
    -DZMK_CONFIG=/workspaces/zmk-config/config -DSHIELD=cradio_left
  west build -s zmk/app -d .build/cradio_right-nice_nano -b nice_nano -- \
    -DZMK_CONFIG=/workspaces/zmk-config/config -DSHIELD=cradio_right
  mkdir -p firmware
  cp .build/cradio_left-nice_nano/zephyr/zmk.uf2 firmware/cradio_left-nice_nano.uf2
  cp .build/cradio_right-nice_nano/zephyr/zmk.uf2 firmware/cradio_right-nice_nano.uf2
  ```

  The pinned manifest calls the Nice!Nano V2 board `nice_nano` (revision 2.0.0), not
  `nice_nano_v2`. The standard `just` recipe copies build outputs into `firmware/` automatically
  when `just` is available.
- `config/west.yml` is maintained by [pin-west](https://github.com/urob/pin-west): never hand-edit
  pinned revisions, run `pin-west bump` instead. Adding or removing a module is fine — edit the
  entry itself and re-run `pin-west pin` to (re)pin.
- `dts-format` (packaged in the dev environment) formats devicetree files. It does not reformat
  C-preprocessor macros, so on the keymaps it is close to a no-op — the hand-aligned grids inside
  `ZMK_LAYER` are safe and need no guarding. It earns its keep on plain devicetree, i.e. when
  developing modules or board definitions. Without arguments it recurses over the working
  directory; pass files explicitly to narrow it.
- After keymap changes that affect the layout, regenerate the diagrams with `just draw` (renders
  `draw/base.svg` and `draw/overview.svg` from the 34-key base keymap).
- `just test` is a snapshot-test harness for developing the ZMK **modules** checked out under
  `modules/zmk/`. It does not test this repo's keymap; the keymap is validated by building.

## How the multi-board layout works

`config/base.keymap` defines all layers exactly once, for 34 keys, using the standardized key labels
from [zmk-helpers](https://github.com/urob/zmk-helpers): `LT0`–`LT4`, `LM0`–`LM4`, `LB0`–`LB4` and
`LH0`–`LH2` for the left top/middle/bottom rows and thumbs, mirrored with `R` on the right (`0` is
innermost). HRM trigger positions, combos (`config/combos.dtsi`), the leader key and the mouse layer
are all written against these labels — so they adapt to any board automatically.

`base.keymap` intentionally includes **no** key-labels header itself. Each board has a small entry
keymap that must, in this order:

1. optionally `#define CONFIG_WIRELESS` (enables the Bluetooth keys on the Sys layer),
2. define a `ZMK_BASE_LAYER(name, LT, RT, LM, RM, LB, RB, LH, RH)` macro that places the eight
   34-key blocks onto the board's full physical grid, filling leftover keys with `&none` or extra
   bindings,
3. include the matching key-labels header from zmk-helpers,
4. `#include "base.keymap"`.

`config/corneish_zen.keymap` is a minimal reference example; `config/planck_rev6.keymap` shows a
non-split, wired board; `config/glove80.keymap` shows a much larger board.

## Adding a new board

1. **Decide on the structure first.** The modular layout above only pays off across several
   boards. If the new board is the only one you build for, consider dropping the indirection:
   widen `base.keymap` to the board's full key count (turn the `ZMK_BASE_LAYER` calls into plain
   `ZMK_LAYER` calls spanning every physical key), add the key-labels `#include` at its top, and
   rename it to `<board>.keymap` so it becomes the entry point. Combos, HRM trigger positions and
   the mouse layer keep working as long as the key labels do. (For a true 34-key board, no adapter
   is needed either way — `base.keymap` has a pass-through fallback.) The steps below assume the
   modular layout.

2. **Identify the ZMK board/shield names** for the hardware (e.g. `nice_nano` +
   `corne_left`/`corne_right`), from the pinned ZMK workspace or the board's vendor config. Board
   names can differ between ZMK revisions; use the name recognized by the repository's pinned
   manifest rather than assuming a controller revision appears in the name.

3. **Check for an existing key-position header** in
   `modules/zmk/helpers/include/zmk-helpers/key-labels/` (after `just init`; the same list is in
   the [zmk-helpers repo](https://github.com/urob/zmk-helpers/tree/main/include/zmk-helpers/key-labels)).
   Many layouts already have one, either by name (`glove80.h`, `sofle.h`, …) or by shape (`36.h`,
   `42.h`, `4x12_wide.h`, …).

   If none matches, write the key-position defines yourself — either at the top of the new entry
   keymap or, better, as a new header contributed to zmk-helpers — following the naming and
   mirroring conventions documented in
   [key_labels.md](https://github.com/urob/zmk-helpers/blob/main/docs/key_labels.md#standardization).

4. **Create `config/<board>.keymap`** following the four-step structure above. Start from
   `corneish_zen.keymap` for wireless splits or `planck_rev6.keymap` for wired boards. When
   mapping the blocks in `ZMK_BASE_LAYER`, keep the 34 base positions in their standard relative
   locations and spend spare physical keys on duplicates or extras (the existing adapters use
   `&kp LGUI` and `&smart_mouse`).

5. **Create `config/<board>.conf`**, copying from an existing one: wireless boards want the sleep
   and Bluetooth settings from `corneish_zen.conf`; all boards want `CONFIG_ZMK_POINTING=y` for
   the mouse layer.

6. **Register the build target** in `build.yaml` under `include:`, as `board:` (plus `shield:` for
   shield-based hardware). Board revisions use ZMK's `name@rev//zmk` syntax — see the existing
   entries.

7. **Build and check**: `just build <name>` (any substring of the board/shield matches), then
   confirm the artifact appears in `firmware/`.

For a Merlinvn-backed 34-key board, `config/merlinvn/keymap.dtsi` supplies binding macros; the
entry keymap must also instantiate the `/keymap` node and its layers, following
`config/boards/petejohanson/zaphod/zaphod.keymap`. Cradio/Sweep has the same 34-key positions as
Zaphod, so its adapter can use the equivalent position definitions. For Bluetooth operation, set
`CONFIG_ZMK_BLE=y` in the shared `config/cradio.conf`; Bluetooth tuning options alone do not enable
BLE.

### Cradio/Sweep split and Bluetooth troubleshooting

- `cradio_left` is the split central; `cradio_right` is a BLE split peripheral. Only the left side
  presents a host keyboard over USB or BLE. The right side must be independently powered and paired
  to the left, and will not appear as its own keyboard in the Mac's Bluetooth list.
- Host output selection and split communication are separate. `CONFIG_ZMK_BLE=y` enables both BLE
  uses. If USB and BLE are both available, ZMK prefers USB by default; use `&out OUT_BLE` to route
  key events to the selected host Bluetooth profile while keeping USB connected for power. In this
  keymap, hold the left thumb's Nav key, hold the first right thumb key to activate Media, then tap
  the second key on the left bottom row (`&out OUT_BLE`). `OUT_TOG` is the first key in that row.
- The Media layer also has Bluetooth profile selection on `ML_MED` (`BT_SEL 0`–`BT_SEL 4`) and
  `BT_CLR` on the fifth key of `BL_MED`. A cleared or unused profile advertises for host pairing.
  If re-pairing with a host, forget the old keyboard entry on the host as well as clearing the ZMK
  profile; host-side bond data is stored separately.
- The `&sys_reset` combo in `config/merlinvn/combos.dtsi` resets only the central. To restart the
  split link, reset both controllers close together (their physical reset buttons work), or reset
  the peripheral immediately after the central. A normal reset does not erase pairing data.
- If the split bond must be cleared, flash settings-reset images to both controllers, then restore
  the matching normal images. Because `config/cradio.conf` enables BLE and USB, those settings can
  override the `settings_reset` shield's default of disabling Bluetooth. For reset builds, provide
  an extra Kconfig fragment containing:

  ```conf
  CONFIG_ZMK_BLE=n
  CONFIG_ZMK_USB=n
  ```

  Pass it as `-DEXTRA_CONF_FILE=/workspaces/zmk-config/config/cradio_reset.conf` and include both
  shields, e.g. `-DSHIELD="cradio_left settings_reset"` or
  `-DSHIELD="cradio_right settings_reset"`. Confirm the resulting `.config` has
  `CONFIG_ZMK_SETTINGS_RESET_ON_START=y` and `# CONFIG_ZMK_BLE is not set`. Flash a reset image to
  each side before restoring either normal image; then power-cycle both close together. This clears
  all on-device settings, including host Bluetooth profiles and output selection. Forget the old
  Cradio entry on the Mac if it was paired over BLE.

## Where to change what

| Change                            | File                                                              |
| --------------------------------- | ----------------------------------------------------------------- |
| Layers, HRMs, thumb keys          | `config/base.keymap`                                              |
| HRM timing                        | `config/base.keymap` (`tapping-term-ms`, `require-prior-idle-ms`; see README troubleshooting) |
| Combos                            | `config/combos.dtsi` (position diagram at the top)                |
| Leader sequences                  | `config/leader.dtsi`                                              |
| Mouse layer                       | `config/mouse.dtsi`                                               |
| Per-board keys, physical mapping  | `config/<board>.keymap`                                           |
| Board settings (BT, sleep, mouse) | `config/<board>.conf`                                             |
| Build targets                     | `build.yaml`                                                      |
| ZMK/module versions               | `config/west.yml` (via `pin-west bump` only)                      |
| Dev environment                   | `flake.nix`, `nix/` (test with `nix develop`)                     |

Removing a feature: delete its `#include` from `base.keymap` (and its bindings/combo references),
and if it was the only consumer of a module, drop the module's entry from `config/west.yml`. The
modules used per feature are commented at the top of `base.keymap`.
