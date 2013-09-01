#define LED_CONFIG		(DDRD	|= (1<<6))
#define LED_OFF			(PORTD	|= (1<<6))
#define LED_ON			(PORTD	&= ~(1<<6))

#define LED_BLUE_CONFIG	(DDRD	|= (1<<1))
#define LED_BLUE_OFF		(PORTD	&= ~(1<<1))
#define LED_BLUE_ON		(PORTD	|= (1<<1))

#define LED_RED_CONFIG	(DDRC	|= (1<<6))
#define LED_RED_OFF	(PORTC	|= (1<<6))
#define LED_RED_ON		(PORTC	&= ~(1<<6))

#define FN_KEY_ID			7*5+4

// Init ports
void init_ports(void) {
	DDRB  = 0b01001010; DDRC  = 0b00100000; DDRD  = 0b00000100;
	PORTB = 0xFF;		PORTC = 0xFF; 		PORTD = 0xFF;
}

// Pins keyboard matrix (have to properly initialize ports)
uint8_t *const	row_port[ROWS]	= { _PORTC,	_PORTD,	_PORTB,	_PORTB,	_PORTB};
const uint8_t	row_bit[ROWS]	= { (1<<5),	(1<<2),	(1<<6),	(1<<3),	(1<<1)};
uint8_t *const	col_pin[COLS]	= {_PIND,	_PIND,	_PIND,	_PINB,	_PINC,	_PIND,		_PINB,	_PINC,	_PINB,	_PINB,	_PINC,	_PINB};
const uint8_t	col_bit[COLS]	= {(1<<3),	(1<<0),	(1<<4),	(1<<0),	(1<<2),	(1<<5),		(1<<4),	(1<<4),	(1<<2),	(1<<5),	(1<<7),	(1<<7)};

/*
r5c1 r1c1 r1c2 r1c3 r1c4 r1c5 r1c6      r1c7 r1c8 r1c9 r1c10 r1c11 r1c12 r5c12
r4c1 r2c1 r2c2 r2c3 r2c4 r2c5 r2c6      r2c7 r2c8 r2c9 r2c10 r2c11 r2c12 r4c12
r4c2 r3c1 r3c2 r3c3 r3c4 r3c5 r3c6      r3c7 r3c8 r3c9 r3c10 r3c11 r3c12 r4c11
  r5c2    r5c3 r4c3 r4c4 r4c5 r4c6      r4c7 r4c8 r4c9 r4c10 r5c10    r5c11
                    r5c4 r5c5   r5c6  r5c7   r5c8 r5c9

[e][~][1][2][3][4][5]  [6][7][8][9][0][-][=]
[a][t][Q][W][E][R][T]  [Y][U][I][O][P][b][b]
[m][c][A][S][D][F][G]  [H][J][K][L][:]["][\]
  [s] [Z][X][C][V][B]  [N][M][,][.][/] [s]
*/
/*
VCC	-
C4	- Col8
C5	- 	Row1
C6	- 		SW_BLUE
C7	- Col11
B7	- Col12
B6	- 	Row3
B5	- Col10
B4	- Col7
B3	- 	Row4
B2	- Col9
B1	- 	Row5

*/

const uint8_t layer1[KEYS] = {
	//ROW1			ROW2				ROW3			ROW4			ROW5
	KEY_TILDE,		KEY_TAB,			KEY_RCTRL,		KEY_ALT_TAB,	KEY_ESC,		// COL1
	KEY_1,			KEY_Q,				KEY_A,			KEY_LGUI,		KEY_LAYER1,		// COL2
	KEY_2,			KEY_W,				KEY_S,			KEY_X,			KEY_Z,			// COL3
	KEY_3,			KEY_E,				KEY_D,			KEY_C,			KEY_LALT,		// COL4
	KEY_4,			KEY_R,				KEY_F,			KEY_V,			KEY_LCTRL,		// COL5
	KEY_5,			KEY_T,				KEY_G,			KEY_B,			KEY_MY_SHIFT,	// COL6
	KEY_6,			KEY_Y,				KEY_H,			KEY_N,			KEY_SPACE,		// COL7 
	KEY_7,			KEY_U,				KEY_J,			KEY_M,			KEY_FN,			// COL8
	KEY_8,			KEY_I,				KEY_K,			KEY_COMMA,		KEY_RALT,		// COL9
	KEY_9,			KEY_O,				KEY_L,			KEY_PERIOD,		KEY_SLASH,		// COL10
	KEY_0,			KEY_P,				KEY_SEMICOLON,	KEY_BACKSLASH,	KEY_LAYER2,		// COL11
	KEY_MINUS,		KEY_LEFT_BRACE,		KEY_QUOTE,		KEY_RIGHT_BRACE,KEY_EQUAL		// COL12
};

/*static uint16_t layer2_typo[KEYS] = {
	'§','±', 0,0, 0,0, 0,0, 0,0,
	'¡','⁄', 'œ','Œ', 'å','Å', 0,0, 0,0,
	'™','€', '∑','„', 'ß','Í', '≈','˛', 'Ω','¸',
	'£','‹', '´','´', '∂','Î', 'ç','Ç', 0,0,
	'¢','›', '®','‰', 'ƒ','Ï', '√','◊', 0,0,
	'∞','ﬁ', '†','ˇ', '©','˝', '∫','ı', 0,0,
	'§','ﬂ', '¥','Á', '˙','Ó', '˜','˜', 0,0,
	'¶','‡', '¨','¨', '∆','Ô', 'µ','Â', 0,0,
	'•','°', 'ˆ','ˆ', '˚','', '≤','¯', 0,0,
	'ª','·', 'ø','Ø', '¬','Ò', '≥','˘', '÷','¿',
	'º','‚', 'π','∏', '…','Ú', '«','»', 0,0,
	'–','—', '“','”', 'æ','Æ', '‘','’', '≠','±'
};


static uint16_t layer1_typo[KEYS] = {
	'§','±', 0,0, 0,0, 0,0, 0,0,
	'!','|', 'ј','Ј', 'ƒ','ƒ', 0,0, 0,0,
	'@','"', 'џ','Џ', 'ы','Ы', '≈','≈', 'ђ','Ђ',
	'#','£', 'ќ','Ќ', 'ћ','Ћ', '≠','≠', 0,0,
	'$','€', '®','®', '÷','÷', 'µ','µ', 0,0,
	'%','∞', '†','†', '©','©', 'и','И', 0,0,
	'^','¬', 'њ','Њ', '}','{', '™','™', 0,0,
	'&','¶', 'ѓ','Ѓ', '°','•', '~','~', 0,0,
	'*','√', 'ѕ','Ѕ', 'љ','Љ', '≤','<', 0,0,
	'(',"'", 'ў','Ў', '∆','∆', '≥','>', '“','„',
	')','`', '‘','/’', '…','…', 'ё','Ё', 0,0,
	'–','—', '“','”', 'э','Э', 'ъ','Ъ', '»','«'
};*/


/*
[e][~][1][2][3][4][5]  [6][7][8][9][0][-][=]
[a][t][Q][W][E][R][T]  [Y][U][I][O][P][b][b]
[m][c][A][S][D][F][G]  [H][J][K][L][:]["][\]
  [s] [Z][X][C][V][B]  [N][M][,][.][/] [s]
            [a][c][s]  [s][f][a]
r5c1 r1c1 r1c2 r1c3 r1c4 r1c5 r1c6      r1c7 r1c8 r1c9 r1c10 r1c11 r1c12 r5c12
r4c1 r2c1 r2c2 r2c3 r2c4 r2c5 r2c6      r2c7 r2c8 r2c9 r2c10 r2c11 r2c12 r4c12
r4c2 r3c1 r3c2 r3c3 r3c4 r3c5 r3c6      r3c7 r3c8 r3c9 r3c10 r3c11 r3c12 r4c11
  r5c2    r5c3 r4c3 r4c4 r4c5 r4c6      r4c7 r4c8 r4c9 r4c10 r5c10    r5c11
                    r5c4 r5c5   r5c6  r5c7   r5c8 r5c9

CatBoard v3:
r5c1 r1c1 r1c2 r1c3 r1c4 r1c5 r1c6      r1c7 r1c8 r1c9 r1c10 r1c11 r1c12 r5c10
r5c2 r2c1 r2c2 r2c3 r2c4 r2c5 r2c6      r2c7 r2c8 r2c9 r2c10 r2c11 r2c12 r5c11
r5c3 r3c1 r3c2 r3c3 r3c4 r3c5 r3c6      r3c7 r3c8 r3c9 r3c10 r3c11 r3c12 r5c12
  r4c1    r4c2 r4c3 r4c4 r4c5 r4c6      r4c7 r4c8 r4c9 r4c10 r4c11    r4c12
                    r5c4 r5c5   r5c6  r5c7   r5c8 r5c9

*/

const uint8_t layer_fn[KEYS] = {
	//ROW1				ROW2			ROW3			ROW4			ROW5
	KEY_PRINTSCREEN,	KEY_TAB,		KEY_RCTRL,		KEY_ALT_TAB,	KEY_TURBO_REPEAT,	// COL1
	KEY_F1,				NULL,			NULL,			KEY_LGUI,		KEY_LAYER1,			// COL2
	KEY_F2,				NULL,			NULL,			NULL,			NULL,				// COL3
	KEY_F3,				NULL,			NULL,			NULL,			KEY_LALT,			// COL4
	KEY_F4,				NULL,			NULL,			NULL,			KEY_LCTRL,			// COL5
	KEY_F5,				KEY_TILDE,		NULL,			NULL,			KEY_MY_SHIFT,		// COL6
	KEY_F6,				KEY_LED,		KEY_ENTER,		KEY_BACKSPACE,	KEY_OS_MODE,		// COL7
	KEY_F7,				KEY_HOME,		KEY_LEFT,		KEY_DELETE,		KEY_FN,				// COL8
	KEY_F8,				KEY_UP,			KEY_DOWN,		KEY_INSERT,		KEY_FN_LOCK,		// COL9
	KEY_F9,				KEY_END,		KEY_RIGHT,		NULL,			NULL,				// COL10
	KEY_F10,			KEY_PAGE_UP,	KEY_PAGE_DOWN,	NULL,			KEY_LOCK,			// COL11
	KEY_F11,			KEY_ESC,		KEY_PAUSE,		KEY_SCROLL_LOCK,KEY_F12				// COL12
};

const uint8_t layer_fnlock[KEYS] = {
	//ROW1				ROW2			ROW3			ROW4			ROW5
	KEY_TILDE,			KEY_TAB,		KEY_RCTRL,		KEY_ALT_TAB,	KEY_TURBO_REPEAT,	// COL1
	KEY_1,				KEY_PAGE_UP,	KEY_PAGE_DOWN,	KEY_LGUI,		KEY_LAYER1,			// COL2
	KEY_2,				KEY_HOME,		KEY_LEFT,		KEY_MACRO,		KEY_MACRO,			// COL3
	KEY_3,				KEY_UP,			KEY_DOWN,		KEY_MACRO,		KEY_LALT,			// COL4
	KEY_4,				KEY_END,		KEY_RIGHT,		KEY_MACRO,		KEY_LCTRL,			// COL5
	KEY_5,				KEY_TILDE,		KEY_ENTER,		KEY_MACRO,		KEY_MY_SHIFT,		// COL6
	KEY_6,				KEYPAD_SLASH,	KEYPAD_ASTERIX,	KEYPAD_0,		KEY_SPACE,			// COL7
	KEY_7,				KEYPAD_7,		KEYPAD_4,		KEYPAD_1,		KEY_FN,				// COL8
	KEY_8,				KEYPAD_8,		KEYPAD_5,		KEYPAD_2,		KEY_RALT,			// COL9
	KEY_9,				KEYPAD_9,		KEYPAD_6,		KEYPAD_3,		NULL,				// COL10
	KEY_0,				KEYPAD_MINUS,	KEYPAD_PLUS,	KEYPAD_PERIOD,	KEY_LAYER2,			// COL11
	KEY_MINUS,			KEY_ESC,		KEY_ENTER,		KEY_NUM_LOCK,	KEY_EQUAL			// COL12
};
