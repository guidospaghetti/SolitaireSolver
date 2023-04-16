
#include "CardDetector.H"

#include <opencv2/imgproc.hpp>
#include <fstream>

CardDetector::CardDetector(const CardDetectorConfig & config) :
	config(config)
{
	LOG_OUT("Reading in model (%s), config (%s), classes (%s)",
			config.modelPath.c_str(), config.configPath.c_str(),
			config.classesPath.c_str());
	try {
		net = cv::dnn::readNet(config.modelPath, config.configPath);
	} catch (cv::Exception ee) {
		LOG_OUT("Hit exception while trying to read in model");
		return;
	}
	net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
	net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

	outputLayerNames = net.getUnconnectedOutLayersNames();
	LOG_OUT("Network has following output layers");
	for (const std::string & name : outputLayerNames) {
		LOG_OUT("\t%s", name.c_str());
	}

	std::vector<int> outputLayers = net.getUnconnectedOutLayers(); 
	LOG_OUT("Network has following output layer types");
	for (const int layer : outputLayers) {
		LOG_OUT("\t%s", net.getLayer(layer)->type.c_str());
	}

	if (config.classesPath.empty() == false) {
		std::ifstream ifs(config.classesPath.c_str());
		if (ifs.is_open()) 
		{
			std::string line;
			while (std::getline(ifs, line)) {
				classes.push_back(line);
			}
			LOG_OUT("Read %lu classes", classes.size());
		}
		else
		{
			LOG_OUT("Failed to open classes file %s, continuing",
					config.classesPath.c_str());
		}
	}

	process = true;

	detectionThread = std::thread(&CardDetector::getDetections, this);

}

CardDetector::~CardDetector()
{
	process = false;
	detectionThread.join();
}

void CardDetector::processFrame(std::map<Enums::FrameType, cv::Mat> & frame)
{

	if (frame.find(Enums::FrameType::RGB) == frame.end()) {
		return;
	}

	cv::Mat rgb = frame[Enums::FrameType::RGB];

	static const cv::Size inputSize(config.inputSize, config.inputSize);
	// Take a 640x640 square in the center of the frame
	const int centerX = rgb.cols / 2;
	const int centerY = rgb.rows / 2;
	cv::Rect centerSquare(
			centerX - inputSize.width / 2,
			centerY - inputSize.height / 2,
			inputSize.width,
			inputSize.height);

	cv::Mat croppedTmp = rgb(centerSquare);
	cv::Mat cropped;
	croppedTmp.copyTo(cropped);

	bool addedFrame = false;
	{
		std::lock_guard<std::mutex> lock(frameQueueMutex);
		if (frameQueue.empty()) {
			frameQueue.push(cropped);
			addedFrame = true;
		}
	}

	static std::vector<Detection> detections;
	if (addedFrame) {
		detections.clear();
	}

	{
		std::lock_guard<std::mutex> lock(detectionQueueMutex);
		while (detectionQueue.empty() == false) {
			detections.push_back(detectionQueue.front());
			detectionQueue.pop();
		}
	}

	for (const Detection & detection : detections) {
		const cv::Rect & box = detection.box;
		drawPred(detection.classId, detection.confidence,
				box.x, box.y, box.x + box.width, box.y + box.height,
				cropped);
	}

	frame[Enums::FrameType::RGB] = cropped;

}

bool CardDetector::finishedWithFrame()
{
	return true;
}

bool CardDetector::finishedProcessing()
{
	return false;
}

void CardDetector::getDetections()
{
	while (process)
	{
		cv::Mat frame;
		{
			std::lock_guard<std::mutex> lock(frameQueueMutex);
			if (frameQueue.empty() == false) {
				frameQueue.front().copyTo(frame);
			}
		}
		if (frame.empty()) {
			continue;
		}

		static const cv::Size inputSize(config.inputSize, config.inputSize);

		preprocess(frame, net, inputSize, config.scale, 0, true);

		std::vector<cv::Mat> outputs;
		net.forward(outputs, outputLayerNames);

		std::vector<Detection> detections;
		postprocess(frame, outputs, net, cv::dnn::DNN_BACKEND_CUDA,
				detections);

		{
			std::lock_guard<std::mutex> lock(detectionQueueMutex);
			for (const Detection & detection : detections)
			{
				detectionQueue.push(detection);
			}
		}

		{
			std::lock_guard<std::mutex> lock(frameQueueMutex);
			frameQueue.pop();
		}
	}
}

// MOSTLY COPIED FROM OPENCV OBJECT DETECTION EXAMPLE
// https://github.com/opencv/opencv/blob/3.4/samples/dnn/object_detection.cpp
////////////////////////////////////////////////////

void CardDetector::preprocess(
		const cv::Mat& frame, 
		cv::dnn::Net& net, 
		cv::Size inpSize, 
		float scale,
		const cv::Scalar& mean,
		bool swapRB)
{
	static cv::Mat blob;
	// Create a 4D blob from a frame.
	if (inpSize.width <= 0) inpSize.width = frame.cols;
	if (inpSize.height <= 0) inpSize.height = frame.rows;
	cv::dnn::blobFromImage(frame, blob, 1.0, inpSize, cv::Scalar(), 
			swapRB, false, CV_8U);

	// Run a model.
	net.setInput(blob, "", scale, mean);
	//if (net.getLayer(0)->outputNameToIndex("im_info") != -1)  // Faster-RCNN or R-FCN
	//{
	//	cv::resize(frame, frame, inpSize);
	//	cv::Mat imInfo = (Mat_<float>(1, 3) << inpSize.height, inpSize.width, 1.6f);
	//	net.setInput(imInfo, "im_info");
	//}
}

void CardDetector::postprocess(cv::Mat& frame, 
		const std::vector<cv::Mat>& outs, cv::dnn::Net& net, int backend,
		std::vector<Detection> & detections)
{
	static std::vector<int> outLayers = net.getUnconnectedOutLayers();
	static std::string outLayerType = net.getLayer(outLayers[0])->type;

	std::vector<int> classIds;
	std::vector<float> confidences;
	std::vector<cv::Rect> boxes;
	if (outLayerType == "DetectionOutput")
	{
		// Network produces output blob with a shape 1x1xNx7 where N is a number of
		// detections and an every detection is a vector of values
		// [batchId, classId, confidence, left, top, right, bottom]
		if (outs.empty()) {
			LOG_OUT("Didn't get ANY outputs?");
			return;
		}
		for (size_t k = 0; k < outs.size(); k++)
		{
			float* data = (float*)outs[k].data;
			for (size_t i = 0; i < outs[k].total(); i += 7)
			{
				float confidence = data[i + 2];
				if (confidence > config.confThreshold)
				{
					int left   = (int)data[i + 3];
					int top	= (int)data[i + 4];
					int right  = (int)data[i + 5];
					int bottom = (int)data[i + 6];
					int width  = right - left + 1;
					int height = bottom - top + 1;
					if (width <= 2 || height <= 2)
					{
						left   = (int)(data[i + 3] * frame.cols);
						top	= (int)(data[i + 4] * frame.rows);
						right  = (int)(data[i + 5] * frame.cols);
						bottom = (int)(data[i + 6] * frame.rows);
						width  = right - left + 1;
						height = bottom - top + 1;
					}
					classIds.push_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
					boxes.push_back(cv::Rect(left, top, width, height));
					confidences.push_back(confidence);
				}
			}
		}
	}
	else if (outLayerType == "Region")
	{
		for (size_t i = 0; i < outs.size(); ++i)
		{
			// Network produces output blob with a shape NxC where N is a number of
			// detected objects and C is a number of classes + 4 where the first 4
			// numbers are [center_x, center_y, width, height]
			float* data = (float*)outs[i].data;
			for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols)
			{
				cv::Mat scores = outs[i].row(j).colRange(5, outs[i].cols);
				cv::Point classIdPoint;
				double confidence;
				cv::minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
				if (confidence > config.confThreshold)
				{
					int centerX = (int)(data[0] * frame.cols);
					int centerY = (int)(data[1] * frame.rows);
					int width = (int)(data[2] * frame.cols);
					int height = (int)(data[3] * frame.rows);
					int left = centerX - width / 2;
					int top = centerY - height / 2;

					classIds.push_back(classIdPoint.x);
					confidences.push_back((float)confidence);
					boxes.push_back(cv::Rect(left, top, width, height));
				}
			}
		}
	}
	else
	{
		LOG_OUT("Unknown output layer type: %s", outLayerType.c_str());
		return;
	}

	// NMS is used inside Region layer only on DNN_BACKEND_OPENCV 
	// for another backends we need NMS in sample
	// or NMS is required if number of outputs > 1
	if (outLayers.size() > 1 || 
		(outLayerType == "Region" && backend != cv::dnn::DNN_BACKEND_OPENCV))
	{
		std::map<int, std::vector<size_t> > class2indices;
		for (size_t i = 0; i < classIds.size(); i++)
		{
			if (confidences[i] >= config.confThreshold)
			{
				class2indices[classIds[i]].push_back(i);
			}
		}
		std::vector<cv::Rect> nmsBoxes;
		std::vector<float> nmsConfidences;
		std::vector<int> nmsClassIds;
		for (std::map<int, std::vector<size_t> >::iterator it = class2indices.begin(); 
				it != class2indices.end(); ++it)
		{
			std::vector<cv::Rect> localBoxes;
			std::vector<float> localConfidences;
			std::vector<size_t> classIndices = it->second;
			for (size_t i = 0; i < classIndices.size(); i++)
			{
				localBoxes.push_back(boxes[classIndices[i]]);
				localConfidences.push_back(confidences[classIndices[i]]);
			}
			std::vector<int> nmsIndices;
			cv::dnn::NMSBoxes(localBoxes, localConfidences, 
					config.confThreshold, config.nmsThreshold, nmsIndices);
			for (size_t i = 0; i < nmsIndices.size(); i++)
			{
				size_t idx = nmsIndices[i];
				nmsBoxes.push_back(localBoxes[idx]);
				nmsConfidences.push_back(localConfidences[idx]);
				nmsClassIds.push_back(it->first);
			}
		}
		boxes = nmsBoxes;
		classIds = nmsClassIds;
		confidences = nmsConfidences;
	}

	for (size_t idx = 0; idx < boxes.size(); ++idx)
	{
		Detection detection;
		detection.classId = classIds[idx];
		detection.confidence = confidences[idx];
		detection.box = boxes[idx];
		detections.push_back(detection);
		//cv::Rect box = boxes[idx];
		//drawPred(classIds[idx], confidences[idx], box.x, box.y,
		//		 box.x + box.width, box.y + box.height, frame);
	}
}

void CardDetector::drawPred(int classId, float conf, 
		int left, int top, int right, int bottom, cv::Mat & frame)
{
	cv::rectangle(frame, cv::Point(left, top), cv::Point(right, bottom), 
			cv::Scalar(0, 255, 0));

    std::string label = cv::format("%.2f", conf);
    if (!classes.empty())
    {
		if (classId >= (int)classes.size()) {
			LOG_OUT("Got classId %d greater than number of classes",
					classId);
		} else {
			label = classes[classId] + ": " + label;
		}
    }

    int baseLine;
	cv::Size labelSize = cv::getTextSize(
			label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = std::max(top, labelSize.height);
	cv::rectangle(frame, cv::Point(left, top - labelSize.height),
              cv::Point(left + labelSize.width, top + baseLine), 
			  cv::Scalar::all(255), cv::FILLED);

	cv::putText(frame, label, cv::Point(left, top), cv::FONT_HERSHEY_SIMPLEX, 
			0.5, cv::Scalar());
}

// END COPIED CODE
////////////////////////////////////////
