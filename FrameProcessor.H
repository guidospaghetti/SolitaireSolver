
#ifndef _SOLITARESOLVER_FRAMEPROCESSOR_H_
#define _SOLITARESOLVER_FRAMEPROCESSOR_H_

#include "Util.H"

#include <map>
#include <opencv2/core/mat.hpp>

class FrameProcessor {
	public:

	virtual void processFrame(std::map<Enums::FrameType, cv::Mat> & frame) = 0;
	virtual bool finishedWithFrame() = 0;
	virtual bool finishedProcessing() = 0;

}; // class FrameProcessor

#endif
