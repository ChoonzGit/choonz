# choonz
Multicast Music Player

Stream music (anything which can be decoded by the FFMPEG library) over
multicast and listen to it on many players at once. The project currently
has a server to store track details, a multicast source which does the
actual playing, and a simple player which receives the multicast stream.

Requires Boost:ASIO and FFMPEG.
