/*
 * main.c
 * LulSynth
 *
 * Created by Allek Mott on 12/29/14.
 * Copyright (c) 2014 Allek Mott. All rights reserved.
 */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>

#include "waves.h"
#include "portaudio.h"

#define SAMPLE_RATE     (44100)
#define VOLUME          (0.5f)
#define FRAMES_PER_BUF  (256)

/* number of current sample (last sample generated) */
int sample_num = 0;

/* fat buffer containing a second's worth of samples
 * used in callback function
 */
float *sample_buffer;

/* number of initial sample in buffer */
float buf_init_sample_num;

/* ghetto input variable */
float a = 440.0f;

/* pointer to current wave function
 * If input designates a change in wave type (triangle, square, etc),
 * wave pointer will be reassigned to corresponding wave function.
 */
float (*wave) (float) = &sine;

/* Data structure to contain stereo sample data */
typedef struct
{
    float left_phase;
    float right_phase;
}
sampleData;


/* Check for portaudio error */
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

/* Thread to grab textual input */
void *grab_input(void *arg) {
    printf("I'm the input thread!\n");
    char input[30];
    while (1) {
        fgets(input, 30, stdin);
        
        switch (input[0]) {
            case 'a': /* sine */
                wave = &sine;
                printf("Sine wave!\n");
                continue;
            case 'b': /* triangle */
                wave = &triangle;
                printf("Triangle wave!\n");
                continue;
            case 'c': /* square */
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


/* Generate 1s worth of samples and throw them all into
 * a nice, pretty buffer.
 *
 * @param init_sampleno sample number represented by index 0 in
 *        new buffer
 */
float *gen_buf(int init_sampleno) {
    return NULL;
}


/* Buffering for audio output.
 * NEW MODEL:
 * generate fat buffer (1 second worth)
 * keep track of position of buffer in time
 * when callback has almost exhausted buffer,
 *  - generate new one behind the scenes, with
 *    sufficient overlap to scrap old buffer
 * if input vars change:
 *  - generate new buffer, leave old alone until
 *    new is generated
 */
void *buffer_dat_shiz(void *arg) {
    printf("I'm the buffer thread!\n");
    
    /* Allocate 1s worth of sample memory, starting from t = 0 */
    sample_buffer = gen_buf(buf_init_sample_num = 0);
    
    float last_a = a;
    void *last_wave = wave;
    
    while (1) {
        if (a != last_a || wave != last_wave) { /* input var changed */
            last_a = a; last_wave = wave;
            float *new_buffer = gen_buf(buf_init_sample_num);
            free(sample_buffer);
            sample_buffer = new_buffer;
        } else if (sample_num > (buf_init_sample_num + (SAMPLE_RATE / 2))) { /* 1/2 s through buffer */
            float *new_buffer = gen_buf(buf_init_sample_num = (buf_init_sample_num + (SAMPLE_RATE / 2)));
            free(sample_buffer);
            sample_buffer = new_buffer;
        }
        usleep(250000);
    }
    
    return NULL;
}

static int synth_callback(const void *inbuf,
                          void *outbuf,
                          unsigned long fpb, /* frames per buffer */
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
        
        float t = ((float) ++sample_num / SAMPLE_RATE);
        float finput = (sin(a * t) + .5f * sin(.5f * a * t + t) + .25f * sin(.25f * a * t)) / 2.0f;
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
    
    /* init sample data structure */
    sample.left_phase = sample.right_phase = 0.0f;
    
    /* init PortAudio library */
    err = Pa_Initialize();
    pa_errcheck(&err);
    
    /* open stream */
    err = Pa_OpenDefaultStream(&stream,         /* stream pointer               */
                               0,               /* no input channels            */
                               2,               /* stereo (2 output channels)   */
                               paFloat32,       /* 32bits per sample            */
                               SAMPLE_RATE,     /* sample rate                  */
                               FRAMES_PER_BUF,  /* frames per buffer            */
                               synth_callback,  /* callback function            */
                               &sample);        /* sample data structure        */
    pa_errcheck(&err);
    
    /* begin writing to stream */
    err = Pa_StartStream(stream);
    pa_errcheck(&err);
    
    pthread_t inputThread;
    pthread_create(&inputThread, NULL, grab_input, NULL);
    pthread_join(inputThread, NULL);
    
    /* close up portaudio */
    err = Pa_StopStream(stream);
    pa_errcheck(&err);
    
    err = Pa_CloseStream(stream);
    pa_errcheck(&err);
    
    Pa_Terminate();
    printf("All done.\n");
    
    /* exit input thread (hopefully) */
    pthread_exit(NULL);
}