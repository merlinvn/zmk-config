/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>

#include <zmk/display/status_screen.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/display/widgets/layer_status.h>
#include <zmk/display/widgets/output_status.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
static struct zmk_widget_battery_status battery_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
static struct zmk_widget_output_status output_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
static struct zmk_widget_layer_status layer_status_widget;
#endif

#if IS_ENABLED(CONFIG_ZAPHOD_BONGO_CAT)
#include "zaphod_bongo_cat_widget.h"
static struct zaphod_bongo_cat_widget bongo_widget;
#endif

// #define STATUS_FONT LV_FONT_DEFAULT
#define STATUS_FONT (&lv_font_montserrat_26)
/* Or:
 */

#define TOP_ROW_H 40
#define BOTTOM_ROW_H 40

static lv_style_t screen_style;
static lv_style_t text_style;
static bool styles_initialized;

static void init_styles(void) {
  if (styles_initialized) {
    return;
  }

  lv_style_init(&screen_style);
  lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);
  lv_style_set_bg_color(&screen_style, lv_color_black());

  lv_style_init(&text_style);
  lv_style_set_text_color(&text_style, lv_color_white());
  lv_style_set_text_font(&text_style, STATUS_FONT);

  styles_initialized = true;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, int32_t x,
                            int32_t y) {
  lv_obj_t *label = lv_label_create(parent);

  lv_obj_add_style(label, &text_style, LV_PART_MAIN);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, x, y);

  return label;
}

static void style_widget_root(lv_obj_t *obj) {
  if (obj == NULL) {
    return;
  }

  /*
   * Keep this minimal.
   * Do not recurse. Do not remove styles.
   */
  lv_obj_set_style_text_color(obj, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);

  /*
   * Try to make icon/image parts white if the widget supports recolor.
   */
  lv_obj_set_style_image_recolor(obj, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_image_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
}

#if IS_ENABLED(CONFIG_ZAPHOD_BONGO_CAT)
static void move_new_bongo_children_to_middle(lv_obj_t *screen,
                                              uint32_t child_count_before,
                                              uint32_t child_count_after,
                                              int32_t middle_y,
                                              int32_t middle_h) {
  /*
   * Save the newly-created direct children first.
   *
   * Do not call lv_obj_move_background() while iterating by index directly,
   * because moving objects changes child order.
   */
  lv_obj_t *bongo_objs[8];
  uint32_t bongo_obj_count = 0;

  for (uint32_t i = child_count_before; i < child_count_after; i++) {
    if (bongo_obj_count >= ARRAY_SIZE(bongo_objs)) {
      LOG_WRN("Too many bongo root objects; ignoring extra children");
      break;
    }

    bongo_objs[bongo_obj_count++] = lv_obj_get_child(screen, i);
  }

  for (uint32_t i = 0; i < bongo_obj_count; i++) {
    lv_obj_t *bongo_obj = bongo_objs[i];

    if (bongo_obj == NULL) {
      continue;
    }

    /*
     * Move the bongo root into the middle band.
     *
     * For 168x144 with TOP_ROW_H=40 and BOTTOM_ROW_H=40:
     *   middle_y = 40
     *   middle_h = 64
     */
    lv_obj_set_pos(bongo_obj, 0, middle_y);
    lv_obj_set_size(bongo_obj, LV_HOR_RES, middle_h);
    lv_obj_clear_flag(bongo_obj, LV_OBJ_FLAG_SCROLLABLE);

    /*
     * Keep it behind top/bottom widgets.
     */
    lv_obj_move_background(bongo_obj);
  }
}
#endif

lv_obj_t *zmk_display_status_screen(void) {
  init_styles();

  lv_obj_t *screen = lv_obj_create(NULL);

  lv_obj_remove_style_all(screen);
  lv_obj_add_style(screen, &screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(screen, LV_HOR_RES, LV_VER_RES);

  int32_t screen_h = LV_VER_RES;

  int32_t top_y = 0;
  int32_t middle_y = TOP_ROW_H;
  /* int32_t bottom_y = screen_h - BOTTOM_ROW_H; */
  int32_t middle_h = screen_h - TOP_ROW_H - BOTTOM_ROW_H;

  if (middle_h < 0) {
    middle_h = 0;
  }

#if IS_ENABLED(CONFIG_ZAPHOD_BONGO_CAT)
  /*
   * Initialize bongo before status widgets.
   *
   * The bongo widget appears to create its root object at screen (0,0).
   * We record which direct children it creates, then move them down into
   * the middle row.
   */
  uint32_t child_count_before_bongo = lv_obj_get_child_count(screen);
  zaphod_bongo_cat_widget_init(&bongo_widget, screen);
  uint32_t child_count_after_bongo = lv_obj_get_child_count(screen);

  move_new_bongo_children_to_middle(screen, child_count_before_bongo,
                                    child_count_after_bongo, middle_y,
                                    middle_h);
#else
  lv_obj_t *center_label = make_label(screen, "ZAPHOD", 0, 0);
  lv_obj_align(center_label, LV_ALIGN_TOP_MID, 0,
               middle_y + (middle_h / 2) - 8);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_OUTPUT_STATUS)
  zmk_widget_output_status_init(&output_status_widget, screen);

  lv_obj_t *output_obj = zmk_widget_output_status_obj(&output_status_widget);
  style_widget_root(output_obj);
  lv_obj_align(output_obj, LV_ALIGN_TOP_LEFT, 2, top_y + 4);
  lv_obj_move_foreground(output_obj);
#else
  make_label(screen, "OUT", 2, top_y + 4);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_BATTERY_STATUS)
  zmk_widget_battery_status_init(&battery_status_widget, screen);

  lv_obj_t *battery_obj = zmk_widget_battery_status_obj(&battery_status_widget);
  style_widget_root(battery_obj);
  lv_obj_align(battery_obj, LV_ALIGN_TOP_RIGHT, -2, top_y + 4);
  lv_obj_move_foreground(battery_obj);
#else
  lv_obj_t *battery_label = make_label(screen, "BAT", 0, top_y + 4);
  lv_obj_align(battery_label, LV_ALIGN_TOP_RIGHT, -2, top_y + 4);
#endif

#if IS_ENABLED(CONFIG_ZMK_WIDGET_LAYER_STATUS)
  zmk_widget_layer_status_init(&layer_status_widget, screen);

  lv_obj_t *layer_obj = zmk_widget_layer_status_obj(&layer_status_widget);
  style_widget_root(layer_obj);
  lv_obj_align(layer_obj, LV_ALIGN_BOTTOM_LEFT, 2, -4);
  lv_obj_move_foreground(layer_obj);
#else
  make_label(screen, "LAYER", 2, bottom_y + 4);
#endif

  lv_obj_update_layout(screen);
  lv_obj_invalidate(screen);

  return screen;
}
