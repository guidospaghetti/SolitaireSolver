# SolitaireSolver
Silly project attempting to solve a real life game of Solitaire using a Kinect v2 on an Nvidia Jetson Nano

## Dependencies
- OpenCV 4.6.0 (probably could be lower, just using 4.6.0)
- libfreenect2
- ffmpeg, with enhancements to use jetson cuda cores
	- Used a great repo [here](https://github.com/Keylost/jetson-ffmpeg)

## Random Notes
When receiving the RTP stream in VLC, set network caching to 50ms
 - Tools -> Preferences -> Show Settings, All -> Input/Codecs -> Network Caching

Need to stream to port 8044 even though port 8045 is specific in SDP file
