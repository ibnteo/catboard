#define LED_CONFIG		(DDRD	|= (1<<6))
#define LED_OFF			(PORTD	|= (1<<6))
#define LED_ON			(PORTD	&= ~(1<<6))

#define LED_BLUE_CONFIG	(DDRD	|= (1<<1))
#define LED_BLUE_OFF		(PORTD	&= ~(1<<1))
#define LED_BLUE_ON		(PORTD	|= (1<<1))

#define LED_RED_CONFIG	(DDRC	|= (1<<6))
#define LED_RED_OFF	(PORTC	|= (1<<6))
#define LED_RED_ON		(PORTC	&= ~(1<<6))

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