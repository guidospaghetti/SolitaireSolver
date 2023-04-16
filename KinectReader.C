
#include "Util.H"
#include "KinectReader.H"

#include <opencv2/imgproc.hpp>

KinectReader::KinectReader(int frameTypes) :
	device(nullptr),
	listener(frameTypes),
	frameTypes(frameTypes)
{}

bool KinectReader::setupKinect()
{
	if (freenect.enumerateDevices() == 0) {
		LOG_OUT("No Kinect devices detected");
		return false;
	}

	std::string serialNumber = freenect.getDefaultDeviceSerialNumber();
	device = freenect.openDevice(serialNumber);
	if (device == nullptr) {
		LOG_OUT("Failed to open device");
		return false;
	}

	if (frameTypes & 
		(Enums::FrameType::RGB | 
		 Enums::FrameType::RGB_DEPTH_REGISTERED |
		 Enums::FrameType::DEPTH_RGB_REGISTERED)) {
		device->setColorFrameListener(&listener);
	}
	if (frameTypes &
		(Enums::FrameType::IR |
		 Enums::FrameType::DEPTH |
		 Enums::FrameType::RGB_DEPTH_REGISTERED |
		 Enums::FrameType::DEPTH_RGB_REGISTERED)) {
		device->setIrAndDepthFrameListener(&listener);
	}

	LOG_OUT("Finished setup up kinect connection");
	return true;
}

bool KinectReader::start()
{
	if (device == nullptr) {
		LOG_OUT("No device created yet, cannot start");
		return false;
	}

	const bool startRGB = 
		frameTypes & 
			(Enums::FrameType::RGB | 
			 Enums::FrameType::RGB_DEPTH_REGISTERED |
			 Enums::FrameType::DEPTH_RGB_REGISTERED);

	const bool startDepth = 
		frameTypes & 
			(Enums::FrameType::IR |
			 Enums::FrameType::DEPTH |
			 Enums::FrameType::RGB_DEPTH_REGISTERED |
			 Enums::FrameType::DEPTH_RGB_REGISTERED);

	if (!device->startStreams(startRGB, startDepth)) {
		LOG_OUT("Failed to start device");
		return false;
	}
	return true;
}

void KinectReader::stop()
{
	if (device == nullptr) {
		LOG_OUT("No device created yet, cannot stop");
		return;
	}

	releaseFrames();

	device->stop();
	device->close();

	// Something deletes the device object so don't
	// delete it here
}

bool KinectReader::areFramesAvailable()
{
	return listener.hasNewFrame();
}

cv::Mat & KinectReader::getFrame(
		const Enums::FrameType type, 
		bool block)
{
	static cv::Mat emptyFrame;
	if ((frameTypes & type) == 0) {
		LOG_OUT("Requested frame type %d but not setup to retrieve "
				"that frame type", type);
		return emptyFrame;
	}

	auto itr = cvFrames.find(type);
	if (itr != cvFrames.end()) {
		return itr->second;
	}

	if (listener.hasNewFrame())
	{
		listener.waitForNewFrame(frames, 0);
	}
	else if (block)
	{
		listener.waitForNewFrame(frames);
	}
	else
	{
		//LOG_OUT("Requested frame non-blocking, but nothing available");
		return emptyFrame;
	}

	// TODO registration of color and depth

	return convertFrame(type);
}

std::map<Enums::FrameType, cv::Mat> & KinectReader::getFrame(bool block)
{
	if (cvFrames.empty() == false) {
		return cvFrames;
	}

	if (listener.hasNewFrame())
	{
		listener.waitForNewFrame(frames, 0);
	}
	else if (block)
	{
		listener.waitForNewFrame(frames);
	}

	if (frames.empty()) {
		LOG_OUT("Didn't get any frames even after attempting");
		return cvFrames;
	}

	convertFrame();

	return cvFrames;
}

void KinectReader::releaseFrames()
{
	listener.release(frames);
	cvFrames.clear();
}

void KinectReader::convertFrame()
{
	// TODO Mirror the frames to look right
	if (frameTypes & Enums::FrameType::RGB &&
		frames.find(libfreenect2::Frame::Color) != frames.end()) 
	{
		libfreenect2::Frame * rgb = frames[libfreenect2::Frame::Color];
		cv::Mat cvRgb(rgb->height, rgb->width, CV_8UC4, rgb->data);
		//cv::Mat cvRgb;
		//cvRgbTmp.copyTo(cvRgb);
		cv::cvtColor(cvRgb, cvRgb, cv::COLOR_RGBA2BGR);
		cvFrames[Enums::FrameType::RGB] = cvRgb;
	}
	if (frameTypes & Enums::FrameType::DEPTH &&
		frames.find(libfreenect2::Frame::Depth) != frames.end())
	{
		libfreenect2::Frame * depth = frames[libfreenect2::Frame::Depth];
		cv::Mat cvDepthTmp(depth->height, depth->width, CV_32FC1, depth->data);
		cv::Mat cvDepth;
		cvDepthTmp.copyTo(cvDepth);
		cvFrames[Enums::FrameType::DEPTH] = cvDepth;
	}
	if (frameTypes & Enums::FrameType::IR &&
		frames.find(libfreenect2::Frame::Ir) != frames.end())
	{
		libfreenect2::Frame * ir = frames[libfreenect2::Frame::Ir];
		cv::Mat cvIrTmp(ir->height, ir->width, CV_8UC1, ir->data);
		cv::Mat cvIr;
		cvIrTmp.copyTo(cvIr);
		cvFrames[Enums::FrameType::IR] = cvIr;
	}

	// TODO Depth and RGB registration
}

cv::Mat & KinectReader::convertFrame(const Enums::FrameType type)
{
	convertFrame();

	static cv::Mat emptyFrame;
	auto itr = cvFrames.find(type);
	if (itr != cvFrames.end()) {
		return itr->second;
	}
	return emptyFrame;
}
