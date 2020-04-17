Virtual AT Channel Example
==========================

This is a simple example for virtual AT channel. The example uses flash
application loader feature.

To download the example, download `virt_at_example.pac` by
ResearchDownload. The result can be observed from trace. When searching
`TVAT` in trace, the followings should be observed:

```
TVAT/I : application image enter, param 0x0
TVAT/I : VAT1 -->: AT
TVAT/I : VAT1 <--(4): AT
TVAT/I : VAT1 <--(6): OK
```

`VAT1 -->` is the AT command, and `VAT1 <--` is the AT response. Also,
it is possible that URC can be observed in virtual AT channel.
