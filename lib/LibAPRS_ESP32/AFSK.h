#ifndef AFSK_H
#define AFSK_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pgmspace.h>
#include "FIFO.h"
#include "HDLC.h"

#define SIN_LEN 512
static const uint8_t sin_table[] =
    {
        128,
        129,
        131,
        132,
        134,
        135,
        137,
        138,
        140,
        142,
        143,
        145,
        146,
        148,
        149,
        151,
        152,
        154,
        155,
        157,
        158,
        160,
        162,
        163,
        165,
        166,
        167,
        169,
        170,
        172,
        173,
        175,
        176,
        178,
        179,
        181,
        182,
        183,
        185,
        186,
        188,
        189,
        190,
        192,
        193,
        194,
        196,
        197,
        198,
        200,
        201,
        202,
        203,
        205,
        206,
        207,
        208,
        210,
        211,
        212,
        213,
        214,
        215,
        217,
        218,
        219,
        220,
        221,
        222,
        223,
        224,
        225,
        226,
        227,
        228,
        229,
        230,
        231,
        232,
        233,
        234,
        234,
        235,
        236,
        237,
        238,
        238,
        239,
        240,
        241,
        241,
        242,
        243,
        243,
        244,
        245,
        245,
        246,
        246,
        247,
        248,
        248,
        249,
        249,
        250,
        250,
        250,
        251,
        251,
        252,
        252,
        252,
        253,
        253,
        253,
        253,
        254,
        254,
        254,
        254,
        254,
        255,
        255,
        255,
        255,
        255,
        255,
        255,
};

inline static uint8_t sinSample(uint16_t i)
{
    uint16_t newI = i % (SIN_LEN / 2);
    newI = (newI >= (SIN_LEN / 4)) ? (SIN_LEN / 2 - newI - 1) : newI;
    uint8_t sine = pgm_read_byte(&sin_table[newI]);
    return (i >= (SIN_LEN / 2)) ? (255 - sine) : sine;
}

#define SWITCH_TONE(inc) (((inc) == MARK_INC) ? SPACE_INC : MARK_INC)
#define BITS_DIFFER(bits1, bits2) (((bits1) ^ (bits2)) & 0x01)
#define DUAL_XOR(bits1, bits2) ((((bits1) ^ (bits2)) & 0x03) == 0x03)
#define SIGNAL_TRANSITIONED(bits) DUAL_XOR((bits), (bits) >> 2)
#define TRANSITION_FOUND(bits) BITS_DIFFER((bits), (bits) >> 1)

#define CPU_FREQ F_CPU

#define CONFIG_AFSK_RX_BUFLEN 350
#define CONFIG_AFSK_TX_BUFLEN 350
#define CONFIG_AFSK_RXTIMEOUT 0
#define CONFIG_AFSK_PREAMBLE_LEN 150UL
#define CONFIG_AFSK_TRAILER_LEN 50UL
#define CONFIG_AFSK_DAC_SAMPLERATE 9600
#define SAMPLERATE 9600
#define BITRATE 1200
#define SAMPLESPERBIT (SAMPLERATE / BITRATE)
#define BIT_STUFF_LEN 5
#define MARK_FREQ 1200
#define SPACE_FREQ 2200
#define PHASE_BITS 8                           // How much to increment phase counter each sample
#define PHASE_INC 1                            // Nudge by an eigth of a sample each adjustment
#define PHASE_MAX (SAMPLESPERBIT * PHASE_BITS) // Resolution of our phase counter = 64
#define PHASE_THRESHOLD (PHASE_MAX / 2)        // Target transition point of our phase window

#define I2S_INTERNAL

#define SPK_PIN ADC1_CHANNEL_0 // Read ADC1_0 From PIN 36(VP)
#define MIC_PIN 26             // Out wave to PIN 26
#define RSSI_PIN 33
#define PTT_PIN 32
#define LED_PIN 2
#define LED_TX_PIN 4

#ifdef I2S_INTERNAL
#include "driver/i2s.h"
#include "driver/dac.h"

#define SAMPLE_RATE 9600 //9580 ปรับชดเชยแซมปลิงค์ของ I2S
#define PIN_I2S_BCLK 26
#define PIN_I2S_LRC 27
#define PIN_I2S_DIN 35
#define PIN_I2S_DOUT 25

typedef enum
{
    LEFTCHANNEL = 0,
    RIGHTCHANNEL = 1
} SampleIndex;
/// @parameter MODE : I2S_MODE_RX or I2S_MODE_TX
/// @parameter BPS : I2S_BITS_PER_SAMPLE_16BIT or I2S_BITS_PER_SAMPLE_32BIT
void I2S_Init(i2s_mode_t MODE, i2s_bits_per_sample_t BPS);
#endif

typedef struct Hdlc
{
    uint8_t demodulatedBits;
    uint8_t bitIndex;
    uint8_t currentByte;
    bool receiving;
} Hdlc;

typedef struct Afsk
{
    // Stream access to modem
    FILE fd;

    // General values
    Hdlc hdlc;               // We need a link control structure
    uint16_t preambleLength; // Length of sync preamble
    uint16_t tailLength;     // Length of transmission tail

    // Modulation values
    uint8_t sampleIndex;       // Current sample index for outgoing bit
    uint8_t currentOutputByte; // Current byte to be modulated
    uint8_t txBit;             // Mask of current modulated bit
    bool bitStuff;             // Whether bitstuffing is allowed

    uint8_t bitstuffCount; // Counter for bit-stuffing

    uint16_t phaseAcc; // Phase accumulator
    uint16_t phaseInc; // Phase increment per sample

    FIFOBuffer txFifo;                    // FIFO for transmit data
    uint8_t txBuf[CONFIG_AFSK_TX_BUFLEN]; // Actial data storage for said FIFO

    volatile bool sending; // Set when modem is sending

    // Demodulation values
    FIFOBuffer delayFifo;                   // Delayed FIFO for frequency discrimination
    int8_t delayBuf[SAMPLESPERBIT / 2 + 1]; // Actual data storage for said FIFO

    FIFOBuffer rxFifo;                    // FIFO for received data
    uint8_t rxBuf[CONFIG_AFSK_RX_BUFLEN]; // Actual data storage for said FIFO

    int16_t iirX[2]; // IIR Filter X cells
    int16_t iirY[2]; // IIR Filter Y cells

    uint16_t sampledBits; // Bits sampled by the demodulator (at ADC speed)
    int8_t currentPhase;  // Current phase of the demodulator
    uint8_t actualBits;   // Actual found bits at correct bitrate

    volatile int status; // Status of the modem, 0 means OK

} Afsk;

#define DIV_ROUND(dividend, divisor) (((dividend) + (divisor) / 2) / (divisor))
#define MARK_INC (uint16_t)(DIV_ROUND(SIN_LEN * (uint32_t)MARK_FREQ, CONFIG_AFSK_DAC_SAMPLERATE))
#define SPACE_INC (uint16_t)(DIV_ROUND(SIN_LEN * (uint32_t)SPACE_FREQ, CONFIG_AFSK_DAC_SAMPLERATE))

#define AFSK_DAC_IRQ_START()         \
    do                               \
    {                                \
        extern bool hw_afsk_dac_isr; \
        hw_afsk_dac_isr = true;      \
        digitalWrite(LED_TX_PIN,HIGH);\
    } while (0)
#define AFSK_DAC_IRQ_STOP()          \
    do                               \
    {                                \
        extern bool hw_afsk_dac_isr; \
        hw_afsk_dac_isr = false;     \
        digitalWrite(LED_TX_PIN,LOW);\
    } while (0)
//#define AFSK_DAC_INIT()        do { DAC_DDR |= (DAC_PINS) ; PTT_DDR = 0b01000000;} while (0)

// Here's some macros for controlling the RX/TX LEDs
// THE _INIT() functions writes to the DDRB register
// to configure the pins as output pins, and the _ON()
// and _OFF() functions writes to the PORT registers
// to turn the pins on or off.

#define LED_RX_ON() digitalWrite(LED_PIN, HIGH);
#define LED_RX_OFF() digitalWrite(LED_PIN, LOW);

void AFSK_init(Afsk *afsk);
void AFSK_transmit(char *buffer, size_t size);
void AFSK_poll(Afsk *afsk);

void afsk_putchar(char c);
int afsk_getchar(void);
void AFSK_Poll();
void AFSK_TimerEnable(bool sts);

#endif