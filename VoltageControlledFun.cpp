
#include "Arduino.h"
#include "PotReader.h"
#include "TapDaBeat.h"
#include "AudioNoiseGate.h"

#include <Audio.h>
#include <Bounce2.h>
#include <SD.h>
#include <SPI.h>
#include <SerialFlash.h>
#include <Wire.h>
#include <EEPROM.h>

AudioInputI2S            i2sIn;
AudioSynthWaveformDc     dcStage1;
AudioSynthWaveformDc     dcStage2;
AudioFilterBiquad        inputFilter;
AudioNoiseGate           noiseGate;
AudioFilterStateVariable filterLeftStage1;
AudioFilterStateVariable filterLeftStage2;
AudioFilterStateVariable filterRightStage1;
AudioFilterStateVariable filterRightStage2;

AudioEffectFade fadeLeftStage1;
AudioEffectFade fadeLeftStage2;
AudioEffectFade fadeRightStage1;
AudioEffectFade fadeRightStage2;

AudioEffectDelay slapBackDelay;

AudioMixer4 mixerFilters;
AudioMixer4 mixerFinal;

AudioOutputI2S   i2sOut;
AudioAnalyzePeak peakIn;

AudioConnection patchCordPeak(i2sIn, 0, peakIn, 0);
AudioConnection patchInputFilter(i2sIn, 0, inputFilter, 0);
AudioConnection patchNoiseGate(inputFilter, 0, noiseGate, 0);
AudioConnection patchCord2(noiseGate, 0, filterRightStage1, 0);
AudioConnection patchCord3(noiseGate, 0, filterRightStage2, 0);

AudioConnection patchCordSlapBack1(noiseGate, 0, slapBackDelay, 0);
AudioConnection patchCordSlapback2(slapBackDelay, 0, mixerFinal, 2);
AudioConnection patchCordSlapback3(slapBackDelay, 1, mixerFinal, 3);

AudioConnection patchCord6(dcStage1, 0, filterLeftStage1, 1);
AudioConnection patchCord7(dcStage1, 0, filterRightStage1, 1);
AudioConnection patchCord8(dcStage2, 0, filterLeftStage2, 1);
AudioConnection patchCord9(dcStage2, 0, filterRightStage2, 1);
AudioConnection patchCord10(noiseGate, 0, filterLeftStage1, 0);
AudioConnection patchCord11(noiseGate, 0, filterLeftStage2, 0);
AudioConnection patchCord12(filterLeftStage1, 1, fadeLeftStage1, 0);
AudioConnection patchCord15(filterRightStage1, 1, fadeRightStage1, 0);
AudioConnection patchCord14(filterLeftStage2, 1, fadeLeftStage2, 0);
AudioConnection patchCord13(filterRightStage2, 1, fadeRightStage2, 0);
AudioConnection patchCord16(fadeLeftStage1, 0, mixerFilters, 0);
AudioConnection patchCord17(fadeLeftStage2, 0, mixerFilters, 1);
AudioConnection patchCord18(fadeRightStage1, 0, mixerFilters, 2);
AudioConnection patchCord19(fadeRightStage2, 0, mixerFilters, 3);
AudioConnection patchCord22(mixerFilters, 0, mixerFinal, 0);
AudioConnection patchCord23(mixerFilters, 0, mixerFinal, 1);
AudioConnection patchCord20(mixerFinal, 0, i2sOut, 0);
AudioConnection patchCord21(mixerFinal, 0, i2sOut, 1);

AudioControlSGTL5000 sgtl5000_1;

int       pinRGBLed[3]   = {3, 5, 4};
int       potPins[4]     = {6, 2, 7, 3};
const int c_centerButton = 2;
Bounce    centerButton;

int lineInLevel  = 13;
int lineOutLevel = 30;

bool  isFlipping = false;
float mix        = 1.0;
float delayGain  = 0.55;

void setMix()
{
    if (isFlipping)
    {
        float fMixEffect;
        float fMixPure;
        fMixEffect = mix;
        fMixPure   = 1.0 - mix;
        mixerFilters.gain(0, fMixEffect);
        mixerFilters.gain(1, fMixEffect);
        mixerFilters.gain(2, fMixEffect);
        mixerFilters.gain(3, fMixEffect);
        mixerFinal.gain(0, 1.0);
        mixerFinal.gain(1, 1.0);
        mixerFinal.gain(2, fMixPure);
        mixerFinal.gain(3, fMixPure * delayGain);
    }
    else
    {
        mixerFilters.gain(0, 0.0);
        mixerFilters.gain(1, 0.0);
        mixerFilters.gain(2, 0.0);
        mixerFilters.gain(3, 0.0);
        mixerFinal.gain(0, 0.0);
        mixerFinal.gain(1, 0.0);
        mixerFinal.gain(2, 1.0);
        mixerFinal.gain(3, delayGain);
    }
}

#define ALGOCOUNT 6

int algorithm = 0;

void setAlgorithm(int algo)
{
    algorithm = algo;
    if (algorithm % 2 == 1)
    {
        delayGain = 0.55; // we could make the delay depending on the position
    }
    else
    {
        delayGain = 0;
    }
    setMix();
}

void showRgb(int r, int g, int b)
{
    analogWrite(pinRGBLed[0], r * 4);
    analogWrite(pinRGBLed[1], g * 2);
    analogWrite(pinRGBLed[2], b * 5);
}

int calibrateInputPressCount = 0;

void calibrateLedPlay()
{
    delay(50);
    showRgb(1, 0, 0);
    delay(50);
    showRgb(1, 1, 0);
    delay(50);
    showRgb(0, 1, 0);
    delay(50);
    showRgb(0, 1, 1);
    delay(50);
    showRgb(0, 0, 1);
    delay(50);
    showRgb(1, 0, 1);
}

void setup()
{
    Serial.begin(115200);
    lineInLevel  = EEPROM.read(0);
    lineOutLevel = EEPROM.read(2);
    Serial.print("Line in level: ");
    Serial.println(lineInLevel);
    for (int i = 0; i < 3; ++i)
    {
        pinMode(pinRGBLed[i], OUTPUT);
        analogWrite(pinRGBLed[i], 0);
    }
    pinMode(c_centerButton, INPUT_PULLUP);
    if (digitalRead(c_centerButton) == 0)
    {
        calibrateLedPlay();
        calibrateInputPressCount = 3;
        lineInLevel              = 15;
    }

    centerButton.attach(c_centerButton);
    centerButton.interval(15); // interval in ms
    pinMode(c_centerButton, INPUT_PULLUP);

    AudioNoInterrupts();
    const unsigned int blocks = 44100 / 128 * 200 / 1000;
    Serial.printf("blocks: %d\n", blocks);
    AudioMemory(blocks);
    // Enable the audio shield, select input, and enable output
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.5);
    sgtl5000_1.adcHighPassFilterEnable();

    sgtl5000_1.audioProcessorDisable();
    // sgtl5000_1.adcHighPassFilterDisable();
    sgtl5000_1.muteHeadphone();
    sgtl5000_1.lineInLevel(lineInLevel); // 13 -> 0.34 Volts --> 1.18647^(6.65-n)
    sgtl5000_1.lineOutLevel(30);         // 1.22V  --> 1.057255^(33.7-n)
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    filterLeftStage1.octaveControl(2);
    filterLeftStage1.frequency(400);
    filterLeftStage1.resonance(4.0);
    filterLeftStage2.octaveControl(2);
    filterLeftStage2.frequency(400);
    filterLeftStage2.resonance(4.0);
    filterRightStage1.octaveControl(2);
    filterRightStage1.frequency(800);
    filterRightStage1.resonance(4.0);
    filterRightStage2.octaveControl(2);
    filterRightStage2.frequency(800);
    filterRightStage2.resonance(4.0);
    inputFilter.setHighpass(0, 40, 1.5);
    inputFilter.setLowpass(1, 5000, 1.5);
    slapBackDelay.delay(0, 0);
    slapBackDelay.delay(1, 80);
    noiseGate.setThresholdInDb(-56);
    setMix();
    AudioInterrupts();
    showRgb(1, 0, 0);
}

elapsedMillis volmsec = 0;

unsigned long nextT;
unsigned long nextAnalysis;

int flip        = 0;
int swapFilters = 0;
int fadetime    = 3;
int fadeOuttime = 3;

void newParams(float new_f)
{
    AudioNoInterrupts();

    if (swapFilters)
    {
        dcStage1.amplitude(new_f);
        fadeLeftStage1.fadeIn(fadetime);
        fadeRightStage1.fadeIn(fadetime);
        fadeLeftStage2.fadeOut(fadeOuttime);
        fadeRightStage2.fadeOut(fadeOuttime);
    }
    else
    {
        dcStage2.amplitude(new_f);
        fadeLeftStage2.fadeIn(fadetime);
        fadeRightStage2.fadeIn(fadetime);
        fadeLeftStage1.fadeOut(fadeOuttime);
        fadeRightStage1.fadeOut(fadeOuttime);
    }
    AudioInterrupts();
    swapFilters = 1 - swapFilters;
}

float lastPeak = 0.0;
float lastRms  = 0.0;
int   cntPeaks = 0;
int   cntRms   = 0;

TapDaBeat tapDaBeat;

uint32_t        nextPot = 0;
static uint32_t longPressStart;

void checkCenterButton()
{
    static int lastCenterButton = 1;
    centerButton.update();
    int value = centerButton.read();
    if (value != lastCenterButton)
    {
        Serial.printf("Center button now %d\n", value);
        lastCenterButton = value;
        if (value == 0)
        {
            if (calibrateInputPressCount)
            {
                --calibrateInputPressCount;
                if (!calibrateInputPressCount)
                {
                    EEPROM.write(0, lineInLevel);
                    Serial.printf("Line in level written to eeprom: %f\n", lineInLevel);
                    EEPROM.write(2, lineOutLevel);
                    Serial.printf("Line out level written to eeprom: %f\n", lineOutLevel);
                }
            }

            isFlipping = !isFlipping;
            setMix();
            longPressStart = millis();
            tapDaBeat.tap(longPressStart);
            showRgb(0, 1, 0);
        }
        else
        {
            showRgb(0, 0, 0);
        }
    }
}

PotReader prFrequency(D6, true, 1023);
PotReader prDivider(D2, true, 1023);
PotReader prAlgorithm(D7, true, 1023);
PotReader prMix(D3, true, 1023);

float dividers[] = {1.0, 0.5, 0.25, 0.125};

void loop()
{
    checkCenterButton();
    uint32_t t = millis();
    if (isFlipping)
    {
        if (t >= nextT)
        {
            float bpm       = tapDaBeat.bpm();
            float msecsPer4 = 60000 / bpm;
            int   slot      = prDivider.read() * (sizeof(dividers) / sizeof(dividers[0])) / 1024;
            nextT           = t + msecsPer4 * dividers[slot];
            flip            = 1 - flip;
            switch (algorithm)
            {
                case 0:
                case 1:
                {
                    float v = float(rand() % RAND_MAX) / RAND_MAX;
                    showRgb(flip == 1 ? 1 : 0, algorithm % 2, 0);
                    if (flip == 1)
                    {
                        newParams(v);
                    }
                    else
                    {
                        newParams(-v);
                    }
                }
                break;
                case 2:
                case 3:
                {
                    float v = (float(rand() % RAND_MAX) / RAND_MAX) * 2.0 - 1;
                    showRgb(0, flip == 1 ? 1 : 0, algorithm % 2);
                    newParams(v);
                    break;
                }
                case 4:
                case 5:
                {
                    float v = (float(rand() % RAND_MAX) / RAND_MAX);
                    v       = v * v * v;
                    v       = v * 2.0f - 1.0f;
                    showRgb(algorithm % 2, 0, flip == 1 ? 1 : 0);
                    newParams(v);
                    break;
                }
            }
        }
    }

    prFrequency.update(t);
    prDivider.update(t);
    prAlgorithm.update(t);
    prMix.update(t);
    if (prAlgorithm.hasNewValueAndResetState())
    {
        setAlgorithm(prAlgorithm.read() / 1024.0 * ALGOCOUNT);
    }
    if (calibrateInputPressCount)
    {
        calibratedLedPlay();
        if (prMix.hasNewValueAndResetState())
        {
            lineOutLevel = 13 + (32 - 13) * (prMix.read() / 1024.0);
            if (lineOutLevel < 13)
                lineOutLevel = 13;
            if (lineOutLevel > 32)
                lineOutLevel = 31;
            AudioNoInterrupts();
            sgtl5000_1.lineOutLevel(lineOutLevel);
            AudioInterrupts();
        }
    }
    else
    {
        if (prMix.hasNewValueAndResetState())
        {
            mix = prMix.read() / 1024.0;
            setMix();
        }
    }
    if (prFrequency.hasNewValueAndResetState())
    {
        int f = 100 + 1900 * prFrequency.read() / 1024;
        filterLeftStage1.frequency(f);
        filterLeftStage2.frequency(f * 2);
        filterRightStage1.frequency(f);
        filterRightStage2.frequency(f * 2);
    }
    if (peakIn.available())
    {
        cntPeaks++;
        auto newPeak = peakIn.read();
        if (newPeak > lastPeak)
        {
            lastPeak = newPeak;
            Serial.printf("Peak: %f\n", log(lastPeak) / log(2) * 6);
            if (calibrateInputPressCount)
            {
                if (lastPeak > 0.85)
                {
                    --lineInLevel;
                    if (lineInLevel < 0)
                        lineInLevel = 0;
                    Serial.printf("Reducing line in level to %d", lineInLevel);
                    AudioNoInterrupts();
                    sgtl5000_1.lineInLevel(lineInLevel); // 0.34 Volts --> 1.18647^(6.65-n)
                    AudioInterrupts();
                    lastPeak = 0.0;
                }
            }
        }
    }
    if (millis() >= nextAnalysis)
    {
        nextAnalysis += 10000;
        Serial.printf("AudioProcessorUsage=%f max=%f\n", (float) AudioProcessorUsage(),
                      (float) AudioProcessorUsageMax());
        Serial.printf("AudioMemoryUsage=%d max=%d\n", AudioMemoryUsage(), AudioMemoryUsageMax());
        AudioProcessorUsageMaxReset();
        AudioMemoryUsageMaxReset();
        lastPeak = 0.0;
        lastRms  = 0.0;
        cntPeaks = 0;
        cntRms   = 0;
    }
}
