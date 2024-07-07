#include "FrameCounter.h"

#include <algorithm>

FrameCounter::FrameCounter()
{
	dataForGraph_.resize(LENGTH_FOR_GRAPH, 0);
}

void FrameCounter::Update(float currentFrame)
{
	deltaSecond_ = currentFrame - lastFrame_;
	lastFrame_ = currentFrame;

	graphTimer_ += deltaSecond_;
	if (graphTimer_ >= GRAPH_DELAY)
	{
		fpsCurr_ = 1.0f / deltaSecond_;
		delayedDeltaMillisecond_ = deltaSecond_ * 1000.f;
		std::ranges::rotate(dataForGraph_.begin(), dataForGraph_.begin() + 1, dataForGraph_.end());
		dataForGraph_[LENGTH_FOR_GRAPH - 1] = fpsCurr_;
		graphTimer_ = 0.f;
	}
}