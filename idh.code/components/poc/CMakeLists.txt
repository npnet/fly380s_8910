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

configure_file(include/poc_config.h.in ${BINARY_TOP_DIR}/include/poc_config.h)

set(target poc)
add_library(${target} STATIC
	src/poc.c 
	src/poc_audio_player.c 
	src/poc_audio_recorder.c 
	src/poc_keypad.c
	
	gui/lv_objx/lv_poc_obj/lv_poc_obj.c
	gui/lv_objx/lv_poc_list/lv_poc_list.c
	gui/lv_objx/lv_poc_rb/lv_poc_rb.c
	gui/lv_objx/lv_poc_win/lv_poc_win.c
	
	gui/lv_external/lv_poc_lib.c
	
	gui/lv_apps/lv_poc_about/lv_poc_about.c
	gui/lv_apps/lv_poc_build_group/lv_poc_build_group.c
	gui/lv_apps/lv_poc_edeg_key/lv_poc_edeg_key.c
	gui/lv_apps/lv_poc_group_list/lv_poc_group_list.c
	gui/lv_apps/lv_poc_idle/lv_poc_idle.c
	gui/lv_apps/lv_poc_main_menu/lv_poc_main_menu.c
	gui/lv_apps/lv_poc_member_list/lv_poc_member_list.c
	gui/lv_apps/lv_poc_setting/lv_poc_setting.c
	gui/lv_apps/lv_poc_setting/lv_poc_display/lv_poc_display.c
	gui/lv_apps/lv_poc_setting/lv_poc_display/lv_poc_bright.c
	gui/lv_apps/lv_poc_setting/lv_poc_display/lv_poc_theme.c
	gui/lv_apps/lv_poc_setting/lv_poc_net/lv_poc_net.c
	gui/lv_apps/lv_poc_setting/lv_poc_sim/lv_poc_sim.c
	gui/lv_apps/lv_poc_setting/lv_poc_volum/lv_poc_volum.c
	gui/lv_apps/lv_poc_single_call/lv_poc_single_call.c
	
	gui/resource/image/battery/stat_sys_no_battery.c
	gui/resource/image/battery/stat_sys_battery_0.c
	gui/resource/image/battery/stat_sys_battery_15.c
	gui/resource/image/battery/stat_sys_battery_28.c
	gui/resource/image/battery/stat_sys_battery_43.c
	gui/resource/image/battery/stat_sys_battery_57.c
	gui/resource/image/battery/stat_sys_battery_71.c
	gui/resource/image/battery/stat_sys_battery_85.c
	gui/resource/image/battery/stat_sys_battery_100.c
	gui/resource/image/battery/stat_sys_battery_charge_anim0.c
	gui/resource/image/battery/stat_sys_battery_charge_anim15.c
	gui/resource/image/battery/stat_sys_battery_charge_anim28.c
	gui/resource/image/battery/stat_sys_battery_charge_anim43.c
	gui/resource/image/battery/stat_sys_battery_charge_anim57.c
	gui/resource/image/battery/stat_sys_battery_charge_anim71.c
	gui/resource/image/battery/stat_sys_battery_charge_anim85.c
	gui/resource/image/battery/stat_sys_battery_charge_anim100.c
	gui/resource/image/main_menu/ic_group.c
	gui/resource/image/main_menu/ic_history.c
	gui/resource/image/main_menu/ic_launcher.c
	gui/resource/image/main_menu/ic_launcher_round.c
	gui/resource/image/main_menu/ic_member_offline.c
	gui/resource/image/main_menu/ic_member_online.c
	gui/resource/image/main_menu/ic_menu_about.c
	gui/resource/image/main_menu/ic_menu_friend.c
	gui/resource/image/main_menu/ic_playing.c
	gui/resource/image/main_menu/ic_select.c
	gui/resource/image/main_menu/ic_setting.c
	gui/resource/image/main_menu/ic_toolbar_bg.c
	gui/resource/image/poc_base/background_240x192.c
	gui/resource/image/poc_base/background_240x216.c
	gui/resource/image/poc_base/background_240x240.c
	gui/resource/image/poc_base/battery_30x24.c
	gui/resource/image/poc_base/control_240x24.c
	gui/resource/image/poc_base/font_colon_60px.c
	gui/resource/image/poc_base/font_eight_60px.c
	gui/resource/image/poc_base/font_five_60px.c
	gui/resource/image/poc_base/font_four_60px.c
	gui/resource/image/poc_base/font_nine_60px.c
	gui/resource/image/poc_base/font_one_60px.c
	gui/resource/image/poc_base/font_seven_60px.c
	gui/resource/image/poc_base/font_six_60px.c
	gui/resource/image/poc_base/font_three_60px.c
	gui/resource/image/poc_base/font_two_60px.c
	gui/resource/image/poc_base/font_zero_60px.c
	gui/resource/image/poc_base/ic_launcher_bg.c
	gui/resource/image/poc_base/ic_launcher_bg_dark.c
	gui/resource/image/poc_base/sim1_30x24.c
	gui/resource/image/poc_base/sim2_30x24.c
	gui/resource/image/poc_base/status_bar_240x24.c
	gui/resource/image/poweron/img_poweron_poc_logo.c
	gui/resource/image/poweron/img_poweron_poc_logo_tlt.c
	gui/resource/image/signal/ic_no_sim.c
	gui/resource/image/signal/ic_signal_1_1.c
	gui/resource/image/signal/ic_signal_1_2.c
	gui/resource/image/signal/ic_signal_1_3.c
	gui/resource/image/signal/ic_signal_1_4.c
	gui/resource/image/signal/ic_signal_2_1.c
	gui/resource/image/signal/ic_signal_2_2.c
	gui/resource/image/signal/ic_signal_2_3.c
	gui/resource/image/signal/ic_signal_2_4.c
	gui/resource/image/signal/ic_signal_no_signal.c
	gui/resource/image/signal/stat_sys_data_connected_1x_sprd.c
	gui/resource/image/signal/stat_sys_data_connected_3g_sprd.c
	gui/resource/image/signal/stat_sys_data_connected_4g_sprd.c
	gui/resource/image/signal/stat_sys_data_connected_e_sprd.c
	gui/resource/image/signal/stat_sys_signal_in_sprd.c
	gui/resource/image/signal/stat_sys_signal_inout_sprd.c
	gui/resource/image/signal/stat_sys_signal_out_sprd.c
	
	gui/resource/font/lv_poc_font_noto_sans_cjk_9.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_10.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_11.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_12.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_13.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_14.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_15.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_16.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_17.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_18.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_19.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_20.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_21.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_22.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_23.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_24.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_25.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_26.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_27.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_28.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_29.c
	gui/resource/font/lv_poc_font_noto_sans_cjk_30.c
	)
	
set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_POC)
target_include_directories(${target} PUBLIC include gui)
target_include_targets(${target} PRIVATE kernel driver hal fs audio ml lvgl)

relative_glob(srcs */*.h */*.c)
beautify_c_code(${target} ${srcs})
