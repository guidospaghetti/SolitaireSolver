
INCLUDE_DIRS = -I/usr/include/ \
		-I/usr/local/include \
		-I/usr/include/opencv4

LIB_DIRS = -L/usr/local/lib \
		-L/usr/lib/aarch64-linux-gnu

LIBS = -lfreenect2 \
	-lstdc++ \
	-lpthread \
	-lopencv_core \
	-lopencv_imgproc \
	-lopencv_dnn

CC = gcc
CFLAGS = -g -Wall

default: camera

camera: Camera.C KinectReader.o FrameOutput.o CardDetector.o
	$(CC) Camera.C KinectReader.o FrameOutput.o CardDetector.o $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -o camera

KinectReader.o: KinectReader.C
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIBS) -c KinectReader.C

FrameOutput.o: FrameOutput.C
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIB_DIRS) -c FrameOutput.C

CardDetector.o: CardDetector.C
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $(LIB_DIRS) $(LIB_DIRS) -c CardDetector.C


clean:
	/bin/rm -f camera KinectReader.o FrameOutput.o
