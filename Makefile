
INCLUDE_DIRS = -I/usr/include/ \
		-I/usr/local/include \
		-I/usr/include/opencv4

LIB_DIRS = -L/usr/local/lib \
		-L/usr/lib/aarch64-linux-gnu

LIBS = -lfreenect2 \
	-lstdc++ \
	#-lopencv_core \
	#-lopencv_highgui \
	#-lopencv_imgproc

CC = gcc
CFLAGS = -g -Wall

default: Camera.C

camera: KinectReader.C FrameOutput.C
	$(CC) Camera.C KinectReader.o FrameOutput.o $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -o camera

Camera.C: camera
KinectReader.C: KinectReader.o
FrameOutput.C: FrameOutput.o

KinectReader.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -c KinectReader.C

FrameOutput.o:
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIB_DIRS) -c FrameOutput.C

clean:
	/bin/rm -f camera KinectReader.o FrameOutput.o
