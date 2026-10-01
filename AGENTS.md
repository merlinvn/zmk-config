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
  [README](README.md#build-firmware)) plus a one-time `just init`, which turns the
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
  export ZEPHYR_BASE=/workspaces/zmk-config/zephyr
  west build -s zmk/app -d .build/cradio_left-nice_nano -b nice_nano -- \
    -DZephyr_DIR=/workspaces/zmk-config/zephyr/share/zephyr-package/cmake \
    -DZMK_CONFIG=/workspaces/zmk-config/config -DSHIELD=cradio_left
  west build -s zmk/app -d .build/cradio_right-nice_nano -b nice_nano -- \
    -DZephyr_DIR=/workspaces/zmk-config/zephyr/share/zephyr-package/cmake \
    -DZMK_CONFIG=/workspaces/zmk-config/config -DSHIELD=cradio_right
  west build -s zmk/app -d .build/planck-rev6 -b "planck@6.0.0//zmk" -- \
    -DZephyr_DIR=/workspaces/zmk-config/zephyr/share/zephyr-package/cmake \
    -DZMK_CONFIG=/workspaces/zmk-config/config
  mkdir -p firmware
  cp .build/cradio_left-nice_nano/zephyr/zmk.uf2 firmware/cradio_left-nice_nano.uf2
  cp .build/cradio_right-nice_nano/zephyr/zmk.uf2 firmware/cradio_right-nice_nano.uf2
  cp .build/planck-rev6/zephyr/zmk.bin firmware/planck_rev6.bin
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
- After changes to the shared upstream-style `config/base.keymap`, regenerate its diagrams with
  `just draw` (renders `draw/base.svg` and `draw/overview.svg`). The Merlinvn keymaps are separate.
- `just test` is a snapshot-test harness for developing the ZMK **modules** checked out under
  `modules/zmk/`. It does not test this repo's keymap; the keymap is validated by building.

## Keymap structure

The active board keymaps share bindings and behaviors from `config/merlinvn/`:

- Cradio/Sweep uses the 34-key position map directly in `config/cradio.keymap`.
- Planck Rev6 uses `config/planck.keymap` and `4x12_wide.h` to place the same 34 positions on its
  4×12 matrix; its extra 14 positions are `&none`.
- Neo Kinesis, Zaphod, and Technikable have board-specific keymap entry files that include the same
  Merlinvn layer definitions.

`config/base.keymap`, `config/combos.dtsi`, `config/leader.dtsi`, and `config/mouse.dtsi` are the
older generic keymap retained for the `just draw` diagrams. They are not included by the active
firmware targets. Keep changes to the active Merlinvn map in `config/merlinvn/` and its board entry
keymaps instead.

## Adding a board

1. Identify the board/shield name in the pinned ZMK workspace; revisions use `name@rev//zmk`.
2. Reuse a matching key-label header from `modules/zmk/helpers/include/zmk-helpers/key-labels/`.
3. Create `config/<board>.keymap`, using `cradio.keymap` for a 34-key Merlinvn layout or
   `planck.keymap` for a 4×12 adaptation. Keep the shared 34 positions in the same order.
4. Create `config/<board>.conf`; Cradio's BLE and sleep settings are in `cradio.conf`, and pointing
   boards need `CONFIG_ZMK_POINTING=y` for the mouse layer.
5. Add the target to `build.yaml`, then run `just build <target>` and check `firmware/`.

For Cradio host Bluetooth, set `CONFIG_ZMK_BLE=y` in `config/cradio.conf`; Bluetooth tuning options
alone do not enable BLE.

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
| Merlinvn layers, HRMs, thumb keys | `config/merlinvn/keymap.dtsi`, `config/merlinvn/behaviors.dtsi`  |
| Legacy diagram keymap             | `config/base.keymap`                                               |
| Combos                            | `config/merlinvn/combos.dtsi`                                      |
| Leader sequences (legacy)         | `config/leader.dtsi`                                               |
| Mouse layer                       | `config/merlinvn/mouse.dtsi`                                       |
| Per-board keys, physical mapping  | `config/<board>.keymap`                                            |
| Board settings (BT, sleep, mouse) | `config/<board>.conf`                                              |
| Build targets                     | `build.yaml`                                                       |
| ZMK/module versions               | `config/west.yml` (via `pin-west bump` only)                       |
| Dev environment                   | `flake.nix`, `nix/` (test with `nix develop`)                      |

Removing a feature: delete its `#include` and bindings from the relevant keymap files, and if it was
the only consumer of a module, drop the module's entry from `config/west.yml`. The modules used per
feature are commented at the top of the corresponding keymap or behavior files.
