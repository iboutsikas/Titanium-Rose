#include "hzpch.h"
#include "Hazel/Core/Timer.h"

#include <exception>

namespace Hazel
{

    Timer::Timer()
    {
        LARGE_INTEGER bigboi;
        if (!QueryPerformanceFrequency(&bigboi))
        {
            auto error = GetLastError();
            char errorString[MAX_PATH];
            ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorString, MAX_PATH, nullptr);
            throw std::exception(errorString);
        }

        m_Frequency = bigboi.QuadPart;
        m_FrequencyD = static_cast<double>(m_Frequency);

        if (!QueryPerformanceCounter(&bigboi))
        {
            auto error = GetLastError();
            char errorString[MAX_PATH];
            ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorString, MAX_PATH, nullptr);
            throw std::exception(errorString);
        }

        // We do all of these calculations here, and in update, to avoid repeated divisions in getters

        m_StartTime = bigboi.QuadPart;
        m_TimeElapsed = 0; // This should be 0
        m_TimeElapsedF = static_cast<float>(m_TimeElapsed);
        elapsedSeconds = m_TimeElapsed / m_Frequency;
        elapsedSecondsD = m_TimeElapsed / m_FrequencyD;
        elapsedSecondsF = static_cast<float>(elapsedSecondsD);
        elapsedMilliseconds = static_cast<int64_t>(elapsedSecondsD * 1000);
        elapsedMillisecondsD = elapsedSecondsD * 1000;
        elapsedMillisecondsF = static_cast<float>(elapsedMillisecondsD);
        elapsedMicroseconds = static_cast<int64_t>(elapsedMillisecondsD * 1000);
        elapsedMicrosecondsD = elapsedMillisecondsD * 1000;
        elapsedMicrosecondsF = static_cast<float>(elapsedMillisecondsD);

        m_TimeDelta = 0;
        m_TimeDeltaF = 0;
        deltaMilliseconds = 0;
        deltaMillisecondsF = 0;
        deltaMicroseconds = 0;
        deltaMicrosecondsF = 0;
    }

    Timer::~Timer()
    {

    }

    void Timer::Tick()
    {
        LARGE_INTEGER bigboi;
        if (!QueryPerformanceCounter(&bigboi))
        {
            auto error = GetLastError();
            char errorString[MAX_PATH];
            ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorString, MAX_PATH, nullptr);
            throw std::exception(errorString);
        }

        int64_t currentTime = bigboi.QuadPart - m_StartTime;
        m_TimeDelta = currentTime - m_TimeElapsed;
        m_TimeDeltaF = static_cast<float>(m_TimeDeltaF);
        deltaSeconds = m_TimeDelta / m_Frequency;
        deltaSecondsD = m_TimeDelta / m_FrequencyD;
        deltaSecondsF = static_cast<float>(deltaSecondsD);
        deltaMillisecondsD = deltaSecondsD * 1000;
        deltaMilliseconds = static_cast<int64_t>(deltaMillisecondsD);
        deltaMillisecondsF = static_cast<float>(deltaMillisecondsD);
        deltaMicrosecondsD = deltaMillisecondsD * 1000;
        deltaMicroseconds = static_cast<int64_t>(deltaMicrosecondsD);
        deltaMicrosecondsF = static_cast<float>(deltaMicrosecondsD);

        m_TimeElapsed = currentTime;
        m_TimeElapsedF = static_cast<float>(m_TimeElapsed);
        elapsedSeconds = m_TimeElapsed / m_Frequency;
        elapsedSecondsD = m_TimeElapsed / m_FrequencyD;
        elapsedSecondsF = static_cast<float>(elapsedSecondsD);
        elapsedMilliseconds = static_cast<int64_t>(elapsedSecondsD * 1000);
        elapsedMillisecondsD = elapsedSecondsD * 1000;
        elapsedMillisecondsF = static_cast<float>(elapsedMillisecondsD);
        elapsedMicroseconds = static_cast<int64_t>(elapsedMillisecondsD * 1000);
        elapsedMicrosecondsD = elapsedMillisecondsD * 1000;
        elapsedMicrosecondsF = static_cast<float>(elapsedMillisecondsD);
    }

    int64_t Timer::ElapsedSeconds() const
    {
        return elapsedSeconds;
    }

    float Timer::ElapsedSecondsF() const
    {
        return elapsedSecondsF;
    }

    double Timer::ElapsedSecondsD() const
    {
        return elapsedSecondsD;
    }

    int64_t Timer::DeltaSeconds() const
    {
        return deltaSeconds;
    }

    float Timer::DeltaSecondsF() const
    {
        return deltaSecondsF;
    }

    double Timer::DeltaSecondsD() const
    {
        return deltaSecondsD;
    }

    int64_t Timer::ElapsedMilliseconds() const
    {
        return elapsedMilliseconds;
    }

    float Timer::ElapsedMillisecondsF() const
    {
        return elapsedMillisecondsF;
    }

    double Timer::ElapsedMillisecondsD() const
    {
        return elapsedMillisecondsD;
    }

    int64_t Timer::DeltaMilliseconds() const
    {
        return deltaMilliseconds;
    }

    float Timer::DeltaMillisecondsF() const
    {
        return deltaMillisecondsF;
    }

    double Timer::DeltaMillisecondsD() const
    {
        return deltaMillisecondsD;
    }

    int64_t Timer::ElapsedMicroseconds() const
    {
        return elapsedMicroseconds;
    }

    float Timer::ElapsedMicrosecondsF() const
    {
        return elapsedMicrosecondsF;
    }

    double Timer::ElapsedMicrosecondsD() const
    {
        return elapsedMicrosecondsD;
    }

    int64_t Timer::DeltaMicroseconds() const
    {
        return deltaMicroseconds;
    }

    float Timer::DeltaMicrosecondsF() const
    {
        return deltaMicrosecondsF;
    }

    double Timer::DeltaMicrosecondsD() const
    {
        return deltaMicrosecondsD;
    }

}