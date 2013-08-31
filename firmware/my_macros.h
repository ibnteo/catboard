const uint8_t macros_pos[] =   {1*5+5, 2*5+4, 3*5+4, 4*5+4, 5*5+4}; // Buttons positions (Col-1)*5+Row
const uint8_t macros1[] = {KEY_F1, NULL};
const uint8_t macros2[] = {KEY_F2, NULL};
const uint8_t macros3[] = {KEY_F3, NULL};
const uint8_t macros4[] = {KEY_F4, NULL};
const uint8_t macros5[] = {
	KEY_C, KEY_SHIFT,
	KEY_A, NULL,
	KEY_T, NULL,
	KEY_B, KEY_SHIFT,
	KEY_O, NULL,
	KEY_A, NULL,
	KEY_R, NULL,
	KEY_D, NULL
};
const uint16_t macros_press[] = {&macros1, &macros2, &macros3, &macros4, &macros5};
