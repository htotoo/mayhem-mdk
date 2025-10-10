#pragma once

#include "standalone_application.hpp"

// Positioning helpers PER CHARACTERS (8*16)
// EACH parameters must be used az CHAR position, not PX coordinates. So If you wanna use the 8,16 coordinates (that is the second character in X and second character in Y you must use UI_POS_X(1) UI_POS_Y(1) (since we count from 0)

// default font height
#define UI_POS_DEFAULT_HEIGHT 16
// small height
#define UI_POS_DEFAULT_HEIGHT_SMALL 8
// default font width
#define UI_POS_DEFAULT_WIDTH 8
// px position of the linenum-th character (Y)
#define UI_POS_Y(linenum) ((int)((linenum) * UI_POS_DEFAULT_HEIGHT))
// px position of the linenum-th character from the bottom of the screen (Y) (please calculate the +1 line top-bar to it too if that is visible!)
#define UI_POS_Y_BOTTOM(linenum) ((int)((*_api->screen_height) - (linenum) * UI_POS_DEFAULT_HEIGHT))
// px position of the linenum-th character from the left of the screen (X)
#define UI_POS_X(charnum) ((int)((charnum) * UI_POS_DEFAULT_WIDTH))
// px position of the linenum-th character from the right of the screen (X)
#define UI_POS_X_RIGHT(charnum) ((int)((*_api->screen_width) - ((charnum) * UI_POS_DEFAULT_WIDTH)))
// px position of the left character from the center of the screen (X)  (for N character wide string)
#define UI_POS_X_CENTER(charnum) ((int)(((*_api->screen_width) / 2) - ((charnum) * UI_POS_DEFAULT_WIDTH / 2)))
// px width of N characters
#define UI_POS_WIDTH(charnum) ((int)((charnum) * UI_POS_DEFAULT_WIDTH))
// px width of the screen
#define UI_POS_MAXWIDTH ((*_api->screen_width))
// px height of N line
#define UI_POS_HEIGHT(linecount) ((int)((linecount) * UI_POS_DEFAULT_HEIGHT))
// px height of the screen's percent
#define UI_POS_HEIGHT_PERCENT(percent) ((int)((*_api->screen_height) * (percent) / 100))
// remaining px from the linenum-th line to the bottom of the screen. (please calculate the +1 line top-bar to it too if that is visible!)
#define UI_POS_HEIGHT_REMAINING(linenum) ((int)((*_api->screen_height) - ((linenum) * UI_POS_DEFAULT_HEIGHT)))
// remaining px from the charnum-th character to the right of the screen
#define UI_POS_WIDTH_REMAINING(charnum) ((int)((*_api->screen_width) - ((charnum) * UI_POS_DEFAULT_WIDTH)))
// px width of the screen
#define UI_POS_MAXHEIGHT ((*_api->screen_height))
