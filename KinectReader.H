
#ifndef _SOLITARE_KINECT_READER_H_
#define _SOLITARE_KINECT_READER_H_

#include "Util.H"
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <opencv2/core/mat.hpp>

class KinectReader {
	public:

	KinectReader(int types);

	bool setupKinect();

	bool start();

	void stop();

	bool areFramesAvailable();

	cv::Mat & getFrame(
			const Enums::FrameType type,
			bool block);

	std::map<Enums::FrameType, cv::Mat> & getFrame(
			bool block);

	void releaseFrames();

	protected:

	void convertFrame();
	cv::Mat & convertFrame(const Enums::FrameType type);

	libfreenect2::Freenect2 freenect;
	libfreenect2::Freenect2Device * device;
	libfreenect2::FrameMap frames;
	libfreenect2::SyncMultiFrameListener listener;
	std::map<Enums::FrameType, cv::Mat> cvFrames;
	const int frameTypes;

};

#endif
