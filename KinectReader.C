
#include "Util.H"
#include "KinectReader.H"

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

	if (frameTypes & libfreenect2::Frame::Color) {
		device->setColorFrameListener(&listener);
	}
	if ((frameTypes & libfreenect2::Frame::Depth) ||
		(frameTypes & libfreenect2::Frame::Ir)) {
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

	const bool startRGB = frameTypes & libfreenect2::Frame::Color;
	const bool startDepth = frameTypes & libfreenect2::Frame::Ir || 
							frameTypes & libfreenect2::Frame::Depth;

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

libfreenect2::Frame * KinectReader::getFrame(
		const libfreenect2::Frame::Type type, 
		bool block)
{
	if ((frameTypes & type) == 0) {
		LOG_OUT("Requested frame type %d but not setup to retrieve "
				"that frame type", type);
		return nullptr;
	}

	auto itr = frames.find(type);
	if (itr != frames.end()) {
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
		return nullptr;
	}

	// TODO registration of color and depth

	itr = frames.find(type);
	if (itr != frames.end()) {
		return itr->second;
	} else {
		LOG_OUT("Didn't get frame type %d even after getting new frames", type);
		return nullptr;
	}
}

libfreenect2::FrameMap & KinectReader::getFrame(bool block)
{
	if (frames.empty() == false) {
		return frames;
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
	}

	return frames;
}

void KinectReader::releaseFrames()
{
	listener.release(frames);
}

