Application Loader Example
==========================

This is a simple example for application loader feature. With
`ninja examples`, the followings will be generated:

* `appimg_hello_example_flash.pac`: this is the example to load application from flash.
* `appimg_hello_example_file.img`: this is the example to load application from file.

Load from Flash
---------------

`appimg_hello_example_flash.pac` can be downloaded to module with
ResearchDownload. The result can be observed from trace. When searching
`MYAP` in trace, the followings should be observed:

```
MYAP/I : application image enter, param 0x0
MYAP/I : data=1 (expect 1), bss=0
MYAP/I : application thread enter, param 0x0
MYAP/I : hello world 0
MYAP/I : hello world 1
MYAP/I : hello world 2
MYAP/I : hello world 3
MYAP/I : hello world 4
MYAP/I : hello world 5
MYAP/I : hello world 6
MYAP/I : hello world 7
MYAP/I : hello world 8
MYAP/I : hello world 9
```

The second trace line is to verify whether `.data` section of application is
correctly loaded, and `.bss` section is correctly cleared.

Erase Application On Flash
--------------------------

Application on flash can be erased by downloading `appimg_flash_delete.pac`
with ResearchDownload.

Load from File
--------------

`appimg_hello_example_file.pac` can be downloaded to module by
ResearchDownload. It will push `CONFIG_APPIMG_LOAD_FILE_NAME` to module.

The result can be observed from trace, and the trace content is the same as
application on flash.

Delete Application on File
-------------------------

Application on file can be erased by downloading `appimg_file_delete.pac`
with ResearchDownload. It will delete `CONFIG_APPIMG_LOAD_FILE_NAME`.
