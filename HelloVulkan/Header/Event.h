#ifndef HELLO_VULKAN_EVENT
#define HELLO_VULKAN_EVENT

#include <vector>
#include <functional>

/*
stackoverflow.com/q/7582546
*/

template<class... T>
class Event
{
private:
	std::vector<std::function<void(T...)>> listeners_;

public:
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