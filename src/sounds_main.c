#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/sleep.h>


#ifdef PROGMEM 
#undef PROGMEM 
#define PROGMEM __attribute__((section(".progmem.data"))) 
#endif


#define SOUND_FREQ 16000


static uint16_t g_curr_sound_len;
static const /*PROGMEM*/ unsigned char* g_pgm_play_buff;
static uint16_t g_play_buff_pos;


/* actual sounds */
extern const PROGMEM unsigned char sound0[];
extern const uint16_t sound0_len;
extern const PROGMEM unsigned char sound1[];
extern const uint16_t sound1_len;
extern const PROGMEM unsigned char sound2[];
extern const uint16_t sound2_len;
extern const PROGMEM unsigned char sound3[];
extern const uint16_t sound3_len;

static void setup();
static void loop();
static void go_to_sleep();


int main()
{
  setup();
  
  while (1) {
    loop();
  }
}


static void sound_setup()
{
  g_play_buff_pos = 0xfffe;

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
  OCR1A = 0;
  
  
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
  
  sei();
}


static void setup()
{
  volatile unsigned char dummy;

  sound_setup();

  /* set the pins as inputs (don't pull them up) */
  DDRD = 0; // input

  /* set port D pins 0-3 to trigger interrupts */
  EICRA = 0xff;
  EIMSK = 0x0f;

  /* use some references to sound0/1/2/3 to keep them in the binary */
  dummy = sound0[0];
  //dummy = sound1[0];
  //dummy = sound2[0];
  //dummy = sound3[0];
}


static void go_to_sleep()
{
  // disable the timer interrupts
  ETIMSK = 0;

  // clear the PWM output
  OCR1A = 0;

  // set sleep mode to POWER DOWN mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  // go to sleep
  sleep_cpu();
}


static void wake_up()
{
  // back from sleep
  sleep_disable();
  
  // enable interrupt when TCNT3 == OCR3A
  ETIMSK = _BV(OCIE3A);
}


void loop()
{
  if (g_play_buff_pos == 0xfffe) {
    g_play_buff_pos = 0xffff;
    go_to_sleep();
  }
}


ISR(TIMER3_COMPA_vect)
{
  if (g_play_buff_pos < 0xfff0) {
    OCR1A = pgm_read_byte_far((uint16_t)&g_pgm_play_buff[g_play_buff_pos]);
    g_play_buff_pos++;
    if (g_play_buff_pos == g_curr_sound_len) {
      g_play_buff_pos = 0xfffe;
    }
  }
}


ISR(INT0_vect)
{
  wake_up();

  cli();
  g_play_buff_pos = 0;
  g_pgm_play_buff = sound0;
  g_curr_sound_len = sound0_len;
  sei();
}

