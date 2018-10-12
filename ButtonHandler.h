#pragma once
#include "Arduino.h"
/*
 * usage:
 *  ButtonHandler buttonHandler(c_yourInputPinOnBoardToGround);
 buttonHandler.setCb([](unsigned long t, unsigned delta, PressType pressType)
                {
                    switch(pressType)
                    {
                       case PT_Down:
                        Serial.printf("%t=ld Button pressed down", t);
                        break;
                       case PT_Release:
                        Serial.printf("%t=ld Button released, delta to t down: %ld", t, delta);
                        break;
                       case PT_LongRelease:
                        Serial.printf("%t=ld Button long release", t);
                        break;
                       case PT_Repeat:
                        Serial.printf("%t=ld Button repeated auto press", t);
                        break;
                    }
                });
 */

class RepeatHandler
{
  public:
    RepeatHandler(int tLong = 1200, int firstPhaseT = 300, int firstPhaseCount = 3, int secondPhaseT = 150,
                  int secondPhaseCount = 10, int lastPhaseT = 50)
        : m_longPress(tLong)
        , m_firstPhaseT(firstPhaseT)
        , m_firstPhaseCount(firstPhaseCount)
        , m_secondPhaseT(secondPhaseT)
        , m_secondPhaseCount(secondPhaseCount)
        , m_lastPhaseT(lastPhaseT)
    {
    }

    void reset(unsigned long t)
    {
        m_startT = t;
        m_phase  = 0;
    }

    bool updateEvent(unsigned long t)
    {
        m_lastT    = t;
        long delta = m_lastT - m_startT;
        switch (m_phase)
        {
            case 0:
                if (delta >= m_longPress)
                {
                    m_phase  = 1;
                    m_count  = 0;
                    m_startT = m_lastT;
                    return true;
                }
                break;
            case 1:
                if (delta >= m_firstPhaseT)
                {
                    if (++m_count >= m_firstPhaseCount)
                    {
                        m_phase = 2;
                        m_count = 0;
                    }
                    m_startT = m_lastT;
                    return true;
                }
                break;
            case 2:
                if (delta >= m_secondPhaseT)
                {
                    if (++m_count >= m_firstPhaseCount)
                    {
                        m_phase = 3;
                        m_count = 0;
                    }
                    m_startT = m_lastT;
                    return true;
                }
                break;
            case 3:
                if (delta >= m_lastPhaseT)
                {
                    m_startT = m_lastT;
                    return true;
                }
                break;
        }
        return false;
    }

    bool newEvent(int& phase)
    {
        if (m_newEvent)
        {
            phase = m_phase;
        }
        return m_newEvent;
    }

    int phase() const
    {
        return m_phase;
    }

  private:
    bool          m_newEvent = false;
    unsigned long m_lastT    = 0;
    unsigned long m_startT   = 0;
    int           m_phase    = 0;

    int m_count = 0;
    int m_longPress;
    int m_firstPhaseT;
    int m_firstPhaseCount;
    int m_secondPhaseT;
    int m_secondPhaseCount;
    int m_lastPhaseT;
};

class ButtonHandler
{
  public:
    typedef enum
    {
        PT_Down,
        PT_Release,
        PT_LongRelease,
        PT_Repeat
    } PressType;

    ButtonHandler(int pin)
        : m_pinIn(pin)
        , m_cb(nullptr)

    {
    }

    void update(unsigned long currentTMsecs)
    {
        int newValue = digitalRead(m_pinIn);
        m_delta      = 0;
        if (newValue != m_oldValue)
        {
            if (newValue == 0)
            {
                m_repeat.reset(m_startMs = currentTMsecs);
                m_oldValue = 0;
                m_tOn      = 0;
                if (m_startMs > m_lastDownEvent + 40)
                {
                    m_lastDownEvent = m_startMs;
                    if (m_cb)
                        m_cb(currentTMsecs, m_delta, PT_Down);
                }
            }
            else
            {
                m_delta    = currentTMsecs - m_startMs;
                m_oldValue = 1;
                if (m_delta > 50)
                {
                    m_tOn           = m_startMs;
                    m_buttonPressed = true;
                    m_lastDownEvent = currentTMsecs;
                    if (m_cb)
                    {
                        if (m_repeat.phase())
                            m_cb(currentTMsecs, m_delta, PT_LongRelease);
                        else
                            m_cb(currentTMsecs, m_delta, PT_Release);
                    }
                }
            }
        }
        else if (newValue == 0)
        {
            if (m_repeat.updateEvent(currentTMsecs))
            {
                m_buttonPressed = true;
                if (m_cb)
                {
                    m_delta         = currentTMsecs - m_startMs;
                    m_lastDownEvent = currentTMsecs;
                    m_cb(currentTMsecs, m_delta, PT_Repeat);
                }
            }
        }
    }

  private:
    RepeatHandler m_repeat;
    unsigned long m_startMs       = 0;
    unsigned long m_delta         = 0;
    unsigned long m_t             = 0;
    unsigned long m_tOn           = 0; // unused
    unsigned long m_lastDownEvent = 0;
    int           m_pinIn;
    int           m_oldValue      = 1;
    bool          m_buttonPressed = false;

  public:
    void setCb(void (*cb)(unsigned long t, unsigned long delta, PressType PressType))
    {
        m_cb = cb;
    }

  private:
    void (*m_cb)(unsigned long t, unsigned long delta, PressType PressType);
};
