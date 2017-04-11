#include <zzjduino.h>
#include <avr/wdt.h>
//#include <TinyDebugSerial.h>

#define btog(X,BIT) ((X) ^= bv(BIT))

/*
			 ___ ___
 RESET	B5  -|  o  |- VCC  
 ADC3	B3  -|atiny|- B2   SCK   ADC1
 ADC2	B4  -|45/85|- B1   MISO
	    GND -|_____|- B0   MOSI  OC0A
*/

/* Pins 

				 ___ ___
 RESET		B5  -|  o  |- VCC  
			B3  -|atiny|- B2   PWM_IN
			B4  -|45/85|- B1     
			GND -|_____|- B0   PWM_OUT   

*/

#define PWM_OUT_PIN 0

#define PWM_A_PORT PINB
#define PWM_A_PIN  2


int getDuty();

int main(void) {
	//Serial.begin(38400);
	//millis_start();

	// enable watchdog reset at 200ms or so
	wdt_enable(WDTO_1S);
	
	// set up inputs and outputs
	DDRB = (1 << PWM_OUT_PIN);
	
	// set up pwm output at 100%
	PORTB = (1 << PWM_OUT_PIN);
		
	// enable PWM output
	TCCR0A = (1 << COM0A1)  |  (1 << WGM01) |  (1 << WGM00); // fast pwm, OCRA=TOP
	TCCR0B = (1 << CS00); //  /8 
	OCR0A = 255;
	
	delay(300); // wait for the fans to spin up
	OCR0A = 255 * 40 / 100;
	
	// no interrupts lol
	//sei();
	
	int outDuty;
		
	while(true) {	
		wdt_reset();
		
		// measure PWM inputs
		int outDuty = constrain(getDuty() + 20, 40, 100); // max(getDuty(),10);
		
		// set fan pwm output to outDuty
		OCR0A = 255 * outDuty / 100;
		
		// loop
	}

	return 0;
}

// get duty cycle of first PWM input
// syncs with input, then measures
int getDuty() {
   int up, down;
  
	asm volatile (
	"	cli						""\n"
	"	ldi %A0,0				""\n"
	"	ldi %B0,0				""\n"
	"	ldi %A1,0				""\n"
	"	ldi %B1,0				""\n"

	".%=H1:						""\n"
	"	adiw %A0,1				""\n"
	"	brvs .%=timeoutH		""\n"
	"	sbic %2, %3	 			""\n"
	"	rjmp .%=H1	 			""\n"

	".%=L1:						""\n"
	"	adiw %A1,1				""\n"
	"	brvs .%=timeoutL		""\n"
	"	sbis %2,%3 				""\n"
	"	rjmp .%=L1				""\n"

	"	ldi %A0,0				""\n"
	"	ldi %B0,0	 			""\n"
	"	ldi %A1,0				""\n"
	"	ldi %B1,0	 			""\n"

	".%=H2:						""\n"
	"	adiw %A0,1				""\n"
	"	brvs .%=timeoutH		""\n"
	"	sbic %2, %3	 			""\n"
	"	rjmp .%=H2	 			""\n"

	".%=L2:						""\n"
	"	adiw %A1,1				""\n"
	"	brvs .%=timeoutL		""\n"
	"	sbis %2, %3 			""\n"
	"	rjmp .%=L2				""\n"
	" 	rjmp .%=end				""\n"	
		
	".%=timeoutH: 				""\n"
	"	ldi %A0,100				""\n"
	"	ldi %B0,0				""\n"
	"	ldi %A1,0				""\n"
	"	ldi %B1,0				""\n"
	" 	rjmp .%=end				""\n"
		
	".%=timeoutL: 				""\n"
	"	ldi %A0,0				""\n"
	"	ldi %B0,0				""\n"
	"	ldi %A1,0				""\n"
	"	ldi %B1,0				""\n"

	".%=end:					""\n"
	"	sei						""\n"
		 : "=w" (up), "=w"(down)  //  output
		 : "M"(_SFR_IO_ADDR(PWM_A_PORT)), "M"(PWM_A_PIN) //input
		 : // clobber
		 );

   return constrain(((long)up) * 100 / (up+down), 0, 100);
}
