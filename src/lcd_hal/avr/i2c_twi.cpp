/*
    MIT License

    Copyright (c) 2018-2019, Alexey Dynda

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "../io.h"

#if defined(CONFIG_TWI_I2C_AVAILABLE) && defined(CONFIG_TWI_I2C_ENABLE)

#include <avr/io.h>
#include <util/twi.h>

/* Max i2c frequency, supported by OLED controllers */
#define SSD1306_TWI_FREQ  400000
#define MAX_RETRIES       64

static uint8_t ssd1306_twi_start(void)
{
    uint8_t twst;
    uint8_t iters = MAX_RETRIES;
    do
    {
        TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
        while ( (TWCR & (1<<TWINT)) == 0 );
        twst = TWSR & 0xF8;
        if (!--iters)
        {
            break;
        }
    } while (twst == TW_MT_ARB_LOST);
    if ((twst != TW_START) && (twst != TW_REP_START))
    {
        return twst;
    }
    return 0;
}

static uint8_t ssd1306_twi_send(uint8_t data)
{
    uint8_t twsr;
    uint8_t iters = MAX_RETRIES;
    do
    {
        TWDR = data;
        TWCR = (1<<TWINT) | (1<<TWEN);
        while ( (TWCR & (1<<TWINT)) == 0 );
        twsr = TWSR & 0xF8;
        if ((twsr == TW_MT_SLA_ACK) || (twsr == TW_MT_DATA_ACK))
        {
            return 0;
        }
        if (twsr == TW_MT_ARB_LOST)
        {
            return twsr;
        }
        iters++;
        if (!--iters)
        {
            break;
        }
    } while (twsr != TW_MT_ARB_LOST);
    return twsr;
}

static void ssd1306_twi_stop(void)
{
    TWCR = (1<<TWEN) | (1<<TWSTO) | (1<<TWINT);
}

void ssd1306_i2cConfigure_Twi(uint8_t arg)
{
#if defined(__AVR_ATmega328P__)
    /* Enable internal pull-ups */
    DDRC &= ~(1<<PINC4); PORTC |= (1<<PINC4);
    DDRC &= ~(1<<PINC5); PORTC |= (1<<PINC5);
#endif
#if defined(TWPS0)
    TWSR = 0;
#endif
    TWBR = ((F_CPU / SSD1306_TWI_FREQ) - 16) / 2 / (1); // Always use prescaler 1 (TWSR 0x00)
    TWCR = (1 << TWEN) | (1 << TWEA);
}


TwiI2c::TwiI2c(uint8_t sa)
   : m_sa( sa )
{
}

TwiI2c::~TwiI2c()
{
}

void TwiI2c::begin()
{
}

void TwiI2c::end()
{
}

void TwiI2c::start()
{
    do
    {
        if (ssd1306_twi_start() != 0)
        {
            /* Some serious error happened, but we don't care. Our API functions have void type */
            return;
        }
    } while (ssd1306_twi_send(m_sa << 1) == TW_MT_ARB_LOST);
}

void TwiI2c::stop()
{
    ssd1306_twi_stop();
}

void TwiI2c::send(uint8_t data)
{
    for(;;)
    {
        if (ssd1306_twi_send(data) != TW_MT_ARB_LOST)
        {
            break;
        }
        if (ssd1306_twi_start() != 0)
        {
            /* Some serious error happened, but we don't care. Our API functions have void type */
            break;
        }
        if (ssd1306_twi_send(m_sa << 1) != TW_MT_ARB_LOST)
        {
            /* Some serious error happened, but we don't care. Our API functions have void type */
            break;
        }
    }
}

void TwiI2c::sendBuffer(const uint8_t *buffer, uint16_t size)
{
    while (size--)
    {
        send(*buffer);
        buffer++;
    }
}

#endif
