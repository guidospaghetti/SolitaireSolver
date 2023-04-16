#include <string>

class FrameOutputConfig {
	public:
	
	enum FrameType {
		RGB,
		DEPTH,
		IR,
		RGB_DEPTH_REGISTERED,
		DEPTH_RGB_REGISTERED
	};

	FrameOutputConfig() :
		outputVideo(true),
		numberOfFramesToOutput(-1),
		typeOfFramesToOutput(RGB),
		width(-1),
		height(-1)
	{}

	bool outputVideo;
	std::string outputDestination;
	long long numberOfFramesToOutput;
	FrameOutputConfig::FrameType typeOfFramesToOutput;
	int width;
	int height;
	std::string pixelFormat;

}; // class FrameOutputConfig
