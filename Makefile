
INCLUDE_DIRS = -I/usr/include/ \
		-I/usr/local/include \
		-I/usr/include/opencv4

LIB_DIRS = -L/usr/local/lib \
		-L/usr/lib/aarch64-linux-gnu

LIBS = -lfreenect2 \
	-lstdc++ \
	-lopencv_core \
	-lopencv_imgproc

CC = gcc
CFLAGS = -g -Wall

default: camera

camera: Camera.C KinectReader.o FrameOutput.o
	$(CC) Camera.C KinectReader.o FrameOutput.o $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -o camera

KinectReader.o: KinectReader.C
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -c KinectReader.C

FrameOutput.o: FrameOutput.C
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIB_DIRS) -c FrameOutput.C

clean:
	/bin/rm -f camera KinectReader.o FrameOutput.o
