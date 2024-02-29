#ifndef FRAME_COUNTER
#define FRAME_COUNTER

#include <vector>
#include <numeric>
#include <algorithm>

class FrameCounter
{
private:
	//float avgFps_;
	float fpsCurr_;

	// Miliseconds
	float delayedDeltaTime_;
	// Seconds
	float deltaTime_; // Time between current frame and last frame
	float lastFrame_;

	//const size_t LENGTH_FOR_AVG = 10; // For smoothing
	//std::vector<float> dataForAvg_;
	//size_t avgCounter_;

	static constexpr size_t LENGTH_FOR_GRAPH = 100;
	static constexpr float GRAPH_DELAY = 0.2f;
	std::vector<float> dataForGraph_;
	float graphTimer_;

public:
	FrameCounter() :
		//avgFps_(0.f),
		fpsCurr_(0.f),
		delayedDeltaTime_(0.f),
		deltaTime_(0.f),
		lastFrame_(0.f),
		//avgCounter_(0),
		graphTimer_(0.f)
	{
		//dataForAvg_.resize(LENGTH_FOR_AVG, 0);
		dataForGraph_.resize(LENGTH_FOR_GRAPH, 0);
	}

	float GetDeltaTime() const { return deltaTime_; }
	float GetDelayedDeltaTime() const { return delayedDeltaTime_; }
	float GetCurrentFPS() const { return fpsCurr_; }

	const float* GetGraph() const { return dataForGraph_.data(); };
	int GetGraphLength() const { return LENGTH_FOR_GRAPH; }

	// IMGUI_API void PlotLines(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float));
	//const float* GetData() { return data_.data(); }

	void Update(float currentFrame)
	{
		deltaTime_ = currentFrame - lastFrame_;
		lastFrame_ = currentFrame;
		//dataForAvg_[avgCounter_] = deltaTime_;
		//avgCounter_ = (avgCounter_ + 1) % LENGTH_FOR_AVG;

		graphTimer_ += deltaTime_;
		if (graphTimer_ >= GRAPH_DELAY)
		{
			//avgFps_ = std::reduce(dataForAvg_.begin(), dataForAvg_.end()) / static_cast<float>(LENGTH_FOR_AVG);
			fpsCurr_ = 1.0f / deltaTime_;
			delayedDeltaTime_ = deltaTime_ * 1000.f;
			std::rotate(dataForGraph_.begin(), dataForGraph_.begin() + 1, dataForGraph_.end());
			dataForGraph_[LENGTH_FOR_GRAPH - 1] = fpsCurr_;
			graphTimer_ = 0.f;
		}
	}
};

#endif