#pragma once
#include <chrono>

class Clock {
using ClockType = std::chrono::high_resolution_clock;
using TimeType = float;

public:
	float get() const {
		auto end = ClockType::now();
		std::chrono::duration<TimeType> dt = end - begin;
		return dt.count();
	}

	void restart() {
		begin = ClockType::now();
	}

private:
	std::chrono::time_point<ClockType> begin = ClockType::now();
};
