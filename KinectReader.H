
#ifndef _SOLITARE_KINECT_READER_H_
#define _SOLITARE_KINECT_READER_H_

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>

class KinectReader {
	public:

	KinectReader(int types);

	bool setupKinect();

	bool start();

	void stop();

	bool areFramesAvailable();

	libfreenect2::Frame * getFrame(
			const libfreenect2::Frame::Type type,
			bool block);

	libfreenect2::FrameMap & getFrame(
			bool block);

	void releaseFrames();

	protected:

	libfreenect2::Freenect2 freenect;
	libfreenect2::Freenect2Device * device;
	libfreenect2::FrameMap frames;
	libfreenect2::SyncMultiFrameListener listener;
	const int frameTypes;

};

#endif
