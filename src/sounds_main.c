#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>


#ifdef PROGMEM
#undef PROGMEM
#define PROGMEM __attribute__((section(".progmem.data")))
#endif


#define GET_FAR_ADDRESS(var)                        \
({                                                  \
    uint_farptr_t tmp;                              \
                                                    \
    __asm__ __volatile__(                           \
                                                    \
            "ldi    %A0, lo8(%1)"           "\n\t"  \
            "ldi    %B0, hi8(%1)"           "\n\t"  \
            "ldi    %C0, hh8(%1)"           "\n\t"  \
            "clr    %D0"                    "\n\t"  \
        :                                           \
            "=d" (tmp)                              \
        :                                           \
            "p"  (&(var))                           \
    );                                              \
    tmp;                                            \
})


#define SOUND_FREQ 16000


volatile uint32_t g_curr_sound_len;
volatile uint32_t g_pgm_play_buff;
volatile uint32_t g_play_buff_pos;


// RC4-based PRNG
#define PRNG_STATE_LEN 256
static uint8_t prng_buff[PRNG_STATE_LEN];
static uint8_t prng_i;
static uint8_t prng_j;

// assumption: key_len is a power of 2
static void prng_put(const uint8_t key[], uint8_t key_len)
{
  uint8_t i, tmp;
  uint8_t j=0;

  i=0;
  do {
    prng_buff[i] = i;
    i++;
  } while (i != PRNG_STATE_LEN-1);

  i=0;
  do {
    j = (uint8_t) (j + prng_buff[i]);
    j = (uint8_t) key [i & (key_len-1)];
    tmp = prng_buff [i];
    prng_buff [i] = prng_buff [j];
    prng_buff [j] = tmp;
    i++;
  } while (i != PRNG_STATE_LEN-1);
}

static uint8_t prng_get()
{
  uint8_t tmp;

  prng_i = (uint8_t) (prng_i + 1);
  prng_j = (uint8_t) (prng_j + prng_buff[prng_i]);

  tmp = prng_buff[prng_i];
  prng_buff[prng_i] = prng_buff[prng_j];
  prng_buff[prng_j] = tmp;

  return prng_buff[(uint8_t) (prng_buff[prng_i] + prng_buff[prng_j])];
}


// actual sounds
extern const PROGMEM unsigned char sound0[];
extern const uint32_t sound0_len;
extern const PROGMEM unsigned char sound1[];
extern const uint32_t sound1_len;
extern const PROGMEM unsigned char sound2[];
extern const uint32_t sound2_len;
extern const PROGMEM unsigned char sound3[];
extern const uint32_t sound3_len;

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
  OCR1A = 0;


  // Set up Timer 1 to send a sample every interrupt.

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
}


static void setup()
{
  // disable interrupts to allow proper setup
  cli();

  sound_setup();

  // initialize the PRNG
  const uint8_t base_key[2] = {0x56, 0xa3};
  prng_put (base_key, sizeof (base_key));

  /* set the pins as inputs (don't pull them up) */
  PORTD = 0;
  DDRD = 0; // input

  // set port D pins 0-3 to trigger interrupts
  EICRA = 0xff; // interrupts on rising edge
  EIFR = 0xff; // clear previous interrupts
  EIMSK = 0x0f; // enable the interrupt sources

  sei();
}


static void go_to_sleep()
{
  // disable the timer interrupts
  ETIMSK = 0;

  TCCR1A = 0;
  TCCR1B = 0;
  TCCR3A = 0;
  TCCR3B = 0;

  // clear the PWM output
  OCR1A = 0;

  // set sleep mode to POWER DOWN mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // put the CPU to sleep

  // sei() is a must, otherwise we won't wake up. atomicity is granted:
  // interrupts won't be handled between the sei() instruction and the
  // sleep_cpu() instruction
  sei();

  sleep_cpu();
}


static void wake_up()
{
  sound_setup();

  // enable interrupt when TCNT3 == OCR3A
  ETIMSK = _BV(OCIE3A);
}


void loop()
{
  // disable interrupts to atomically check the position
  cli();
  if (g_play_buff_pos == 0xffff) {
    go_to_sleep(); // NOTE: interrupts will be enabled after this line
    // back from sleep
    sleep_disable();
  }
  // if we're here then we've either woken up or we're still playing a sound.
  // either way, we should enable interrupts back again in order to handle the
  // timer interrupt and the external trigger
  sei();
  // add a few NOPs to allow the CPU to receive interrupts. there's an
  // atomicity thing that prevents the CPU from taking interrupts immediately
  // after sei() instructions
  _NOP();
  _NOP();
}


ISR(TIMER3_COMPA_vect)
{
  // disable interrupts to enable proper interrupt handling
  cli();

  if (g_play_buff_pos < 0xfff0) {
    OCR1A = pgm_read_byte_far(g_pgm_play_buff + g_play_buff_pos);
    g_play_buff_pos++;
    if (g_play_buff_pos >= g_curr_sound_len) {
      g_play_buff_pos = 0xffff;
    }
  }

  // enable interrupts back again
  sei();
}


static void select_sound(uint8_t sound_index)
{
  switch (sound_index) {
    case 0:
      g_pgm_play_buff = GET_FAR_ADDRESS(sound0);
      g_curr_sound_len = sound0_len;
      break;
    case 1:
      g_pgm_play_buff = GET_FAR_ADDRESS(sound1);
      g_curr_sound_len = sound1_len;
      break;
    case 2:
      g_pgm_play_buff = GET_FAR_ADDRESS(sound2);
      g_curr_sound_len = sound2_len;
      break;
    case 3:
      g_pgm_play_buff = GET_FAR_ADDRESS(sound3);
      g_curr_sound_len = sound3_len;
      break;
    default:
      break;
  }
}

static void choose_random_sound(uint8_t button_index)
{
  uint8_t rnd;

  // choose a random sound or use the interrupt index
  switch (button_index) {
    case 0:
      // select a sound between 0 and 3
      select_sound(prng_get() & 0x03);
      break;
    case 1:
      // select sound number 4
      select_sound(4);
      break;
    case 2:
      // select a sound between 5 and 6
      select_sound(5 + (prng_get() & 0x01));
      break;
    case 3:
    default:
      // select a random sound
      rnd = prng_get();
      switch ((rnd & 0x0c) >> 2) {
        case 0:
        case 1:
          select_sound(rnd & 0x03);
          break;
        case 2:
          select_sound(4);
        case 3:
        default:
          select_sound(5 + (rnd & 0x01));
          break;
      }
      break;
  }
}

static void external_interrupt_handler(uint8_t interrupt_index)
{
  // disable interrupts to enable proper interrupt handling
  cli();

  // only allow selecting a new sound with a selection frequency of less than
  // 10 Hz
  if (g_play_buff_pos > 1600) {
    wake_up();

    g_play_buff_pos = 0;

    if (0)
      choose_random_sound(interrupt_index);
    select_sound(interrupt_index);
  }

  // enable interrupts back again
  sei();
}


ISR(INT0_vect)
{
  // clear the interrupt
  EIFR = 0x01;
  // to avoid unnecessary interrupts, only handle the interrupt in case the
  // button was actually pressed
  if (PIND & 0x01)
    external_interrupt_handler(0);
}

ISR(INT1_vect)
{
  // clear the interrupt
  EIFR = 0x02;
  // to avoid unnecessary interrupts, only handle the interrupt in case the
  // button was actually pressed
  if (PIND & 0x02)
    external_interrupt_handler(1);
}

ISR(INT2_vect)
{
  // clear the interrupt
  EIFR = 0x04;
  // to avoid unnecessary interrupts, only handle the interrupt in case the
  // button was actually pressed
  if (PIND & 0x04)
    external_interrupt_handler(2);
}

ISR(INT3_vect)
{
  // clear the interrupt
  EIFR = 0x08;
  // to avoid unnecessary interrupts, only handle the interrupt in case the
  // button was actually pressed
  if (PIND & 0x08)
    external_interrupt_handler(3);
}

