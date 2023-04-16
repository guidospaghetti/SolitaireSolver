
#ifndef _SOLITARESOLVER_FRAMEOUTPUTCONFIG_H_
#define _SOLITARESOLVER_FRAMEOUTPUTCONFIG_H_

#include "Util.H"
#include <string>

class FrameOutputConfig {
	public:

	FrameOutputConfig() :
		outputVideo(true),
		numberOfFramesToOutput(-1),
		typeOfFramesToOutput(Enums::FrameType::RGB),
		width(-1),
		height(-1)
	{}

	bool outputVideo;
	std::string outputDestination;
	long long numberOfFramesToOutput;
	Enums::FrameType typeOfFramesToOutput;
	int width;
	int height;
	std::string pixelFormat;

}; // class FrameOutputConfig

#endif
