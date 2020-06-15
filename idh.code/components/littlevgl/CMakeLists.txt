# Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# RDA assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. RDA reserves the right to make changes in the
# software without notification.  RDA also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

configure_file(include/lv_gui_config.h.in ${BINARY_TOP_DIR}/include/lv_gui_config.h)

set(target lvgl)
# add_app_libraries($<TARGET_FILE:${target}>)

add_library(${target} STATIC ${dummy_c_file})
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_LVGL)
target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SRC_DIR} lvgl include lv_lib_lodepng)
target_include_targets(${target} PRIVATE kernel driver poc hal)

target_sources(${target} PRIVATE
    lvgl/src/lv_core/lv_group.c
    lvgl/src/lv_core/lv_indev.c
    lvgl/src/lv_core/lv_disp.c
    lvgl/src/lv_core/lv_obj.c
    lvgl/src/lv_core/lv_refr.c
    lvgl/src/lv_core/lv_style.c
    lvgl/src/lv_core/lv_debug.c
    lvgl/src/lv_draw/lv_draw_basic.c
    lvgl/src/lv_draw/lv_draw.c
    lvgl/src/lv_draw/lv_draw_rect.c
    lvgl/src/lv_draw/lv_draw_label.c
    lvgl/src/lv_draw/lv_draw_line.c
    lvgl/src/lv_draw/lv_draw_img.c
    lvgl/src/lv_draw/lv_draw_arc.c
    lvgl/src/lv_draw/lv_draw_triangle.c
    lvgl/src/lv_draw/lv_img_decoder.c
    lvgl/src/lv_draw/lv_img_cache.c
    lvgl/src/lv_font/lv_font.c
    lvgl/src/lv_font/lv_font_fmt_txt.c
    lvgl/src/lv_font/lv_font_roboto_12.c
    lvgl/src/lv_font/lv_font_roboto_16.c
    lvgl/src/lv_font/lv_font_roboto_22.c
    lvgl/src/lv_font/lv_font_roboto_28.c
    lvgl/src/lv_font/lv_font_unscii_8.c
    lvgl/src/lv_hal/lv_hal_disp.c
    lvgl/src/lv_hal/lv_hal_indev.c
    lvgl/src/lv_hal/lv_hal_tick.c
    lvgl/src/lv_misc/lv_circ.c
    lvgl/src/lv_misc/lv_area.c
    lvgl/src/lv_misc/lv_task.c
    lvgl/src/lv_misc/lv_fs.c
    lvgl/src/lv_misc/lv_anim.c
    lvgl/src/lv_misc/lv_mem.c
    lvgl/src/lv_misc/lv_ll.c
    lvgl/src/lv_misc/lv_color.c
    lvgl/src/lv_misc/lv_txt.c
    lvgl/src/lv_misc/lv_math.c
    lvgl/src/lv_misc/lv_log.c
    lvgl/src/lv_misc/lv_gc.c
    lvgl/src/lv_misc/lv_utils.c
    lvgl/src/lv_misc/lv_async.c
    lvgl/src/lv_misc/lv_printf.c
    lvgl/src/lv_misc/lv_bidi.c
    lvgl/src/lv_objx/lv_arc.c
    lvgl/src/lv_objx/lv_bar.c
    lvgl/src/lv_objx/lv_cb.c
    lvgl/src/lv_objx/lv_cpicker.c
    lvgl/src/lv_objx/lv_ddlist.c
    lvgl/src/lv_objx/lv_kb.c
    lvgl/src/lv_objx/lv_line.c
    lvgl/src/lv_objx/lv_mbox.c
    lvgl/src/lv_objx/lv_preload.c
    lvgl/src/lv_objx/lv_roller.c
    lvgl/src/lv_objx/lv_table.c
    lvgl/src/lv_objx/lv_tabview.c
    lvgl/src/lv_objx/lv_tileview.c
    lvgl/src/lv_objx/lv_btn.c
    lvgl/src/lv_objx/lv_calendar.c
    lvgl/src/lv_objx/lv_chart.c
    lvgl/src/lv_objx/lv_canvas.c
    lvgl/src/lv_objx/lv_gauge.c
    lvgl/src/lv_objx/lv_label.c
    lvgl/src/lv_objx/lv_list.c
    lvgl/src/lv_objx/lv_slider.c
    lvgl/src/lv_objx/lv_ta.c
    lvgl/src/lv_objx/lv_spinbox.c
    lvgl/src/lv_objx/lv_btnm.c
    lvgl/src/lv_objx/lv_cont.c
    lvgl/src/lv_objx/lv_img.c
    lvgl/src/lv_objx/lv_imgbtn.c
    lvgl/src/lv_objx/lv_led.c
    lvgl/src/lv_objx/lv_lmeter.c
    lvgl/src/lv_objx/lv_page.c
    lvgl/src/lv_objx/lv_sw.c
    lvgl/src/lv_objx/lv_win.c
    lvgl/src/lv_themes/lv_theme_alien.c
    lvgl/src/lv_themes/lv_theme.c
    lvgl/src/lv_themes/lv_theme_default.c
    lvgl/src/lv_themes/lv_theme_night.c
    lvgl/src/lv_themes/lv_theme_templ.c
    lvgl/src/lv_themes/lv_theme_zen.c
    lvgl/src/lv_themes/lv_theme_material.c
    lvgl/src/lv_themes/lv_theme_nemo.c
    lvgl/src/lv_themes/lv_theme_mono.c
	lv_lib_lodepng/lodepng.c
	lv_lib_lodepng/lv_lodepng.c
)

if(CONFIG_SOC_8910)
    target_sources(${target} PRIVATE
        lv_port/lv_gui_main.c
        lv_port/lv_gui_example1.c
        lv_port/lv_gui_example2.c
    )
endif()


relative_glob(srcs include/*.h 
					lv_lib_lodepng/*.h 
					lvgl/*.h 
					*/*.c 
					*.h
					*.c
					lvgl/porting/*.h
					lvgl/porting/*.c
					lvgl/src/*.h
					lvgl/src/*.c
					lvgl/src/lv_core/*.h
					lvgl/src/lv_core/*.c
					lvgl/src/lv_draw/*.h
					lvgl/src/lv_draw/*.c
					lvgl/src/lv_font/*.h
					lvgl/src/lv_font/*.c
					lvgl/src/lv_hal/*.h
					lvgl/src/lv_hal/*.c
					lvgl/src/lv_misc/*.h
					lvgl/src/lv_misc/*.c
					lvgl/src/lv_themes*.h
					lvgl/src/lv_themes/*.c
					lvgl/src/lv_objx/*.h
					lvgl/src/lv_objx/*.c)
beautify_c_code(${target} ${srcs})
