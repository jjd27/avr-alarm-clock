// Based on AVR134

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

typedef struct {
	unsigned char s;
	unsigned char m;
	unsigned char h;
	unsigned char d;
} time;

time t;

char NOW_H = 22;
char NOW_M = 32;
char NOW_S = 30;
char NOW_D = 3; // sun = 0; sat = 6

/* weekday start time */
char START_H = 6;
char START_M = 0;
char START_S = 0;

/* weekend start time */
char WE_START_H = 6;
char WE_START_M = 0;
char WE_START_S = 0;

/* weekday wake time */
char WAKE_H = 6;
char WAKE_M = 45;
char WAKE_S = 0;

/* weekend wake time */
char WE_WAKE_H = 6;
char WE_WAKE_M = 45;
char WE_WAKE_S = 0;

/* weekday off time */
char OFF_H = 7;
char OFF_M = 15;
char OFF_S = 0;

/* weekend off time */
char WE_OFF_H = 7;
char WE_OFF_M = 15;
char WE_OFF_S = 0;

char NUM_COUNT = 15;

char pow(char base, char exp) {
	char res = base;
	char i;
	for (i=0; i<exp; i++) {
		res = res * base;
	}
	return res;
}

char tick = 0;

#define UNARY(x) (pow(2, (x)-1) - 1)

#define SECS(h, m, s) (h*3600UL + m*60UL + s)

#define WE(d) (d==0 || d==6)

void update_leds() {
	//PORTA = t.s & 0xFF;

	// 'int' has max 32768, but we need to represent up to 86400
	unsigned long now = SECS(t.h, t.m, t.s);
	unsigned long START = WE(t.d) ? SECS(WE_START_H, WE_START_M, WE_START_S) : SECS(START_H, START_M, START_S);
	unsigned long WAKE  = WE(t.d) ? SECS(WE_WAKE_H,  WE_WAKE_M,  WE_WAKE_S)  : SECS(WAKE_H,  WAKE_M,  WAKE_S);
	unsigned long OFF   = WE(t.d) ? SECS(WE_OFF_H,   WE_OFF_M,   WE_OFF_S)   : SECS(OFF_H,   OFF_M,   OFF_S);

	if (now >= WAKE && now <= OFF) {
		if (tick) {
			PORTA = 0xAA;
			PORTC = 0x15;
			PORTD = 0x00;
		} else {
			PORTA = 0x55;
			PORTC = 0x2A;
			PORTD = 0x80;
		}
		tick = !tick;
	} else if (now >= START && now <= WAKE) {
		// interpolate between START and WAKE
		int illum = (NUM_COUNT+1) * (now - START) / (WAKE - START);
		// first 1 -> PORTD (top bit)
		PORTD = (illum > 0) ? 0x80 : 0x00;
		// next 6 -> PORTC (bottom bits)
		PORTC = (illum > 1) ? UNARY(illum-1) : 0x00;
		// next 8 -> PORTA (top bits, inverted order)
		PORTA = (illum > 7) ? ~UNARY(9-(illum-6)) : 0x00;
	} else if (t.s == 0) {
		// flash the bottom LED once per minute
		PORTD = 0x80;
	} else {
		PORTD = 0x00;
		PORTA = 0x00;
		PORTC = 0x00;
	}
}

// Wait for external clock crystal to stabilise
void wait_ext_crystal() {
	int temp0, temp1;

	for (temp0=0; temp0<0x0040; temp0++) {
		for (temp1=0; temp1<0xFFFF; temp1++);
	}
}

int main() {
	// initialise t to now
	t.s = NOW_S;
	t.m = NOW_M;
	t.h = NOW_H;
	t.d = NOW_D;

	DDRA = 0xFF;
	DDRC = 0x3F; // all except top two
	DDRD = 0x80; // just PD7

	PORTA = 0xFF;
	PORTC = 0x3F;
	PORTD = 0x80;

	wait_ext_crystal();
	PORTA = 0x08;

	TIMSK &= ~((1<<TOIE2)|(1<<OCIE2)); // disable TC2 interrupt

	ASSR |= 1<<AS2; // configure timer2 to be clocked by TOSC

	TCNT2 = 0x00;
	TCCR2 = 0x05; // pre-scale to 1/128

	while (ASSR & 0x07); // wait until TC2 finished updating

	PORTA = 0x80;

	TIMSK |= 1<<TOIE2; // timer2 overflow interrupt enable
	sei(); // global interrupt enable

	while (1) {
		MCUCR = 0x38; // select power-save mode

		sleep_enable();
		sei();
		sleep_cpu();
		sleep_disable();

		TCCR2 = 0x05; // dummy value to control reg
		while (ASSR & 0x07); // wait until TC2 updated
	}

	return 0;
}

ISR(TIMER2_OVF_vect) {
	if (++t.s == 60) {
		t.s = 0;
		if (++t.m == 60) {
			t.m = 0;
			if (++t.h == 24) {
				t.h = 0;
				if (++t.d == 7) {
					t.d = 0;
				}
			}
		}
	}

	update_leds();
}
