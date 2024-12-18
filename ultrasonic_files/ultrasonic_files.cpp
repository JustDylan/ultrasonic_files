// ultrasonic_files.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <SDL.h>
#include <math.h>
#include <vector>

static float const PI = M_PI;

int const pulseWidth = 3000;

// frequency 20400hz
static int const clockFreq = 1350;
static int const clockFreqSig = 21600;

// frequency 19200hz
static int const offFreq = 1200;
static int const offFreqSig = 19200;
// frequency 21600hz
static int const onFreq = 1275;
static int const onFreqSig = 20400;
// frequency 18000hz
static int const stopFreq = 1125;
static int const stopFreqSig = 18000;

using std::cout;
using std::endl;
using std::vector;

struct WaveData
{
    int samplesPerSecond;
    int bytesPerSample;
    float tonePosition;
    float toneHz;
    float targetToneHz;
    float toneVolume;
    bool* bitBuffer;
    int bufferIndex;
    int bufferLength;
};

struct ListenData
{
    float peak;
    float test;
    int samplesPerSecond;
    int bytesPerSample;
    int* outBuffer;
    int outLength;
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
    //cout << "start\n";
    WaveData* AudioUserData = (WaveData*)UserData;

    Sint16* SampleBuffer = (Sint16*)Stream;
    int SamplesToWrite = Length / AudioUserData->bytesPerSample;

    // write each pulse to samples
    int j = 0;
    for (j = 0; j < SamplesToWrite; j += pulseWidth)
    {
        if (AudioUserData->bufferIndex >= AudioUserData->bufferLength)
        {
            AudioUserData->targetToneHz = stopFreqSig;
        }
        else if (AudioUserData->bitBuffer[AudioUserData->bufferIndex])
        {
            AudioUserData->bufferIndex++;
            AudioUserData->targetToneHz = onFreqSig;
        }
        else
        {
            AudioUserData->bufferIndex++;
            AudioUserData->targetToneHz = offFreqSig;
        }

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

        //DEBUG
        SDL_assert(j + pulseWidth <= SamplesToWrite);
        //write samples
        for (int i = j; i < j+pulseWidth; ++i)
        {
            Sint16 ToneValue = (0.5f + (AudioUserData->toneVolume * sin(2.0f * PI * AudioUserData->tonePosition / samplesPerCycle)));
            SampleBuffer[i] = ToneValue;

            // advance in tone
            ++(AudioUserData->tonePosition);
        }

        
    }
    //cout << "stop\n";
}

void ListenCallback(void* UserData, Uint8* Stream, int Length)
{
    ListenData* AudioUserData = (ListenData*)UserData;

    Sint16* SampleBuffer = (Sint16*)Stream;
    int samplesToRead = Length / AudioUserData->bytesPerSample;

    

    //TEST magic number
    int const bitBufferSize = 48000 / pulseWidth;
    int* bitArray = new int[bitBufferSize];
    for (int i = 0; i < bitBufferSize; ++i)
    {
        bitArray[i] = -1;
    }

    AudioUserData->outBuffer = bitArray;

    //find start of pulse
    int maxIndex = 0;
    float maxMag = 0;
    for (int i = 0; i < pulseWidth; i += 30)
    {
        float onBitMag = DFT(SampleBuffer + i, pulseWidth, onFreq);
        float offBitMag = DFT(SampleBuffer + i, pulseWidth, offFreq);
        float stopBitMag = DFT(SampleBuffer + i, pulseWidth, stopFreq);

        float maxLocalMag = stopBitMag;
        if (stopBitMag > onBitMag && stopBitMag > offBitMag)
        {
            maxLocalMag = stopBitMag;
        }
        else if (onBitMag > offBitMag && onBitMag > stopBitMag)
        {
            maxLocalMag = onBitMag;
        }

        if (maxLocalMag > maxMag)
        {
            maxIndex = i;
            maxMag = maxLocalMag;
        }
    }

    //cout << "Start index: " << maxIndex << endl;

    // interpret bits
    int arrayIndex = 0;
    for (int i = maxIndex; i+pulseWidth <= samplesToRead; i += pulseWidth)
    {
        float onBitMag = DFT(SampleBuffer + i, pulseWidth, onFreq);
        float offBitMag = DFT(SampleBuffer + i, pulseWidth, offFreq);
        float stopBitMag = DFT(SampleBuffer + i, pulseWidth, stopFreq);

        if (stopBitMag > onBitMag && stopBitMag > offBitMag)
        {
            bitArray[arrayIndex++] = -2;
        }
        else if (onBitMag > offBitMag && onBitMag > stopBitMag)
        {
            bitArray[arrayIndex++] = 1;
        }
        else
        {
            bitArray[arrayIndex++] = 0;
        }
    }

    // signal that buffer is full
    AudioUserData->outLength = bitBufferSize;

    //AudioUserData->peak = DFT(SampleBuffer, 100, 41);
    
    /*for (int i = 0; i < 200; ++i)
    {
        AudioUserData->peak += DFT(SampleBuffer+i, 100, 41);
    }

    for (int i = 0; i < 200; ++i)
    {
        AudioUserData->test += DFT(SampleBuffer+i, 100, 1);
    }

    AudioUserData->peak /= 200;
    AudioUserData->test /= 200;*/

    // frequency 19980hz
    //AudioUserData->test = DFT(SampleBuffer, 100, 1);


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
    if (argc < 2)
    {
        //const char* testArgs[] = { "ultrasonic_files.exe", "listen", "message" };
        argc = 3;
        //argv = (char**)&testArgs;
        argv = new char* [argc];
        argv[0] = (char*)("test");
        argv[1] = (char*)("listen");
        argv[2] = (char*)("message");
        //argv[3] = (char*)"message";
        cout << "argc: " << argc << "\nargv:\n";
        for (int i = 0; i < argc; ++i)
        {
            cout << argv[i] << endl;
        }
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
        char message = argv[2][0];

        cout << "Transmit Mode\n";

        WaveData AudioUserData = { 0 };
        AudioUserData.samplesPerSecond = 48000;
        AudioUserData.bytesPerSample = sizeof(Sint16);
        AudioUserData.tonePosition = 0;
        AudioUserData.toneVolume = 3000;
        AudioUserData.toneHz = 19000;
        AudioUserData.targetToneHz = AudioUserData.toneHz;

        // load data:
        AudioUserData.bufferIndex = 0;
        AudioUserData.bufferLength = 8;
        //TEST
        AudioUserData.bitBuffer = new bool[AudioUserData.bufferLength];
        /*for (int i = 0; i < AudioUserData.bufferLength; ++i)
        {
            AudioUserData.bitBuffer[i] = i%6 < 3;
        }*/

        for (int i = 0; i < 8; ++i)
        {
            bool nextBit = (message >> i) & 1;
            AudioUserData.bitBuffer[i] = nextBit;
            //AudioUserData.bitBuffer[3*i+1] = nextBit;
            //AudioUserData.bitBuffer[3*i+2] = nextBit;
            /*AudioUserData.bitBuffer[i * 5+1] = nextBit;
            AudioUserData.bitBuffer[i * 5+2] = nextBit;
            AudioUserData.bitBuffer[i * 5 + 3] = nextBit;
            AudioUserData.bitBuffer[i * 5 + 4] = nextBit;*/
        }
        

        SDL_AudioSpec Want, Have;
        SDL_AudioDeviceID AudioDeviceID;
        Want.freq = AudioUserData.samplesPerSecond;
        Want.format = AUDIO_S16;
        Want.channels = 1;
        // number of samples to process before processing change in tone
        //Want.samples = 4096;
        Want.samples = AudioUserData.samplesPerSecond;

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

            //AudioUserData.targetToneHz += 25;
            //cout << AudioUserData.bufferIndex << endl;
            if (AudioUserData.bufferIndex >= AudioUserData.bufferLength)
            {
                AudioUserData.bufferIndex = 0;
                //cout << "sent" << endl;
            }

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

        AudioUserData.outLength = 0;

        SDL_AudioSpec Want, Have;
        SDL_AudioDeviceID AudioDeviceID;
        Want.freq = AudioUserData.samplesPerSecond;
        Want.format = AUDIO_S16;
        Want.channels = 1;
        // number of samples to process before processing change in tone
        //Want.samples = 4096;
        Want.samples = AudioUserData.samplesPerSecond;

        Want.userdata = &AudioUserData;
        Want.callback = &ListenCallback;
        AudioDeviceID = SDL_OpenAudioDevice(0, 1, &Want, &Have, 0);

        SDL_PauseAudioDevice(AudioDeviceID, 0);

        int readIndex = 0;
        vector<int> outputPulses = vector<int>();
        outputPulses.push_back(-2);

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

            // get buffer from last sample buffer
            if (AudioUserData.outLength != 0)
            {
                //cout << endl;
                for (int i = 0; i < AudioUserData.outLength; ++i)
                {
                    //cout << AudioUserData.outBuffer[i] << ", ";
                }
                //cout << endl;

                int redundancy = 1;
                // condense redundant bits
                for (int i = 0; i+redundancy <= AudioUserData.outLength; i += redundancy)
                {
                    int onTally = 0;
                    int offTally = 0;
                    int stopTally = 0;
                    for (int j = 0; j < redundancy; ++j)
                    {
                        switch (AudioUserData.outBuffer[i])
                        {
                        case -2:
                            ++stopTally;
                            break;
                        case 0:
                            ++offTally;
                            break;
                        case 1:
                            ++onTally;
                            break;
                        }
                    }

                    if (onTally > offTally && onTally > stopTally)
                    {
                        outputPulses.push_back(1);
                    }
                    else if (offTally > onTally && offTally > stopTally)
                    {
                        outputPulses.push_back(0);
                    }
                    else
                    {
                        outputPulses.push_back(-2);
                    }
                    
                }

                //cout << endl;
                // show new bits added
                for (int i = readIndex + 1; i < outputPulses.size(); ++i)
                {
                    //cout << outputPulses[i] << ", ";
                }
                //cout << endl;

                AudioUserData.outLength = 0;
                delete AudioUserData.outBuffer;

                //check if new character is completed
                int completeCharEnd = -1;

                if (outputPulses[readIndex] != -2)
                    for (; readIndex < outputPulses.size(); ++readIndex)
                        if (outputPulses[readIndex] == -2)
                            break;

                if (outputPulses[readIndex] == -2)
                    for (int i = readIndex+1; i < outputPulses.size(); ++i)
                    {
                        if (outputPulses[i] == -2)
                        {
                            if (outputPulses[i - 1] != -2)
                            {
                                completeCharEnd = i;
                                break;
                            }
                            else
                                readIndex = i;
                        }
                    }

                if (completeCharEnd != -1)
                {
                    //cout << endl;
                    // show new bits added
                    for (int i = readIndex + 1; i < completeCharEnd; ++i)
                    {
                        //cout << outputPulses[i] << ", ";
                    }
                    //cout << endl;

                    // convert binary to string
                    for (int j = readIndex + 1; j+8 <= completeCharEnd; j += 8)
                    {
                        char newChar = '\0';
                        for (int i = 0; i < 8; ++i)
                        {
                            if (outputPulses[j+i] == 1)
                            {
                                //newChar |= (1 << 8) >> i;
                                newChar |= 1 << i;
                            }
                        }

                        cout << newChar;
                    }

                    for(int i = outputPulses.size()-1; i >= 0; --i)
                        if (outputPulses[i] == -2)
                        {
                            readIndex = i;
                            break;
                        }
                    //readIndex = completeCharEnd;
                }
            }

            SDL_Delay(300);
        }
    }

    return 0;
}

