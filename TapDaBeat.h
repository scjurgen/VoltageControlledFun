#pragma once

#define MAXTAPS 5

class TapDaBeat
{
  public:
    TapDaBeat();

    float bpm() const;

    void tap(unsigned long t);

  private:
    unsigned long m_lastTime;
    unsigned long m_tapCount;
    unsigned long m_tapBuffer[MAXTAPS];
    float         m_lastBpm;
};
