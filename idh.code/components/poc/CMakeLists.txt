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
	gui/lv_apps/lv_poc_member_call/lv_poc_member_call.c
	gui/lv_apps/lv_poc_notation/lv_poc_notation.c
	gui/lv_apps/lv_poc_led/lv_poc_led.c
	gui/lv_apps/lv_poc_sntp/lv_poc_sntp.c

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

	gui/resource/font/lv_poc_font_msyh_1bit_2500_10.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_11.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_12.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_13.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_14.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_15.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_16.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_17.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_18.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_19.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_20.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_21.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_22.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_23.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_24.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_25.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_26.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_27.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_28.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_29.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_30.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_31.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_32.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_33.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_34.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_35.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_36.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_37.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_38.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_39.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_40.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_41.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_42.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_43.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_44.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_45.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_46.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_47.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_48.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_49.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_50.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_51.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_52.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_53.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_54.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_55.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_56.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_57.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_58.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_59.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_60.c
	gui/resource/font/lv_poc_font_msyh_1bit_2500_9.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_10.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_11.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_12.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_13.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_14.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_15.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_16.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_17.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_18.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_19.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_20.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_21.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_22.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_23.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_24.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_25.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_26.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_27.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_28.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_29.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_30.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_31.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_32.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_33.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_34.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_35.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_36.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_37.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_38.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_39.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_40.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_41.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_42.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_43.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_44.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_45.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_46.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_47.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_48.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_49.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_50.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_51.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_52.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_53.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_54.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_55.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_56.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_57.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_58.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_59.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_60.c
	gui/resource/font/lv_poc_font_msyh_1bit_3500_9.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_10.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_11.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_12.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_13.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_14.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_15.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_16.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_17.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_18.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_19.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_20.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_21.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_22.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_23.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_24.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_25.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_26.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_27.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_28.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_29.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_30.c
	gui/resource/font/lv_poc_font_msyh_1bit_6500_9.c

	gui/resource/audiofile/lv_poc_audio_msg.c
	gui/resource/audiofile/lv_poc_audio_start_machine.c
	gui/resource/audiofile/lv_poc_audio_no_connected.c
	gui/resource/audiofile/lv_poc_audio_no_login.c
	gui/resource/audiofile/lv_poc_audio_fail_to_update_group.c
	gui/resource/audiofile/lv_poc_audio_fail_to_update_member.c
	gui/resource/audiofile/lv_poc_audio_success_to_build_group.c
	gui/resource/audiofile/lv_poc_audio_low_battery.c
	gui/resource/audiofile/lv_poc_audio_success_member_call.c
	gui/resource/audiofile/lv_poc_audio_join_group.c
	gui/resource/audiofile/lv_poc_audio_offline_member.c
	gui/resource/audiofile/lv_poc_audio_insert_sim_card.c
	gui/resource/audiofile/lv_poc_audio_success_to_login.c
	gui/resource/audiofile/lv_poc_audio_tone_cannot_speak.c
	gui/resource/audiofile/lv_poc_audio_tone_lost_mic.c
	gui/resource/audiofile/lv_poc_audio_tone_note.c
	gui/resource/audiofile/lv_poc_audio_tone_start_listen.c
	gui/resource/audiofile/lv_poc_audio_tone_start_speak.c
	gui/resource/audiofile/lv_poc_audio_tone_stop_listen.c
	gui/resource/audiofile/lv_poc_audio_tone_stop_sepak.c

	idt/src/app_test.cpp
	idt/core/idt-freertos.o

	guiIdtCom/src/guiIdtCom.cpp
	
	)

set_target_properties(${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${out_lib_dir})
target_compile_definitions(${target} PRIVATE OSI_LOG_TAG=LOG_TAG_POC)
target_include_directories(${target} PUBLIC include gui idt/include idt/src guiIdtCom/include guiIdtCom/src cJSON)
target_include_targets(${target} PRIVATE kernel driver hal fs audio ml lvgl cfw nvm tts net lwip)


relative_glob(srcs */*.h */*.c)
beautify_c_code(${target} ${srcs})
