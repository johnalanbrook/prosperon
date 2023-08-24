# Yugine video playing

Yugine plays the open source MPEG-TS muxer, using MPEG1 video and MP2 audio.

MPEG-1 video works best at about 1.5 Mbit/s, in SD [720x480] video.
For HD [1920x1080] video, use 9 Mbit/s.

ffmpeg -i "input.video" -vcodec mpeg1video -b:v 1.5M -s 720x480 -acodec mp2 "out.ts"
ffmpeg -i "input.video" -vcodec mpeg1video -b:v 9M -s 1920x1080 -acodec mp2 "out.ts"
