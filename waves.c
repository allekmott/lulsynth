/*  waves.c
 *  LulSynth
 *
 *  Created by Allek Mott on 2/23/15.
 *  Copyright (c) 2015 Allek Mott. All rights reserved.
 */

#include "waves.h"
#include <math.h>

#define MULTIPLIER 2 * M_PI

float sine(float input) {
    return sin(input * MULTIPLIER);
}

float square(float input) {
    if (sine(input) > 0.0f)
        return 1.0f;
    else if (sine(input < 0.0f))
        return -1.0f;
    else
        return 0.0f;
}

/* pi / 2 -> 3 pi / 2:
 * y = -x + 1
 *
 * - pi / 2 -> pi /2:
 * y = x - 1
 */
float triangle(float input) {;
    float dec = input - (float) ((int) input);
    float output;
    if (sine(input) > 0.0f) {
        /* positive half */
        if (dec <= 0.50f)
            output = 0.0f + dec;
        else
            output = 1.0f - dec;
    } else if (sine(input) < 0.0f) {
        if (dec <= 0.50f)
            output = 0.0f - dec;
        else
            output = -1.0f + dec;
    } else {
        return 0.0f;
    }
    return 2.0f * output;
}