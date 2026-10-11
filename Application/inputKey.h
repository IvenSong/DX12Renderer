#pragma once

// Letter keys. Uppercase values apply regardless of Shift or Caps Lock.
#define KEY_A 'A'
#define KEY_B 'B'
#define KEY_C 'C'
#define KEY_D 'D'
#define KEY_E 'E'
#define KEY_F 'F'
#define KEY_G 'G'
#define KEY_H 'H'
#define KEY_I 'I'
#define KEY_J 'J'
#define KEY_K 'K'
#define KEY_L 'L'
#define KEY_M 'M'
#define KEY_N 'N'
#define KEY_O 'O'
#define KEY_P 'P'
#define KEY_Q 'Q'
#define KEY_R 'R'
#define KEY_S 'S'
#define KEY_T 'T'
#define KEY_U 'U'
#define KEY_V 'V'
#define KEY_W 'W'
#define KEY_X 'X'
#define KEY_Y 'Y'
#define KEY_Z 'Z'

// Number keys above the letters, separate from the numeric keypad.
#define KEY_0 '0'
#define KEY_1 '1'
#define KEY_2 '2'
#define KEY_3 '3'
#define KEY_4 '4'
#define KEY_5 '5'
#define KEY_6 '6'
#define KEY_7 '7'
#define KEY_8 '8'
#define KEY_9 '9'

// Common control keys. Modifier codes do not distinguish left and right.
#define KEY_BACKSPACE 0x08
#define KEY_TAB       0x09
#define KEY_ENTER     0x0D
#define KEY_SHIFT     0x10
#define KEY_CTRL      0x11
#define KEY_ALT       0x12
#define KEY_PAUSE     0x13
#define KEY_CAPS_LOCK 0x14
#define KEY_ESCAPE    0x1B
#define KEY_SPACE     0x20

// Navigation and editing keys.
#define KEY_PAGE_UP      0x21
#define KEY_PAGE_DOWN    0x22
#define KEY_END          0x23
#define KEY_HOME         0x24
#define KEY_LEFT         0x25
#define KEY_UP           0x26
#define KEY_RIGHT        0x27
#define KEY_DOWN         0x28
#define KEY_PRINT_SCREEN 0x2C
#define KEY_INSERT       0x2D
#define KEY_DELETE       0x2E

// Function keys.
#define KEY_F1  0x70
#define KEY_F2  0x71
#define KEY_F3  0x72
#define KEY_F4  0x73
#define KEY_F5  0x74
#define KEY_F6  0x75
#define KEY_F7  0x76
#define KEY_F8  0x77
#define KEY_F9  0x78
#define KEY_F10 0x79
#define KEY_F11 0x7A
#define KEY_F12 0x7B

// Numeric keypad. With Num Lock off, digit keys can send navigation codes.
#define KEY_NUMPAD_0 0x60
#define KEY_NUMPAD_1 0x61
#define KEY_NUMPAD_2 0x62
#define KEY_NUMPAD_3 0x63
#define KEY_NUMPAD_4 0x64
#define KEY_NUMPAD_5 0x65
#define KEY_NUMPAD_6 0x66
#define KEY_NUMPAD_7 0x67
#define KEY_NUMPAD_8 0x68
#define KEY_NUMPAD_9 0x69

#define KEY_NUMPAD_MULTIPLY  0x6A
#define KEY_NUMPAD_ADD       0x6B
#define KEY_NUMPAD_SEPARATOR 0x6C
#define KEY_NUMPAD_SUBTRACT  0x6D
#define KEY_NUMPAD_DECIMAL   0x6E
#define KEY_NUMPAD_DIVIDE    0x6F

// Lock keys.
#define KEY_NUM_LOCK    0x90
#define KEY_SCROLL_LOCK 0x91

// Alt combinations can arrive as WM_SYSKEYDOWN / WM_SYSKEYUP.
// Both Enter keys use KEY_ENTER; lParam's extended-key flag distinguishes them.
