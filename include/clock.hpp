#pragma once
#include <chrono>

class Clock {
using ClockType = std::chrono::high_resolution_clock;
using TimeType = float;

public:
	float getSeconds() const {
		auto end = ClockType::now();
		std::chrono::duration<TimeType> dt = end - begin;
		return dt.count();
	}

	float getMilliSeconds() const {
		return getSeconds() * 1000.f;
	}

	float getNanoSeconds() const {
		return getMilliSeconds() * 1000.f;
	}

	void restart() {
		begin = ClockType::now();
	}

private:
	std::chrono::time_point<ClockType> begin = ClockType::now();
};
