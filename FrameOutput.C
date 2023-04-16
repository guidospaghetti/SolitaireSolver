#include "FrameOutput.H"
#include "Util.H"

FrameOutput::FrameOutput(
		const FrameOutputConfig & outputConfig) :
	FrameProcessor(),
	config(outputConfig),
	encoder(nullptr),
	doneProcessing(false)
{
	switch (config.typeOfFramesToOutput) {
		case FrameOutputConfig::RGB:
			outputFormat = libfreenect2::Frame::Color;
			requiredFormats.push_back(libfreenect2::Frame::Color);
			break;
		case FrameOutputConfig::DEPTH:
			outputFormat = libfreenect2::Frame::Depth;
			requiredFormats.push_back(libfreenect2::Frame::Depth);
			break;
		case FrameOutputConfig::IR:
			outputFormat = libfreenect2::Frame::Ir;
			requiredFormats.push_back(libfreenect2::Frame::Ir);
			break;
		case FrameOutputConfig::RGB_DEPTH_REGISTERED:
			outputFormat = libfreenect2::Frame::Color;
			requiredFormats.push_back(libfreenect2::Frame::Color);
			requiredFormats.push_back(libfreenect2::Frame::Depth);
			break;
		case FrameOutputConfig::DEPTH_RGB_REGISTERED:
			outputFormat = libfreenect2::Frame::Depth;
			requiredFormats.push_back(libfreenect2::Frame::Color);
			requiredFormats.push_back(libfreenect2::Frame::Depth);
			break;
		default:
			break;
	}
}

FrameOutput::~FrameOutput()
{
	if (encoder != nullptr) {
		pclose(encoder);
		encoder = nullptr;
	}
}

void FrameOutput::processFrame(libfreenect2::FrameMap & frame)
{
	for (const auto format : requiredFormats) {
		if (frame.find(format) == frame.end()) {
			return;
		}
	}

	if (encoder == nullptr) {
		encoder = setupOutput(frame[outputFormat]);
		if (encoder == nullptr) {
			LOG_OUT("Failed to setup output encoder, won't process more frames");
			doneProcessing = true;
			return;
		}
	}

	// TODO do registration of depth and rgb
	// TODO mirror the frame so it looks right
	if (!writeFrame(frame[outputFormat])) {
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

FILE * FrameOutput::setupOutput(libfreenect2::Frame * frame)
{
	std::string command;
	config.width = frame->width;
	config.height = frame->height;

	if (frame->format == libfreenect2::Frame::Format::RGBX) {
		config.pixelFormat = "bgr32"; // intentionally flipped
	} else if (frame->format == libfreenect2::Frame::Format::BGRX) {
		config.pixelFormat = "rgb32"; // intentionally flipped
	} else if (frame->format == libfreenect2::Frame::Format::Float) {
		config.pixelFormat = "gray16be"; // requires pixel conversion TODO
		LOG_OUT("Output of \"Float\" format frame requires unsupported "
				"conversion, giving up setting up encoder");
		return nullptr;
	} else if (frame->format == libfreenect2::Frame::Format::Gray) {
		config.pixelFormat = "gray";
	}

	if (config.outputVideo)
	{
		command = "/home/aaron/repos/ffmpeg/ffmpeg -f rawvideo "
			"-s " + std::to_string(config.width) + "x" + std::to_string(config.height) +
			" -pix_fmt " + config.pixelFormat + " -i - -sdp_file saved_sdp_file " 
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

bool FrameOutput::writeFrame(libfreenect2::Frame * frame)
{
	const int bytesToWrite = frame->width * frame->height * frame->bytes_per_pixel;
	int numWritten = 0;

	while (numWritten < bytesToWrite) {
		const int written = fwrite(&frame->data[numWritten], 1, 
			bytesToWrite - numWritten, encoder);

		if (written <= 0) {
			LOG_OUT("Failed to write image to ffmpeg");
			break;
		}
		numWritten += written;
	}

	return numWritten == bytesToWrite;
}

