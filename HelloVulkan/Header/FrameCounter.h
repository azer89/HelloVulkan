#ifndef FRAME_COUNTER
#define FRAME_COUNTER

#include <vector>

class FrameCounter
{
private:
	float fpsCurr_;
	float delayedDeltaMillisecond_;
	float deltaSecond_; // Time between current frame and last frame
	float lastFrame_;

	std::vector<float> dataForGraph_;
	float graphTimer_;

	static constexpr size_t LENGTH_FOR_GRAPH = 100;
	static constexpr float GRAPH_DELAY = 0.1f;

public:
	[[nodiscard]] float GetDeltaSecond() const { return deltaSecond_; }
	[[nodiscard]] float GetDelayedDeltaMillisecond() const { return delayedDeltaMillisecond_; }
	[[nodiscard]] float GetCurrentFPS() const { return fpsCurr_; }
	[[nodiscard]] const float* GetGraph() const { return dataForGraph_.data(); }
	static int GetGraphLength() { return LENGTH_FOR_GRAPH; }
	FrameCounter();
	void Update(float currentFrame);
};

#endif