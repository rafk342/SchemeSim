#pragma once
#include <chrono>


class Timer
{
	std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
	std::chrono::time_point<std::chrono::steady_clock> m_EndTime;

	bool m_IsRunning = false;

public:

	Timer()
	{
		Reset();
	}

	static Timer StartNew()
	{
		Timer result;
		result.Start();
		return result;
	}

	void Start()
	{
		m_StartTime = std::chrono::steady_clock::now();
		m_IsRunning = true;
	}

	void Stop()
	{
		if (!m_IsRunning)
			return;

		m_EndTime = std::chrono::steady_clock::now();
		m_IsRunning = false;
	}

	void Update()
	{
		if (!m_IsRunning)
			return;

		m_EndTime = std::chrono::steady_clock::now();
	}

	void Restart()
	{
		Reset();
		Start();
	}

	void Reset()
	{
		std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();
		m_StartTime = time;
		m_EndTime = time;
	}

	bool GetIsRunning() const { return m_IsRunning; }

	long long GetElapsedTicks() const
	{
		return (m_EndTime - m_StartTime).count();
	}

	double GetElapsedMinutes() const
	{
		return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<60>>>(m_EndTime - m_StartTime).count();
	}

	double GetElapsedSeconds() const
	{
		double duration = std::chrono::duration_cast<std::chrono::duration<double>>(m_EndTime - m_StartTime).count();
		return duration;
	}

	double GetElapsedMilliseconds() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(m_EndTime - m_StartTime).count();
	}

	double GetElapsedMicroseconds() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(m_EndTime - m_StartTime).count();
	}
};
