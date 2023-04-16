
#ifndef _SOLITARESOLVER_CARDDETECTORCONFIG_H_
#define _SOLITARESOLVER_CARDDETECTORCONFIG_H_

class CardDetectorConfig {
	public:

	CardDetectorConfig() :
		inputSize(640),
		scale(0.00392),
		confThreshold(0.5f),
		nmsThreshold(0.4f)
	{}
	
	std::string modelPath;
	std::string configPath;
	std::string classesPath;
	int inputSize;
	float scale;
	float confThreshold;
	float nmsThreshold;

	// I'm sure I'll have others...

}; // class CardDetectorConfig

#endif
