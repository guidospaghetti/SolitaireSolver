#include "FrameProcessor.H"
#include "FrameOutputConfig.H"

#include <vector>

class FrameOutput : public FrameProcessor {
	public:
	
	FrameOutput(const FrameOutputConfig & outputConfig);
	~FrameOutput();

	virtual void processFrame(libfreenect2::FrameMap & frame);
	virtual bool finishedWithFrame();
	virtual bool finishedProcessing();

	private:

	FILE * setupOutput(libfreenect2::Frame * frame);
	bool writeFrame(libfreenect2::Frame * frame);

	FrameOutputConfig config;
	std::vector<libfreenect2::Frame::Type> requiredFormats;
	libfreenect2::Frame::Type outputFormat;
	FILE * encoder;
	long long framesProcessed;
	bool doneProcessing;

}; // class FrameOutput
