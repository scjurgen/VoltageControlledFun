
#pragma once

#include "Arduino.h"
#include "AudioStream.h"

class AudioNoiseGate : public AudioStream
{
  public:
    AudioNoiseGate(void)
        : AudioStream(1, inputQueueArray)
    {
        setThresholdInDb(m_signalThresholdDb);
    }

    void setThresholdInDb(float db)
    {
        m_signalThresholdDb = db;
        m_threshold         = (int16_t)(32767.0 * powf(10.0f, db / 20.0f));
        Serial.printf("Noisegate theshold: %d\n", m_threshold);
    }

    virtual void update(void) override
    {
        audio_block_t* block = receiveWritable();
        if (!block)
            return;

        bool gateOpen = false;
        for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
        {
            if (block->data[i] > m_threshold)
            {
                gateOpen = true;
                break;
            }
            if (-block->data[i] > m_threshold)
            {
                gateOpen = true;
                break;
            }
        }
        if (!gateOpen)
        {
            for (int i = 0; i < AUDIO_BLOCK_SAMPLES; ++i)
            {
                block->data[i] = 0;
            }
        }
        transmit(block);
        release(block);
    }

  private:
    float          m_signalThresholdDb;
    int16_t        m_threshold;
    audio_block_t* inputQueueArray[1];
};
