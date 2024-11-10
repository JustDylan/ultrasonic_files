// ultrasonic_files.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL.h>
#include <math.h>
#include "FrequencyGenerator.h"

using std::cout;

struct WaveData
{
    int samplesPerSecond;
    int bytesPerSample;
    int tonePosition;
    float toneHz;
    float targetToneHz;
    float toneVolume;
    float wavePeriod;

};

void AudioCallback(void* UserData, Uint8* Stream, int Length)
{
    WaveData* AudioUserData = (WaveData*)UserData;
    static Uint32 Count = 0;

    Sint16* SampleBuffer = (Sint16*)Stream;
    int SamplesToWrite = Length / AudioUserData->bytesPerSample;

    // link target waveform to current waveform
    if (AudioUserData->toneHz != AudioUserData->targetToneHz)
    {
        AudioUserData->tonePosition = AudioUserData->toneHz * AudioUserData->tonePosition / AudioUserData->targetToneHz;
        AudioUserData->toneHz = AudioUserData->targetToneHz;
    }

    for (int i = 0; i < SamplesToWrite; ++i)
    {
        // number of samples to one cycle of the tone
        float samplesPerCycle = float(AudioUserData->samplesPerSecond) / AudioUserData->toneHz;
        Sint16 ToneValue = (0.5f + (AudioUserData->toneVolume * sin(2.0f * M_PI * AudioUserData->tonePosition / samplesPerCycle)));
        SampleBuffer[i] = ToneValue;
        
        // advance in tone
        ++(AudioUserData->tonePosition);
    }

    AudioUserData->toneHz = AudioUserData->targetToneHz;
}

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        cout << SDL_GetError();
        return 1;
    }

    std::cout << "Hello World!\n";

    WaveData AudioUserData = { 0 };
    AudioUserData.samplesPerSecond = 48000;
    AudioUserData.bytesPerSample = sizeof(Sint16);
    AudioUserData.tonePosition = 0;
    AudioUserData.toneVolume = 3000;
    AudioUserData.toneHz = 840;
    AudioUserData.targetToneHz = AudioUserData.toneHz;
    

    SDL_AudioSpec Want, Have;
    SDL_AudioDeviceID AudioDeviceID;
    Want.freq = AudioUserData.samplesPerSecond;
    Want.format = AUDIO_S16;
    Want.channels = 1;
    // number of samples to process before processing change in tone
    Want.samples = 4096;
    Want.callback = &AudioCallback;
    Want.userdata = &AudioUserData;
    AudioDeviceID = SDL_OpenAudioDevice(0, 0, &Want, &Have, 0);

    SDL_PauseAudioDevice(AudioDeviceID, 0);

    int Running = 1;
    while (Running)
    {

        SDL_Event Event;
        while (SDL_PollEvent(&Event))
        {
            if (Event.type == SDL_QUIT)
            {
                Running = 0;
                break;
            }
        }

        AudioUserData.targetToneHz -= 25;

        SDL_Delay(500);
    }

    return 0;
}

