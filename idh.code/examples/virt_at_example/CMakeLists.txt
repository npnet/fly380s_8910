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

if(CONFIG_APPIMG_LOAD_FLASH)
    set(target virt_at_example)
    add_appimg_flash_example(${target} virt_at_example.c)
    target_include_targets(${target} PRIVATE kernel atr)
endif()

relative_glob(srcs include/*.h src/*.c src/*.h *.c)
beautify_c_code(${target} ${srcs})
