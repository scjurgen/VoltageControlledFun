
#include "Arduino.h"

// Low pass bessel filter order=1 alpha1=0.02118934
// parameters choosen to have efficient coeeficients
class ChFilter
{
  public:
    ChFilter()
    {
        v[0] = 0;
        v[1] = 0;
    }

    short step(short x)
    {
        v[0]     = v[1];
        long tmp = ((((x * 32768L) >> 4) + ((v[0] * 57344L) >> 1)) + 16384) >> 15;
        v[1]     = (short) tmp;
        return (short) (v[0] + v[1]);
    }

  private:
    short v[2];
};

class PotReader
{
  public:
    PotReader(uint16_t pin, bool inverse = false, uint16_t maxValue = 1023)
        : m_pin(pin)
        , m_inverse(inverse)
        , m_newValue(false)
        , m_lastValue(0)
        , m_nextPot(0)
        , c_maxValue(maxValue)
        , m_stretchMin(15)
        , m_stretchMax(1005)
    {
        m_nextPot = millis();
    }

    // caling with a time might be smart, but timing is not critical and
    // everything can be done at idle time in the main loop
    void update(uint32_t msec)
    {
        // if the routing was blocked somehow, time checking is messed up
        if (m_nextPot + 100 < msec)
        {
            m_nextPot = msec;
        }
        if (msec >= m_nextPot)
        {
            m_nextPot += 10; // Lowpass filter is based on 100 Hz samplerate, and ~2Hz responseSpeed
            int32_t inValue = analogRead(m_pin);
            inValue -= m_stretchMin;

            // extend range i.e. real range: 8..1010 to mapped range 0..1023
            inValue = inValue * c_maxValue / (m_stretchMax - m_stretchMin);

            int16_t valuePot = m_filter.step(inValue);
            if (valuePot < 0)
                valuePot = 0;
            if (valuePot > c_maxValue)
                valuePot = c_maxValue;
            if (m_lastValue != valuePot)
            {
                m_newValue  = true;
                m_lastValue = valuePot;
                Serial.printf("new value: %d\n", m_lastValue);
            }
        }
    }

    bool hasNewValue() const
    {
        return m_newValue;
    }

    bool hasNewValueAndResetState()
    {
        bool tmp   = m_newValue;
        m_newValue = false;
        return tmp;
    }

    int clamp(int in)
    {
        if (in < 0)
            return 0;
        if (in > c_maxValue)
            return c_maxValue;
        return in;
    }

    int read()
    {
        if (m_inverse)
            return clamp(c_maxValue - m_lastValue);
        else
            return clamp(m_lastValue);
    }

  private:
    uint16_t m_pin;
    bool     m_inverse;
    bool     m_newValue;
    uint16_t m_lastValue; // std::atomic<int> m_lastValue; // when using in
    // different thread
    uint32_t       m_nextPot;
    const uint16_t c_maxValue;
    uint16_t       m_stretchMin, m_stretchMax;
    ChFilter       m_filter;
};
