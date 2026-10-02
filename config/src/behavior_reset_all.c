/*
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_reset_all

#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/kernel.h>
#include <drivers/behavior.h>
#include <zmk/behavior.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/central.h>

static void reset_central(struct k_work *work) { sys_reboot(SYS_REBOOT_WARM); }

static K_WORK_DELAYABLE_DEFINE(reset_central_work, reset_central);
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
static int on_binding_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    struct zmk_behavior_binding reset_binding = {
        .behavior_dev = "sysreset",
        .param1 = 0,
        .param2 = 0,
    };

    // Send the regular reset behavior to peripheral 0, then allow time for
    // its Bluetooth write to complete before rebooting the central.
    zmk_split_central_invoke_behavior(0, &reset_binding, event, true);
    k_work_schedule(&reset_central_work, K_MSEC(300));
#else
    sys_reboot(SYS_REBOOT_WARM);
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api reset_all_driver_api = {
    .binding_pressed = on_binding_pressed,
    .locality = BEHAVIOR_LOCALITY_GLOBAL,
};

#define RESET_ALL_INST(n)                                                                          \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &reset_all_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RESET_ALL_INST)
#endif
