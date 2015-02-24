//
//  waves.c
//  LulSynth
//
//  Created by Allek Mott on 2/23/15.
//  Copyright (c) 2015 Allek Mott. All rights reserved.
//

#include "waves.h"
#include <math.h>

float sine(float input) {
    return sin(input * M_PI / 180);
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
float triangle(float input) {
    float degrees = input * M_PI / 180;
    float pit_dec = degrees - (float) ((int) degrees);
    float output;
    if (sine(input) > 0.0f) {
        // positive half
        if (pit_dec <= 0.50f)
            output = 0.0f + pit_dec;
        else
            output = 1.0f - pit_dec;
    } else if (sine(input) < 0.0f) {
        if (pit_dec <= 0.50f)
            output = 0.0f - pit_dec;
        else
            output = -1.0f + pit_dec;
    } else {
        return 0.0f;
    }
    return 2.0f * output;
}