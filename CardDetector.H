
#ifndef _SOLITARESOLVER_CARDDETECTOR_H_
#define _SOLITARESOLVER_CARDDETECTOR_H_

#include "FrameProcessor.H"
#include "CardDetectorConfig.H"

#include <opencv2/dnn.hpp>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>

class CardDetector : public FrameProcessor {
	public:

	CardDetector(const CardDetectorConfig & config);
	~CardDetector();

	virtual void processFrame(std::map<Enums::FrameType, cv::Mat> & frame);
	virtual bool finishedWithFrame();
	virtual bool finishedProcessing();

	private:

	struct Detection{
		int classId;
		float confidence;
		cv::Rect box;
	};

	// Executed in the detection thread
	void getDetections();

	void preprocess(const cv::Mat & frame, cv::dnn::Net & net, 
			cv::Size inputSize, float scale, const cv::Scalar & mean,
			bool swapRB);

	void postprocess(cv::Mat & frame, const std::vector<cv::Mat> & outputs,
			cv::dnn::Net & net, int backend,
			std::vector<Detection> & detections);

	void drawPred(int classId, float conf, int left, int top,
			int right, int bottom, cv::Mat & frame);

	CardDetectorConfig config;
	cv::dnn::Net net;
	std::vector<std::string> classes;
	std::vector<std::string> outputLayerNames;

	std::thread detectionThread;

	std::queue<cv::Mat> frameQueue;
	std::mutex frameQueueMutex;

	std::queue<Detection> detectionQueue;
	std::mutex detectionQueueMutex;

	std::atomic<bool> process;

}; // class CardDetector

#endif
