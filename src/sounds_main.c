#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>


#ifdef PROGMEM 
#undef PROGMEM 
#define PROGMEM __attribute__((section(".progmem.data"))) 
#endif


#define SOUND_FREQ 16000

#define NUM_BUTTONS 4
#define BTN_MASK ((1<<NUM_BUTTONS) - 1)

static uint16_t g_curr_sound_len;
static const /*PROGMEM*/ unsigned char* g_pgm_play_buff;
static uint16_t g_play_buff_pos;

static unsigned char g_btn_status;

/* actual sounds */
extern const PROGMEM unsigned char sound0[];
extern const PROGMEM unsigned char sound1[];
extern const PROGMEM unsigned char sound2[];
extern const PROGMEM unsigned char sound3[];

void setup();
void loop();


int main()
{
  setup();
  
  while (1) {
    loop();
  }
}


void sound_setup()
{
  g_play_buff_pos = 0xffff;

  DDRB = 0xff; // set all port B pins as outputs
  
  // Set up Timer 0 to do pulse width modulation on the speaker
  // pin.
  
  /*
  // Use internal clock
  ASSR &= ~(_BV(AS0));
  */
  
  // disable all timer interrupts
  TIMSK = 0;
  ETIMSK = 0;

  // Set 8-bit fast PWM mode
  TCCR1A |= _BV(WGM10);
  TCCR1A &= ~_BV(WGM11);
  TCCR1B |= _BV(WGM12);
  TCCR1B &= ~_BV(WGM13);
  
  // Do non-inverting PWM on pin OC1A
  TCCR1A = (TCCR1A | _BV(COM1A1)) & ~_BV(COM1A0);
  TCCR1A &= ~(_BV(COM1B1) | _BV(COM1B0));
  
  // No prescaler
  TCCR1B = (TCCR1B & ~(_BV(CS12) | _BV(CS11))) | _BV(CS10);
  
  // Set initial pulse width to the first sample.
  OCR1A = 0x80;
  
  
  // Set up Timer 1 to send a sample every interrupt.
  
  cli();
  
  // Set CTC mode (Clear Timer on Compare Match)
  // Have to set OCR3A *after*, otherwise it gets reset to 0!
  TCCR3B = (TCCR3B & ~_BV(WGM33)) | _BV(WGM32);
  TCCR3A = TCCR3A & ~(_BV(WGM31) | _BV(WGM30));
  
  // No prescaler
  TCCR3B = (TCCR3B & ~(_BV(CS32) | _BV(CS31))) | _BV(CS30);
  
  // Set the compare register (OCR3A).
  // OCR3A is a 16-bit register, so we have to do this with
  // interrupts disabled to be safe.
  OCR3A = (F_CPU / SOUND_FREQ);    // 8e6 / 16000 for atmega128
  
  // Enable interrupt when TCNT3 == OCR3A
  ETIMSK = _BV(OCIE3A);
  sei();
}


void setup()
{
  volatile unsigned char dummy;

  sound_setup();

  /* set the pins and pull them up */
  PORTC |= (PC0 | PC1 | PC2 | PC3); // pullup
  DDRC = 0; // input

  /* use some references to sound0/1/2/3 to keep them in the binary */
  dummy = sound0[0];
  //dummy = sound1[0];
  //dummy = sound2[0];
  //dummy = sound3[0];

  g_btn_status = BTN_MASK;
}

void loop()
{
  unsigned char btn_status = (PINC & BTN_MASK);

  if (g_play_buff_pos != 0xffff) return;

  /* wait for a button to be pressed */
  if (((btn_status & (1<<PINC0)) == 0) &&
      ((g_btn_status & (1<<PINC0)) != 0))
  {
    // disable interrupts
    cli();
    
    g_pgm_play_buff = sound0;
    g_curr_sound_len = 30000;
    g_play_buff_pos = 0;

    sei();
  }

  g_btn_status = btn_status;
}


ISR(TIMER3_COMPA_vect)
{
  if (g_play_buff_pos != 0xffff) {
    OCR1A = pgm_read_byte_far((uint16_t)&g_pgm_play_buff[g_play_buff_pos]);
    g_play_buff_pos++;
    if (g_play_buff_pos == g_curr_sound_len) {
      g_play_buff_pos = 0xffff;
    }
  }
}
