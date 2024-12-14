// ultrasonic_files.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL.h>
#include <math.h>
#include "FrequencyGenerator.h"

static float const PI = M_PI;

using std::cout;
using std::endl;

struct WaveData
{
    int samplesPerSecond;
    int bytesPerSample;
    float tonePosition;
    float toneHz;
    float targetToneHz;
    float toneVolume;
};

struct ListenData
{
    float peak;
    float test;
    int samplesPerSecond;
    int bytesPerSample;
};

// discrete fourier transform component for frequency k
float DFT(Sint16* samples, int length, int k)
{
    // calculate real and imaginary components for frequency component k
    float realSum = 0;
    float imaginarySum = 0;
    for (int n = 0; n < length; ++n)
    {
        realSum += samples[n] * cosf(-2.0f * PI / length * k * n);
        imaginarySum += samples[n] * sinf(-2.0f * PI / length * k * n);
    }

    // calculate frequency magnitude
    return sqrt(realSum * realSum + imaginarySum * imaginarySum);
}

void AudioCallback(void* UserData, Uint8* Stream, int Length)
{
    WaveData* AudioUserData = (WaveData*)UserData;

    Sint16* SampleBuffer = (Sint16*)Stream;
    int SamplesToWrite = Length / AudioUserData->bytesPerSample;

    // number of samples to one cycle of the target tone
    float samplesPerCycle = float(AudioUserData->samplesPerSecond) / AudioUserData->targetToneHz;

    // link target waveform to current waveform
    if (AudioUserData->toneHz != AudioUserData->targetToneHz)
    {
        float c = 2.0f * PI / AudioUserData->samplesPerSecond;
        int i = AudioUserData->tonePosition * AudioUserData->toneHz * c / (2.0f * PI);

        //offset to nearest identical position for target tone
        float positionOffset = 2.0f * PI * i / (AudioUserData->targetToneHz * c);

        AudioUserData->tonePosition = AudioUserData->toneHz * AudioUserData->tonePosition / AudioUserData->targetToneHz - positionOffset;
        AudioUserData->toneHz = AudioUserData->targetToneHz;
    }

    for (int i = 0; i < SamplesToWrite; ++i)
    {
        Sint16 ToneValue = (0.5f + (AudioUserData->toneVolume * sin(2.0f * PI * AudioUserData->tonePosition / samplesPerCycle)));
        SampleBuffer[i] = ToneValue;
        
        // advance in tone
        ++(AudioUserData->tonePosition);
    }
}

void ListenCallback(void* UserData, Uint8* Stream, int Length)
{
    ListenData* AudioUserData = (ListenData*)UserData;

    Sint16* SampleBuffer = (Sint16*)Stream;
    int samplesToRead = Length / AudioUserData->bytesPerSample;

    // frequency 234hz for 4096 samples
    int frequencyK = 20;
    AudioUserData->peak = DFT(SampleBuffer, samplesToRead, frequencyK);

    // frequency 1172hz
    AudioUserData->test = DFT(SampleBuffer, samplesToRead, 100);


    /*
    for (int i = 0; i < 50; ++i)
    {
        if(i != frequencyK)
            AudioUserData->test += DFT(SampleBuffer, samplesToRead, i);
    }*/

    /*
    for (int i = 0; i < samplesToRead; ++i)
    {
        if ((float)SampleBuffer[i]*(float)SampleBuffer[i] > AudioUserData->peak*AudioUserData->peak)
            AudioUserData->peak = SampleBuffer[i];
    }*/
    
}

int main(int argc, char* argv[])
{
    //TESTING
    //const char* testArgs[] = { "ultrasonic_files.exe", "listen", "message" };
    argc = 2;
    //argv = (char**)&testArgs;
    argv = new char* [argc];
    argv[0] = (char*)("test");
    argv[1] = (char*)("listen");
    //argv[3] = (char*)"message";
    cout << "argc: " << argc << "\nargv:\n";
    for (int i = 0; i < argc; ++i)
    {
        cout << argv[i] << endl;
    }

    //TESTING END

    if (argc < 2)
    {
        cout << "missing arguments: mode, message";
    }

    if (SDL_Init(SDL_INIT_AUDIO) == -1)
    {
        cout << SDL_GetError();
        return 1;
    }

    // transmit mode
    if (strcmp(argv[1], "transmit") == 0 && argc >= 3)
    {

        cout << "Transmit Mode\n";

        WaveData AudioUserData = { 0 };
        AudioUserData.samplesPerSecond = 48000;
        AudioUserData.bytesPerSample = sizeof(Sint16);
        AudioUserData.tonePosition = 0;
        AudioUserData.toneVolume = 3000;
        AudioUserData.toneHz = 19000;
        AudioUserData.targetToneHz = AudioUserData.toneHz;

        SDL_AudioSpec Want, Have;
        SDL_AudioDeviceID AudioDeviceID;
        Want.freq = AudioUserData.samplesPerSecond;
        Want.format = AUDIO_S16;
        Want.channels = 1;
        // number of samples to process before processing change in tone
        Want.samples = 4096;

        Want.userdata = &AudioUserData;
        Want.callback = &AudioCallback;
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

            AudioUserData.targetToneHz += 25;
            cout << AudioUserData.tonePosition << endl;

            SDL_Delay(500);
        }
    }
    // listen mode
    else if (strcmp(argv[1], "listen") == 0)
    {
        cout << "Listen Mode\n";

        ListenData AudioUserData = { 0 };
        AudioUserData.samplesPerSecond = 48000;
        AudioUserData.bytesPerSample = sizeof(Sint16);
        AudioUserData.peak = 0;

        SDL_AudioSpec Want, Have;
        SDL_AudioDeviceID AudioDeviceID;
        Want.freq = AudioUserData.samplesPerSecond;
        Want.format = AUDIO_S16;
        Want.channels = 1;
        // number of samples to process before processing change in tone
        Want.samples = 4096;

        Want.userdata = &AudioUserData;
        Want.callback = &ListenCallback;
        AudioDeviceID = SDL_OpenAudioDevice(0, 1, &Want, &Have, 0);

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

            cout << "234hz : " << AudioUserData.peak << endl;
            cout << "1172hz: " << AudioUserData.test << endl;
            AudioUserData.peak = 0;
            AudioUserData.test = 0;

            SDL_Delay(500);
        }
    }

    return 0;
}

