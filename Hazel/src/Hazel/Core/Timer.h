#pragma once
#include <cstdint>

namespace Hazel
{
    class Timer
    {
    public:
        Timer();
        ~Timer();

        void Tick();

        int64_t ElapsedSeconds() const;
        float ElapsedSecondsF() const;
        double ElapsedSecondsD() const;
        int64_t DeltaSeconds() const;
        float DeltaSecondsF() const;
        double DeltaSecondsD() const;

        int64_t ElapsedMilliseconds() const;
        float ElapsedMillisecondsF() const;
        double ElapsedMillisecondsD() const;
        int64_t DeltaMilliseconds() const;
        float DeltaMillisecondsF() const;
        double DeltaMillisecondsD() const;

        int64_t ElapsedMicroseconds() const;
        float ElapsedMicrosecondsF() const;
        double ElapsedMicrosecondsD() const;
        int64_t DeltaMicroseconds() const;
        float DeltaMicrosecondsF() const;
        double DeltaMicrosecondsD() const;

    private:

        int64_t m_StartTime;

        int64_t m_Frequency;
        double m_FrequencyD;

        int64_t m_TimeElapsed;
        int64_t m_TimeDelta;

        float m_TimeElapsedF;
        float m_TimeDeltaF;

        double elapsedD;
        double deltaD;

        int64_t elapsedSeconds;
        int64_t deltaSeconds;

        float elapsedSecondsF;
        float deltaSecondsF;

        double elapsedSecondsD;
        double deltaSecondsD;

        int64_t elapsedMilliseconds;
        int64_t deltaMilliseconds;

        float elapsedMillisecondsF;
        float deltaMillisecondsF;

        double elapsedMillisecondsD;
        double deltaMillisecondsD;

        int64_t elapsedMicroseconds;
        int64_t deltaMicroseconds;

        float elapsedMicrosecondsF;
        float deltaMicrosecondsF;

        double elapsedMicrosecondsD;
        double deltaMicrosecondsD;
    };
}