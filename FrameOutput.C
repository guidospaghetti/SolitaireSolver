#include "FrameOutput.H"
#include "Util.H"

FrameOutput::FrameOutput(
		const FrameOutputConfig & outputConfig) :
	FrameProcessor(),
	config(outputConfig),
	encoder(nullptr),
	doneProcessing(false)
{}

FrameOutput::~FrameOutput()
{
	if (encoder != nullptr) {
		pclose(encoder);
		encoder = nullptr;
	}
}

void FrameOutput::processFrame(std::map<Enums::FrameType, cv::Mat> & frame)
{
	if (frame.find(config.typeOfFramesToOutput) == frame.end()) {
		return;
	}

	if (encoder == nullptr) {
		encoder = setupOutput(frame[config.typeOfFramesToOutput]);
		if (encoder == nullptr) {
			LOG_OUT("Failed to setup output encoder, won't process more frames");
			doneProcessing = true;
			return;
		}
	}

	if (!writeFrame(frame[config.typeOfFramesToOutput])) {
		LOG_OUT("Failed to output the frame! Won't process more frames");
		doneProcessing = true;
	}

	if (config.numberOfFramesToOutput > 0 &&
		++framesProcessed >= config.numberOfFramesToOutput) {
		LOG_OUT("Reached required number of frames, won't process more frames");
		doneProcessing = true;
	}
}

bool FrameOutput::finishedWithFrame()
{
	// If this was threaded, would check with thread to see if complete
	return true;
}

bool FrameOutput::finishedProcessing()
{
	return doneProcessing;
}

FILE * FrameOutput::setupOutput(cv::Mat & frame)
{
	std::string command;
	config.width = frame.cols;
	config.height = frame.rows;

	switch (frame.type()) {
		case CV_8UC4:
			// assuming libfreenect2 frame converted appropriately to bgra
			config.pixelFormat = "bgra";
			break;
		case CV_8UC3:
			// assuming libfreenect2 frame converted appropriately to bgr24
			config.pixelFormat = "bgr24";
			break;
		case CV_8UC1:
			config.pixelFormat = "gray";
			break;
		case CV_16UC1:
			config.pixelFormat = "gray16be";
			break;
		case CV_32FC1:
			config.pixelFormat = "gray16be";
			LOG_OUT("Pixel format requires unsupported conversion for now, "
					"giving up setting up encoder");
			return nullptr;
		default:
			LOG_OUT("Don't understand pixel format %d", frame.type());
			return nullptr;
	}

	if (config.outputVideo)
	{
		// Use built ffmpeg tweaked for jetson 
		command = "/home/aaron/repos/ffmpeg/ffmpeg -f rawvideo "
			"-s " + std::to_string(config.width) + "x" + std::to_string(config.height) +
			" -pix_fmt " + config.pixelFormat + " -i - -sdp_file saved_sdp_file.sdp " 
			"-vcodec h264_nvmpi -f rtp rtp://" + config.outputDestination;
	}
	else
	{
		// TODO determine output format from destination
		command = "ffmpeg -y -f rawvideo "
			"-s " + std::to_string(config.width) + "x" + std::to_string(config.height) +
			" -pix_fmt " + config.pixelFormat + " -i - "
			"-f image2 -vframes " + std::to_string(config.numberOfFramesToOutput) +
			" -vcodec png " + config.outputDestination;
	}
	LOG_OUT("Running: %s", command.c_str());

	// TODO check if ffmpeg exits for any reason and stop outputting
	// likely requires spawning off thread blocking on a wait() call
	return popen(command.c_str(), "w");
}

bool FrameOutput::writeFrame(cv::Mat & frame)
{
	// elemSize accounts for number of channels i.e. CV_8UC4 = 4 bytes
	const int bytesToWrite = frame.rows * frame.cols * frame.elemSize();
	int numWritten = 0;

	while (numWritten < bytesToWrite) {
		const int written = fwrite(frame.ptr(numWritten), 1, 
			bytesToWrite - numWritten, encoder);

		if (written <= 0) {
			LOG_OUT("Failed to write image to ffmpeg");
			break;
		}
		numWritten += written;
	}

	return numWritten == bytesToWrite;
}

