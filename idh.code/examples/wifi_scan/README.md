Wifi Scan Example
==========================

This is a simple example for wifi scan. The example uses flash
application loader feature.

To download the example, download `wifi_scan_example.pac` by
ResearchDownload. The result can be observed from trace. When searching
`WIFI` in trace, the followings should be observed:

```
wifi scan channel 4/500
found ap - {mac address: aabbccddeeff, rssival: -94 dBm, channel: 4}
wifi scan all 1 rounds
found ap - {mac address: aabbccddeeff, rssival: -94 dBm, channel: 4}
found ap - {mac address: cdefaadd1122, rssival: -77 dBm, channel: 5}
...
```
