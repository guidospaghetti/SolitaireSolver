
#include "libfreenect2/frame_listener_impl.h"

class FrameProcessor {
	public:

	virtual void processFrame(libfreenect2::FrameMap & frame) = 0;
	virtual bool finishedWithFrame() = 0;
	virtual bool finishedProcessing() = 0;

}; // class FrameOutput
