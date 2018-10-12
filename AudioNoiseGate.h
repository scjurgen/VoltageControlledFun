
#pragma once

#include "Arduino.h"
#include "AudioStream.h"

class AudioNoiseGate : public AudioStream
{
  public:
    AudioNoiseGate(void);

    void setThresholdInDb(float db);

    virtual void update(void) override;

  private:
    float          m_signalThresholdDb;
    int16_t        m_threshold;
    audio_block_t* inputQueueArray[1];
};
