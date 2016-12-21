/*
 * Copyright (c) 2016, OpenAV Productions,
 * Harry van Haaren <harryhaaren@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef OPENAV_CTLR_NI_KONTROL_D2_H
#define OPENAV_CTLR_NI_KONTROL_D2_H

#if 0
enum NI_KONTROL_D2_LEDS {
	NI_KONTROL_D2_LED_FX_SELECT = 0,
	NI_KONTROL_D2_LED_FX_BTN_1,
	NI_KONTROL_D2_LED_FX_BTN_2,
	NI_KONTROL_D2_LED_FX_BTN_3,
	NI_KONTROL_D2_LED_FX_BTN_4,
	NI_KONTROL_D2_LED_SCREEN_LEFT_1,
	NI_KONTROL_D2_LED_SCREEN_LEFT_2,
	NI_KONTROL_D2_LED_SCREEN_LEFT_3,
	NI_KONTROL_D2_LED_SCREEN_LEFT_4,
	NI_KONTROL_D2_LED_SCREEN_RIGHT_1,
	NI_KONTROL_D2_LED_SCREEN_RIGHT_2,
	NI_KONTROL_D2_LED_SCREEN_RIGHT_3,
	NI_KONTROL_D2_LED_SCREEN_RIGHT_4,
	NI_KONTROL_D2_LED_BACK,
	NI_KONTROL_D2_LED_CAPTURE,
	NI_KONTROL_D2_LED_EDIT,
	NI_KONTROL_D2_LED_TRACK_ON_1,
	NI_KONTROL_D2_LED_TRACK_ON_2,
	NI_KONTROL_D2_LED_TRACK_ON_3,
	NI_KONTROL_D2_LED_TRACK_ON_4,
	NI_KONTROL_D2_LED_HOTQUEUE,
	NI_KONTROL_D2_LED_LOOP,
	NI_KONTROL_D2_LED_FREEZE,
	NI_KONTROL_D2_LED_REMIX,
	NI_KONTROL_D2_LED_FLUX,
	NI_KONTROL_D2_LED_DECK,
	NI_KONTROL_D2_LED_SHIFT,
	NI_KONTROL_D2_LED_SYNC,
	NI_KONTROL_D2_LED_CUE,
	NI_KONTROL_D2_LED_PLAY,
};

enum NI_KONTROL_D2_CONTROLS {
	NI_KONTROL_FX_BTN_UPPER_1 = 0, /* Left */
	NI_KONTROL_FX_BTN_UPPER_2 = 1,
	NI_KONTROL_FX_BTN_UPPER_3 = 2,
	NI_KONTROL_FX_BTN_UPPER_4 = 3,
	NI_KONTROL_FX_BTN_LOWER_1 = 4, /* Left */
	NI_KONTROL_FX_BTN_LOWER_2 = 5,
	NI_KONTROL_FX_BTN_LOWER_3 = 6,
	NI_KONTROL_FX_BTN_LOWER_4 = 7,
	NI_KONTROL_FX_BTN_COUNT,
	NI_KONTROL_D2_CONTROLS,
};

+	// GRID light system
+	1-24   pads
+
+	// individual lights
+	25     fx select
+	26-29  fx btn 1 to 4
+	30-33  screen left 1 to 4
+	34-37  screen right 1 to 4
+	38 back
+	39 cap
+	40 edit
+	41-44 track on 1 to 4
+
+	45 hotque(white)
+	46     (blue)
+	47,48 loop w/b
+	49 50 freeze w/b
+	51 52 Remix
+	53 Flux
+	54 55 Deck
+	56 Shift
+	57 sync (green)
+	58 sync (red)
+	59 cue
+	60 play
+
+	61-64 loop surround white
+	65-68 loop surround blue
+
+	// Grid system two
+	69-94: 25 touchstrip leds blue left to right
+	94-+25: 25 touchstrip leds orange left to right
+
+	119 - 122 decks a b c d
+	*/

#endif

enum NI_KONTROL_D2_LEDS {
	/* Left vol LEDs, first 5 blue, last 2 orange */
	NI_KONTROL_D2_LED_LEVEL_L1 = 1,
	NI_KONTROL_D2_LED_LEVEL_L2 = 2,
	NI_KONTROL_D2_LED_LEVEL_L3 = 3,
	NI_KONTROL_D2_LED_LEVEL_L4 = 4,
	NI_KONTROL_D2_LED_LEVEL_L5 = 5,
	NI_KONTROL_D2_LED_LEVEL_L6 = 6,
	NI_KONTROL_D2_LED_LEVEL_L7 = 7,

	/* Right vol LEDs, first 5 blue, last 2 orange */
	NI_KONTROL_D2_LED_LEVEL_R1 = 8,
	NI_KONTROL_D2_LED_LEVEL_R2 = 9,
	NI_KONTROL_D2_LED_LEVEL_R3 = 10,
	NI_KONTROL_D2_LED_LEVEL_R4 = 11,
	NI_KONTROL_D2_LED_LEVEL_R5 = 12,
	NI_KONTROL_D2_LED_LEVEL_R6 = 13,
	NI_KONTROL_D2_LED_LEVEL_R7 = 14,

	NI_KONTROL_D2_LED_CUE_A = 15,
	NI_KONTROL_D2_LED_CUE_B = 16,

	/* FX ON supports Orange and Blue colours */
	NI_KONTROL_D2_LED_FX_ON_LEFT = 17,	/* orange */
	NI_KONTROL_D2_LED_UNUSED_1 = 18,	/* blue */

	NI_KONTROL_D2_LED_MODE = 19,

	/* FX ON supports Orange and Blue colours */
	NI_KONTROL_D2_LED_FX_ON_RIGHT = 20,	/* orange */
	NI_KONTROL_D2_LED_UNUSED_2 = 21,	/* blue */

	NI_KONTROL_D2_LED_COUNT,
};

enum NI_KONTROL_D2_CONTROLS {
	NI_KONTROL_D2_SLIDER_LEFT_GAIN,
	/* FX */
	NI_KONTROL_D2_BTN_DECK_A,
	NI_KONTROL_D2_BTN_DECK_B,
	NI_KONTROL_D2_BTN_DECK_C,
	NI_KONTROL_D2_BTN_DECK_D,
	NI_KONTROL_D2_BTN_FX_1,
	NI_KONTROL_D2_BTN_FX_2,
	NI_KONTROL_D2_BTN_FX_3,
	NI_KONTROL_D2_BTN_FX_4,
	NI_KONTROL_D2_BTN_FX_DIAL_TOUCH_1,
	NI_KONTROL_D2_BTN_FX_DIAL_TOUCH_2,
	NI_KONTROL_D2_BTN_FX_DIAL_TOUCH_3,
	NI_KONTROL_D2_BTN_FX_DIAL_TOUCH_4,
	NI_KONTROL_D2_BTN_FX_SELECT,
	/* Screen */
	NI_KONTROL_D2_BTN_SCREEN_LEFT_1,
	NI_KONTROL_D2_BTN_SCREEN_LEFT_2,
	NI_KONTROL_D2_BTN_SCREEN_LEFT_3,
	NI_KONTROL_D2_BTN_SCREEN_LEFT_4,
	NI_KONTROL_D2_BTN_SCREEN_RIGHT_1,
	NI_KONTROL_D2_BTN_SCREEN_RIGHT_2,
	NI_KONTROL_D2_BTN_SCREEN_RIGHT_3,
	NI_KONTROL_D2_BTN_SCREEN_RIGHT_4,
	NI_KONTROL_D2_BTN_SCREEN_ENCODER_TOUCH_1,
	NI_KONTROL_D2_BTN_SCREEN_ENCODER_TOUCH_2,
	NI_KONTROL_D2_BTN_SCREEN_ENCODER_TOUCH_3,
	NI_KONTROL_D2_BTN_SCREEN_ENCODER_TOUCH_4,
	NI_KONTROL_D2_BTN_ENCODER_BROWSE_PRESS,
	NI_KONTROL_D2_BTN_ENCODER_BROWSE_TOUCH,
	NI_KONTROL_D2_BTN_BACK,
	NI_KONTROL_D2_BTN_CAPTURE,
	NI_KONTROL_D2_BTN_EDIT,
	NI_KONTROL_D2_BTN_ENCODER_LOOP_PRESS,
	NI_KONTROL_D2_BTN_ENCODER_LOOP_TOUCH,
	/* Performance */
	NI_KONTROL_D2_BTN_ON_1,
	NI_KONTROL_D2_BTN_ON_2,
	NI_KONTROL_D2_BTN_ON_3,
	NI_KONTROL_D2_BTN_ON_4,
	NI_KONTROL_D2_BTN_FADER_TOUCH_1,
	NI_KONTROL_D2_BTN_FADER_TOUCH_2,
	NI_KONTROL_D2_BTN_FADER_TOUCH_3,
	NI_KONTROL_D2_BTN_FADER_TOUCH_4,
	NI_KONTROL_D2_BTN_PAD_1,
	NI_KONTROL_D2_BTN_PAD_2,
	NI_KONTROL_D2_BTN_PAD_3,
	NI_KONTROL_D2_BTN_PAD_4,
	NI_KONTROL_D2_BTN_PAD_5,
	NI_KONTROL_D2_BTN_PAD_6,
	NI_KONTROL_D2_BTN_PAD_7,
	NI_KONTROL_D2_BTN_PAD_8,
	/* Global */
	NI_KONTROL_D2_BTN_HOTCUE,
	NI_KONTROL_D2_BTN_LOOP,
	NI_KONTROL_D2_BTN_FREEZE,
	NI_KONTROL_D2_BTN_REMIX,
	NI_KONTROL_D2_BTN_FLUX,
	NI_KONTROL_D2_BTN_DECK,
	/* Playback */
	NI_KONTROL_D2_BTN_SHIFT,
	NI_KONTROL_D2_BTN_SYNC,
	NI_KONTROL_D2_BTN_CUE,
	NI_KONTROL_D2_BTN_PLAY,
	/* Sliders */
	NI_KONTROL_D2_SLIDER_SCREEN_ENCODER_1,
	NI_KONTROL_D2_SLIDER_SCREEN_ENCODER_2,
	NI_KONTROL_D2_SLIDER_SCREEN_ENCODER_3,
	NI_KONTROL_D2_SLIDER_SCREEN_ENCODER_4,
	NI_KONTROL_D2_SLIDER_FADER_1,
	NI_KONTROL_D2_SLIDER_FADER_2,
	NI_KONTROL_D2_SLIDER_FADER_3,
	NI_KONTROL_D2_SLIDER_FADER_4,
	NI_KONTROL_D2_SLIDER_FX_DIAL_1,
	NI_KONTROL_D2_SLIDER_FX_DIAL_2,
	NI_KONTROL_D2_SLIDER_FX_DIAL_3,
	NI_KONTROL_D2_SLIDER_FX_DIAL_4,
	/* Count of all controls */
	NI_KONTROL_D2_CONTROLS_COUNT
};

#endif /* OPENAV_CTLR_NI_KONTROL_D2_H */

