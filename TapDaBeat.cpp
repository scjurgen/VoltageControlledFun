#include "TapDaBeat.h"
#include "Arduino.h"

TapDaBeat::TapDaBeat()
    : m_lastTime(0)
    , m_lastBpm(120.0)
{
}

float TapDaBeat::bpm() const
{
    return m_lastBpm;
}

void TapDaBeat::tap(unsigned long t)
{
    if (m_lastTime == 0)
    {
        m_tapCount               = 0;
        m_tapBuffer[MAXTAPS - 1] = t;
        m_lastTime               = t;
    }
    else
    {
        unsigned long delta = t - m_lastTime;
        if (delta < 20)
            return;
        if (delta > 2000)
        {
            m_tapCount               = 0;
            m_tapBuffer[MAXTAPS - 1] = t;
            m_lastTime               = t;
        }
        else
        {
            ++m_tapCount;
            for (auto i = 0; i < MAXTAPS - 1; ++i)
            {
                m_tapBuffer[i] = m_tapBuffer[i + 1];
            }
            m_tapBuffer[MAXTAPS - 1] = t;
            if (m_tapCount > MAXTAPS - 1)
            {
                m_tapCount = MAXTAPS - 1;
            }
            if (m_tapCount)
            {
                unsigned long deltaBuffer = m_tapBuffer[MAXTAPS - 1] - m_tapBuffer[MAXTAPS - 1 - m_tapCount];
                auto          bpm         = 60000.0 / (float(deltaBuffer) / float(m_tapCount));
                m_lastBpm                 = bpm;
            }
            m_lastTime = t;
        }
    }
}
