

/*
 * new algorithms
 * remove slap back
 *
 * 0 = flip low high
 * 1 = random
 * 2 = mainly deep
 * 3 = random with sine modulation beat*8
 */

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

#include "filter_variable_limited.h"

// #define TEST_STANDALONE
// #define DEBUG_FILTER_VARIABLE_LIMITED
AudioInputI2S            i2sIn;
AudioSynthWaveformDc     dcStage1;
AudioSynthWaveformDc     dcStage2;
AudioFilterBiquad        inputFilter;
AudioNoiseGate           noiseGate;
AudioFilterStateVariableLimited filterLeftStage1;
AudioFilterStateVariableLimited filterLeftStage2;
AudioFilterStateVariableLimited filterRightStage1;
AudioFilterStateVariableLimited filterRightStage2;

AudioEffectFade fadeLeftStage1;
AudioEffectFade fadeLeftStage2;
AudioEffectFade fadeRightStage1;
AudioEffectFade fadeRightStage2;


AudioMixer4 mixerFilters;
AudioMixer4 mixerFinal;

AudioOutputI2S   i2sOut;
AudioAnalyzePeak peakIn;
AudioAnalyzePeak peakOut;
#ifdef TEST_STANDALONE
AudioConnection patchCordPeak(i2sIn, 1, peakIn, 0);
AudioConnection patchInputFilter(i2sIn, 1, inputFilter, 0);
#else
AudioConnection patchCordPeak(i2sIn, 0, peakIn, 0);
AudioConnection patchInputFilter(i2sIn, 0, inputFilter, 0);
#endif
AudioConnection patchNoiseGate(inputFilter, 0, noiseGate, 0);
AudioConnection patchCord2(noiseGate, 0, filterRightStage1, 0);
AudioConnection patchCord3(noiseGate, 0, filterRightStage2, 0);
AudioConnection patchCordMixSignal(noiseGate, 0, mixerFinal, 2);

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
AudioConnection patchCordPeakOut(mixerFinal, 0,  peakOut, 0);

AudioControlSGTL5000 sgtl5000_1;

int       pinRGBLed[3]   = {3, 5, 4};
#ifndef TEST_STANDALONE
int       potPins[4]     = {6, 2, 7, 3};

const int c_centerButton = 2;
#endif

Bounce    centerButton;
int lineInLevel  = 1;
int lineOutLevel = 31;

bool  isFlipping = true;
float mix        = 1.0;

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
    }
}

#define ALGOCOUNT 4

int algorithm = 0;

void setAlgorithm(int algo)
{
    algorithm = algo;
    setMix();
}

int calibrateInputPressCount = 0;
void showRgb(int r, int g, int b)
{
#ifndef TEST_STANDALONE
    analogWrite(pinRGBLed[0], r * 4);
    analogWrite(pinRGBLed[1], g * 2);
    analogWrite(pinRGBLed[2], b * 5);
#endif
}


void calibrateLedPlay()
{
    static int step = 0;
    static int tNext = 0;
    if (millis()<tNext)
        return;
    tNext = millis()+50;
    step++;
    switch (step)
    {
        case 0: showRgb(1, 0, 0); break;
        case 1: showRgb(1, 1, 0); break;
        case 2: showRgb(0, 1, 0); break;
        case 3: showRgb(0, 1, 1); break;
        case 4: showRgb(0, 0, 1); break;
        case 5: showRgb(1, 0, 1); step = -1; break;
    }
}


void setup()
{
    Serial.begin(115200);
#ifndef TEST_STANDALONE
    lineInLevel  = EEPROM.read(0);
    if (lineInLevel < 0) lineInLevel = 0;
    if (lineInLevel > 15) lineInLevel = 15;
    lineOutLevel = EEPROM.read(2);
    if (lineOutLevel < 13) lineOutLevel = 13;
    if (lineOutLevel > 31) lineOutLevel = 31;
    Serial.print("EEprom Line in level: ");
    Serial.println(lineInLevel);
    Serial.print("EEprom Line out level: ");
    Serial.println(lineOutLevel);
    for (int i = 0; i < 3; ++i)
    {
        pinMode(pinRGBLed[i], OUTPUT);
        analogWrite(pinRGBLed[i], 0);
    }
    showRgb(1, 0, 0);

    pinMode(c_centerButton, INPUT_PULLUP);
    delay(100);
    if (digitalRead(c_centerButton) == 0)
    {
        calibrateLedPlay();
        calibrateInputPressCount = 3;
        lineInLevel              = 15;
        Serial.print("Calibration, setting Line in level: ");
        Serial.println(lineInLevel);
    }

    centerButton.attach(c_centerButton);
    centerButton.interval(15); // interval in ms
    pinMode(c_centerButton, INPUT_PULLUP);
#endif
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
    sgtl5000_1.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000_1.lineInLevel(lineInLevel); // 13 -> 0.34 Volts --> 1.18647^(6.65-n)
    sgtl5000_1.lineOutLevel(lineOutLevel);         // 1.22V  --> 1.057255^(33.7-n)

    filterLeftStage1.octaveControl(2);
    filterLeftStage1.frequency(100);
    filterLeftStage1.resonance(5.0);

    filterLeftStage2.octaveControl(2);
    filterLeftStage2.frequency(200);
    filterLeftStage2.resonance(5.0);

    filterRightStage1.octaveControl(2);
    filterRightStage1.frequency(100);
    filterRightStage1.resonance(5.0);

    filterRightStage2.octaveControl(2);
    filterRightStage2.frequency(200);
    filterRightStage2.resonance(5.0);
    inputFilter.setHighpass(0, 40, 1.5);
    inputFilter.setLowpass(1, 5000, 1.5);
    noiseGate.setThresholdInDb(-56);
    setMix();
    AudioInterrupts();
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
float lastPeakOut = 0.0;
float lastRms  = 0.0;
int   cntPeaks = 0;
int   cntRms   = 0;

TapDaBeat tapDaBeat;

int sweepWidth = 60000/120*8;
uint32_t sweepStart = 0;

uint32_t        nextPot = 0;
static uint32_t longPressStart;
#ifndef TEST_STANDALONE
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
                    Serial.printf("Line in level written to eeprom: %d\n", lineInLevel);
                    EEPROM.write(2, lineOutLevel);
                    Serial.printf("Line out level written to eeprom: %df\n", lineOutLevel);
                }
            }

            isFlipping = !isFlipping;
            setMix();
            longPressStart = millis();
            tapDaBeat.tap(longPressStart);
            float msecsPerBeat = 60000 / tapDaBeat.bpm();
            sweepWidth = static_cast<int>(3 * msecsPerBeat);
            sweepStart = millis();
            showRgb(0, 1, 0);
        }
        else
        {
            showRgb(0, 0, 0);
        }
    }
}


PotReader prFrequency(6, true, 1023);
PotReader prDivider(2, true, 1023);
PotReader prAlgorithm(7, true, 1023);
PotReader prMix(3, true, 1023);
#else
void checkCenterButton()
{}

#endif

float dividers[] = {1.0, 0.5, 0.25, 0.125};

int centerFreq = 400;
int currentFrequency = 0;

void setFrequency(int f)
{
    if (f!=currentFrequency)
    {
        currentFrequency = f;
        filterLeftStage1.frequency(f);
        filterLeftStage2.frequency(f * 2);
        filterRightStage1.frequency(f);
        filterRightStage2.frequency(f * 2);
    }
}

void setNewCenterFrequency(int f)
{
    centerFreq = f;
    setFrequency(f);
}

void loop()
{
    checkCenterButton();
    uint32_t t = millis();
    if (algorithm == 3)
    {
        float phase = float(t-sweepStart) / sweepWidth;
        if (phase>=1.0)
            sweepStart += sweepWidth;
        float sw = sin(M_PI*2.0*phase);
        //Serial.printf("f=%f sw=%f phase=%f\n",centerFreq+centerFreq*sw/2,sw, phase);
        setFrequency(centerFreq+centerFreq*sw*3/4);
    }
    if (isFlipping)
    {
        if (t >= nextT)
        {
            float bpm       = tapDaBeat.bpm();
            float msecsPer4 = 60000 / bpm;
#ifndef TEST_STANDALONE
            int   slot      = prDivider.read() * (sizeof(dividers) / sizeof(dividers[0])) / 1024;
#else
            int slot  = 2;
#endif
            nextT           = t + msecsPer4 * dividers[slot];
            flip            = 1 - flip;
            switch (algorithm)
            {
                case 3:
                case 0:
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
                case 1:
                {
                    float v = (float(rand() % RAND_MAX) / RAND_MAX) * 2.0 - 1;
                    showRgb(0, flip == 1 ? 1 : 0, 0);
                    newParams(v);
                    break;
                }
                case 2:
                {
                    float v = (float(rand() % RAND_MAX) / RAND_MAX);
                    v       = v * v * v;
                    v       = v * 2.0f - 1.0f;
                    showRgb(0, 0, flip == 1 ? 1 : 0);
                    newParams(v);
                    break;
                }
            }
        }
    }
#ifndef TEST_STANDALONE
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
        calibrateLedPlay();
        if (prMix.hasNewValueAndResetState())
        {
            lineOutLevel = 13 + (32 - 13) * (prMix.read() / 1024.0);
            if (lineOutLevel < 13)
                lineOutLevel = 13;
            if (lineOutLevel > 32)
                lineOutLevel = 31;
            Serial.printf("Lineout level set to %d\n", lineOutLevel);
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
       setNewCenterFrequency(f);
    }
#endif
    static float peakInCurrent;
    if (peakIn.available())
    {
        cntPeaks++;
        auto newPeak = peakIn.read();
        peakInCurrent = log(newPeak) / log(2) * 6;
        if (newPeak > lastPeak)
        {
            lastPeak = newPeak;
            Serial.printf("Peak: %f\n", log(lastPeak) / log(2) * 6);
            if (calibrateInputPressCount)
            {
                if (lastPeak > 0.5)
                {
                    --lineInLevel;
                    if (lineInLevel < 0)
                        lineInLevel = 0;
                    Serial.printf("Reducing line in level to %d\n", lineInLevel);
                    AudioNoInterrupts();
                    sgtl5000_1.lineInLevel(lineInLevel); // 0.34 Volts --> 1.18647^(6.65-n)
                    AudioInterrupts();
                    lastPeak = 0.0;
                }
            }
        }
    }
#ifdef DEBUG_FILTER_VARIABLE_LIMITED
    static float peakOutCurrent;
    if (peakOut.available())
    {
        auto newPeak = peakOut.read();
        peakOutCurrent = log(newPeak) / log(2) * 6;
        if (newPeak > lastPeakOut)
        {
            lastPeakOut = newPeak;
            Serial.printf("PeakOut: %f\n", log(lastPeakOut) / log(2) * 6);
        }
    }
    static unsigned long voltageShow = 0;
    if (millis() >voltageShow)
    {
        if (peakOutCurrent > -1)
        if (filterLeftStage1.getSignalPower() > 2)
        {
            Serial.printf("%5d %5d %5d %5d pin=%5.2f  p=%5.2f\n", (int) filterLeftStage1.getSignalPower(),
                          (int) filterLeftStage1.getStateLimitFactor(), (int) filterLeftStage2.getSignalPower(),
                          (int) filterLeftStage2.getStateLimitFactor(), peakInCurrent, peakOutCurrent);
        }
        voltageShow = millis()+20;
    }
#endif
    if (millis() >= nextAnalysis)
    {
        nextAnalysis += 10000;
        Serial.printf("AudioProcessorUsage=%f max=%f\n", (float) AudioProcessorUsage(),
                      (float) AudioProcessorUsageMax());
        Serial.printf("AudioMemoryUsage=%d max=%d\n", AudioMemoryUsage(), AudioMemoryUsageMax());
        AudioProcessorUsageMaxReset();
        AudioMemoryUsageMaxReset();
        lastPeak = 0.0;
        lastPeakOut = 0.0;
        lastRms  = 0.0;
        cntPeaks = 0;
        cntRms   = 0;
    }
}
