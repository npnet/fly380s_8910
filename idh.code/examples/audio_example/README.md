Audio Example
=============

The primary APIs of audio is `auPlayer_t` for playback and `auRecorder_t`
for recording. And several APIs are provided for various playback and
recording.

In this example,

* play from memory
* play from file
* play from pipe
* play from external reader
* play from external decoder
* MIC recording to memory
* MIC recording to file
* MIC recording to pipe
* MIC recording to external writer
* MIC recording to external encoder

NOTE: in audio recording, overflow is not permitted. So, encoder and writer
should be fast enough to avoid overflow. NOR flash is slow device, it may
be not fast enough for recording. Pipe is recommended method for recording.

The example just demonstrates the API sequence. In many cases, the return
value isn't checked. So, don't use them directly in products.
