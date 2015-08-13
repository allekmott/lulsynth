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
#include <time.h>

#include "waves.h"
#include "portaudio.h"

#define SAMPLE_RATE     (44100)
#define VOLUME          (0.5f)
#define FRAMES_PER_BUF  (256)

int sample_num = 0;
float a = 440.0f;
float (*wave) (float) = &sine;

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

// number of current callback buffer
float cur_bufferno = 0;


float *sample_buffer;
float buffer_nos[] = {0, 0, 0, 0, 0};

/* Returns maximum buffer number currently in sample buffer.
 */
int max_buf_no() {
    int i,
        max = 0;
    for (i = 0; i < 5; i++) {
        if (sample_buffer[i] > max)
            max = sample_buffer[i];
    }
    return max;
}

/* Returns index of provided buffer number in sample buffer.
 */
int index_of_buffer(int buffer_no) {
    int x;
    for (x = 0; x < 5; x++) {
        if (buffer_nos[x] == buffer_no)
            return x;
    }
    return -1;
}

/* Returns the number of used buffers present in the sample buffer.
 */
int num_old_buffers() {
    int x,
    old = 0;
    for (x = 0; x < 5; x++) {
        if (buffer_nos[x] < cur_bufferno)
            old++;
    }
    return old;
}



/* Buffering for audio output.
 * Buffer tries to maintain "one second ahead rule".
 * Upon change in any value, flushes buffer and regenerates.
 *
 * So how this works:
 * Buffer leads callback by around 4 callback buffers
 * (FRAMES_PER_BUF samples).
 * Ex)
 * sample_buffer: [frame1*, frame2, frame3, frame4, frame5]
 * * = active (being read, or due to be read by callback first)
 * buffer generates frames 2-5 and then sleeps for ~ 2 frames.
 *
 * Then:
 * sample_buffer: [frame1, frame2, frame3*, frame4, frame5]
 * frames 1 & 2 are now useless, so lets generate new frames in
 * place of those;
 * sample_buffer: [frame6, frame7, frame3*, frame4, frame5]
 * now leading by about 4. sleep for 4/2
 *
 * Woah!:
 * sample_buffer: [frame6, frame7, frame3, frame4, frame5*]
 * frames 3 and 4 are now useless; generate new frames in place
 * of them, and sleep.
 */
void *buffer_dat_shiz(void *arg) {
    printf("I'm the buffer thread!\n");
    
    sample_buffer = malloc(sizeof(float) * FRAMES_PER_BUF * 5);
    
    // generate inital sample frames
    int latest_bufferno;
    for (latest_bufferno = 0; latest_bufferno < 5; latest_bufferno++) {
        int sample_no;
        for (sample_no = 0; sample_no < FRAMES_PER_BUF; sample_no++) {
            float t = (float) ((float) ((FRAMES_PER_BUF * latest_bufferno) + sample_no)) / SAMPLE_RATE;
            sample_buffer[(FRAMES_PER_BUF * latest_bufferno) + sample_no] = wave(a * t);
            buffer_nos[latest_bufferno] = latest_bufferno;
        }
    }
    
    while (1) {
        // TODO add maths for calculating position of sample in time
        
        int num_old = num_old_buffers(),
            cur_idx = index_of_buffer(cur_bufferno),
            init_max = max_buf_no();
        // if old buffers, replace dat shiz
        if (num_old) {
            if (cur_idx == 0) {
                /* current is 0, old start at index (end - num_old) */
                int i;
                for (i = 0; i < num_old; i++) {
                    int newbuf_idx = 4 - num_old + i;
                    int newbuf_no = init_max + i;
                    void (*gen_buf) (int, int);
                    gen_buf(newbuf_idx, newbuf_no);
                }
            }
        }
    }
}

static int synth_callback(const void *inbuf,
                          void *outbuf,
                          unsigned long fpb, // frames per buffer
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
    
    cur_bufferno++;
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
                               FRAMES_PER_BUF,             // frames per buffer
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