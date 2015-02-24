//
//  main.c
//  LulSynth
//
//  Created by Allek Mott on 12/29/14.
//  Copyright (c) 2014 Allek Mott. All rights reserved.
//

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>

#include "waves.h"
#include "portaudio.h"

#define SAMPLE_RATE   (44100)
#define VOLUME        (1.0f);

int sample_num = 0;
float a = 0.0f;
float (*wave) (float) = &sine;

typedef struct
{
    float left_phase;
    float right_phase;
}
sampleData;


/* Output error message and GTFO (kill pa) */
void pa_errcheck(PaError *err) {
    if (*err == paNoError) {
        return;
    } else {
        Pa_Terminate();
        fprintf(stderr,
                "Apparently portaudio decided to suck.\n"\
                "ErrNo: %d\n"\
                "Message: %s\n",
                *err,
                Pa_GetErrorText(*err));
        exit(*err);
    }
}


void *grab_input(void *arg) {
    printf("I'm the input thread!\n");
    char input[30];
    while (1) {
        fgets(input, 30, stdin);
        
        switch (input[0]) {
            case 'a': // sine
                wave = &sine;
                printf("Sine wave!\n");
                continue;
            case 'b': // triangle
                wave = &triangle;
                printf("Triangle wave!\n");
                continue;
            case 'c': // square
                wave = &square;
                printf("Square wave!\n");
                continue;
        }
        
        float input_f = atof(input);
        if (input_f == NAN)
            input_f = 0;
        a = atof(input);
    }
}

static int synth_callback(const void *inbuf,
                          void *outbuf,
                          unsigned long fpb, // framse per buffer
                          const PaStreamCallbackTimeInfo *time_info,
                          PaStreamCallbackFlags callback_flags,
                          void *usample) {
    
    sampleData *sample = (sampleData*) usample;
    float *out = (float*) outbuf;
    unsigned int i;
    (void) inbuf;
    
    for (i=0; i<fpb; i++) {
        *out++ = sample->left_phase;
        *out++ = sample->right_phase;
        
        float t = ((float) ++sample_num * fpb / SAMPLE_RATE);
        float finput = a * t;
        sample->left_phase = sample->right_phase = wave(finput);
        
        sample->left_phase *= VOLUME;
        sample->right_phase *= VOLUME;
    }
    
    return 0;
}

static sampleData sample;
int main(int argc, char *argv[]) {
    PaStream *stream;
    PaError err;
    
    srand((unsigned int) time(NULL));
    printf("Initializing PortAudio\n");
    
    // init sample data structure
    sample.left_phase = sample.right_phase = 0.0f;
    
    // init PortAudio library
    err = Pa_Initialize();
    pa_errcheck(&err);
    
    // open stream
    err = Pa_OpenDefaultStream(&stream,         // stream pointer
                               0,               // no input channels
                               2,               // stereo (2 output channels)
                               paFloat32,       // 32bits per sample
                               SAMPLE_RATE,     // sample rate
                               256,             // frames per buffer
                               synth_callback,  // callback function
                               &sample);        // sample data structure
    pa_errcheck(&err);
    
    // begin writing to stream
    err = Pa_StartStream(stream);
    pa_errcheck(&err);
    
    pthread_t inputThread;
    pthread_create(&inputThread, NULL, grab_input, NULL);
    pthread_join(inputThread, NULL);
    
    // close up portaudio
    err = Pa_StopStream(stream);
    pa_errcheck(&err);
    
    err = Pa_CloseStream(stream);
    pa_errcheck(&err);
    
    Pa_Terminate();
    printf("All done.\n");
    
    // exit input thread
    pthread_exit(NULL);
}