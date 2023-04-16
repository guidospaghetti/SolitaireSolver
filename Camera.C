// Retrieves images from the kinect

#include "Util.H"
#include "KinectReader.H"
#include "FrameOutput.H"

#include <cstdlib>
#include <unistd.h>
#include <memory>
#include <signal.h>
#include <atomic>
#include <chrono>
#include <list>

#include <opencv2/core/mat.hpp>

std::atomic<bool> shutdown(false);

void signalHandler(int s)
{
	shutdown = true;
}

void registerSignals()
{
	// Register for SIGINT, aka CTRL+C

	struct sigaction sa;
	sa.sa_handler = signalHandler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char* argv[])
{

	int numFramesToRead = -1;
	bool outputFrames = false;
	bool outputPNGs = false;
	std::string outputDestination = "192.168.1.157:8044";

	int c = 0;
	while ((c = getopt(argc, argv, "d:n:op")) != EOF)
	{
		switch (c) {
			case 'd':
				outputDestination = optarg;
				break;
			case 'n':
				numFramesToRead = atoi(optarg);
				break;
			case 'o':
				outputFrames = true;
				LOG_OUT("Will be outputting frames");
				break;
			case 'p':
				outputPNGs = true;
			default:
				break;
		}
	}

	if (numFramesToRead > 0) {
		LOG_OUT("Will only read %d frames", numFramesToRead);
	} else {
		LOG_OUT("Will read frames until interrupted");
	}

	if (outputFrames) {
		LOG_OUT("Will be outputting %s to %s", 
				outputPNGs ? "PNGs" : "video",
				outputDestination.c_str());
	}
	
	registerSignals();

	KinectReader reader(Enums::FrameType::RGB);
	if (!reader.setupKinect()) {
		LOG_OUT("Failed to setup kinect");
		return -1;
	}

	if (!reader.start()) {
		LOG_OUT("Failed to start kinect device");
		return -1;
	}

	int numFrames = 0;

	std::list<std::unique_ptr<FrameProcessor> > processors;
	if (outputFrames)
	{
		FrameOutputConfig outputConfig;
		outputConfig.outputVideo = !outputPNGs;
		outputConfig.outputDestination = outputDestination;
		outputConfig.numberOfFramesToOutput = numFramesToRead;
		outputConfig.typeOfFramesToOutput = Enums::FrameType::RGB;
		processors.push_back(std::unique_ptr<FrameProcessor>(
					new FrameOutput(outputConfig)));
	}
	const auto start = std::chrono::steady_clock::now();
	while (shutdown == false)
	{
		std::map<Enums::FrameType, cv::Mat> & frame = reader.getFrame(true);

		// do something;

		for (auto & processor : processors) {
			processor->processFrame(frame);
		}

		while (true) {
			bool allDone = true;
			for (auto & processor : processors) {
				allDone &= processor->finishedWithFrame();
			}
			if (allDone) {
				break;
			}
		}
		reader.releaseFrames();

		bool allFinishedProcessing = true;
		for (auto & processor : processors) {
			allFinishedProcessing &= processor->finishedProcessing();
		}
		if (allFinishedProcessing) {
			LOG_OUT("All frame processors are done processing forever, stopping");
			shutdown = true;
		}

		numFrames++;

		if (numFramesToRead > 0) {
			if (numFrames >= numFramesToRead) {
				LOG_OUT("Read %d frames, stopping", numFrames);
				shutdown = true;
			}
		}
	}
	const auto end = std::chrono::steady_clock::now();
	std::chrono::duration<double> readTime = end - start;

	LOG_OUT("Shutting down, Read %d frames in %.2f sec -> %.2f fps",
			numFrames, readTime.count(), numFrames / readTime.count());

	processors.clear();

	reader.stop();

	LOG_OUT("Done shutting down");
	return 0;

}
