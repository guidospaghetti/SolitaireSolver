
#ifndef _SOLITARESOLVER_FRAMEOUTPUT_H_
#define _SOLITARESOLVER_FRAMEOUTPUT_H_

#include "FrameProcessor.H"
#include "FrameOutputConfig.H"

#include <vector>
#include <opencv2/core/mat.hpp>

class FrameOutput : public FrameProcessor {
	public:
	
	FrameOutput(const FrameOutputConfig & outputConfig);
	~FrameOutput();

	virtual void processFrame(std::map<Enums::FrameType, cv::Mat> & frame);
	virtual bool finishedWithFrame();
	virtual bool finishedProcessing();

	private:

	FILE * setupOutput(cv::Mat & frame);
	bool writeFrame(cv::Mat & frame);

	FrameOutputConfig config;
	FILE * encoder;
	long long framesProcessed;
	bool doneProcessing;

}; // class FrameOutput

#endif
