//
//  waves.h
//  LulSynth
//
//  Created by Allek Mott on 2/23/15.
//  Copyright (c) 2015 Allek Mott. All rights reserved.
//

#ifndef __LulSynth__waves__
#define __LulSynth__waves__

#include <stdio.h>

/* Because laziness.
 * Sine of input in DEGREES (library function takes rads)
 */
float sine(float input);

/* Models square wave from input time.
 * 
 * Genral structure:
 * Positive end of sine wave = positive output (+1)
 * Negative end of sine wave = negative output (-1)
 */
float square(float input);

/* Models triangle wave for input time.
 *
 *
 */
float triangle(float input);

#endif /* defined(__LulSynth__waves__) */
