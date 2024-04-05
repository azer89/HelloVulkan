#ifndef HELLO_VULKAN_EVENT
#define HELLO_VULKAN_EVENT

#include <vector>
#include <functional>

template<class... T>
class Event
{
private:
	std::vector<std::function<void(T...)>> listeners_ = {};

public:
	// You can use std::bind or lambdas
	void AddListener(std::function<void(T...)> f)
	{
		listeners_.push_back(f);
	}

	// TODO Implement RemoveListener()
	// stackoverflow.com/q/60212046

	void Invoke(T... t)
	{
		for (auto& f : listeners_)
		{
			f(t...);
		}
	}
};

#endif