#include <msp430.h> 
#include <stdlib.h>
#include "peripherals.h"

unsigned int waveIter = 0;
unsigned int wheelISR = 0;
unsigned int dac_codeISR = 0;
uint8_t hiByteISR = 0;
uint8_t loByteISR = 0;
uint8_t waveType = 0;
uint8_t bitFlip = 0;

void timerSquirtSetup() {
    TA2CTL = (TASSEL_1 + MC_1 + ID_0);
    TA2CCR0 = 204;
    TA2CCTL0 = CCIE;
}

void timerBoofSetup() {
    TA2CTL = (TASSEL_2 + MC_1 + ID_0);
    TA2CCR0 = 698;
    TA2CCTL0 = CCIE;
}

void timerAngelSetup() {
    TA2CTL = (TASSEL_2 + MC_1 + ID_0);
    TA2CCR0 = 275;
    TA2CCTL0 = CCIE;
}

void configButt() {
    P7SEL &= ~(BIT4 | BIT0);
    P2SEL &= ~(BIT2);
    P3SEL &= ~(BIT6);

    P7DIR &= ~(BIT4 | BIT0);
    P2DIR &= ~(BIT2);
    P3DIR &= ~(BIT6);

    P7REN |= (BIT4 | BIT0);
    P2REN |= (BIT2);
    P3REN |= (BIT6);

    P7OUT |= (BIT4 | BIT0);
    P2OUT |= (BIT2);
    P3OUT |= (BIT6);
}

void configDAC() {
    DAC_PORT_LDAC_SEL &= ~DAC_PIN_LDAC;
    DAC_PORT_LDAC_DIR |= DAC_PIN_LDAC;
    DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;

    DAC_PORT_CS_SEL &= ~DAC_PIN_CS;
    DAC_PORT_CS_DIR |= DAC_PIN_CS;
    DAC_PORT_CS_OUT |= DAC_PIN_CS;
}

void configWheel() {
    REFCTL0 &= ~REFMSTR; /* Release control from REFMSTR to internal reference */
    ADC12CTL0 = ADC12SHT0_9 | ADC12ON | ADC12REFON | ADC12MSC;
    ADC12CTL1 = ADC12SHP | ADC12CONSEQ_1;
    ADC12MCTL0 = ADC12SREF_0 | ADC12INCH_0;
    ADC12MCTL1 = ADC12SREF_0 | ADC12INCH_1;
    __delay_cycles(100);
    P6SEL |= (BIT1 | BIT0);
    __delay_cycles(100);
    ADC12CTL0 &= ~ADC12SC;
    ADC12CTL0 |= ADC12SC | ADC12ENC;
}

unsigned char readButt() {
    unsigned char out = 0;
    if (!(P7IN & BIT0)) {
        out |= BIT3;
    }
    if (!(P3IN & BIT6)) {
        out |= BIT2;
    }
    if (!(P2IN & BIT2)) {
        out |= BIT1;
    }
    if (!(P7IN & BIT4)) {
        out |= BIT0;
    }
    return out;
}
uint8_t whalecumScreen() {
    unsigned char buttResult = 0;
    uint8_t waveSelec = 0;
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "WELCOME", AUTO_STRING_LENGTH, 48, 10, TRANSPARENT_TEXT);
    Graphics_drawString(&g_sContext, "Butt 1 DC", AUTO_STRING_LENGTH, 3, 25, TRANSPARENT_TEXT);
    Graphics_drawString(&g_sContext, "Butt 2 Square", AUTO_STRING_LENGTH, 3, 40, TRANSPARENT_TEXT);
    Graphics_drawString(&g_sContext, "Butt 3 Sawtooth", AUTO_STRING_LENGTH, 3, 55, TRANSPARENT_TEXT);
    Graphics_drawString(&g_sContext, "Butt 4 Triangle", AUTO_STRING_LENGTH, 3, 70, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    while(1) {
        buttResult = readButt();
        if (buttResult == 8) {
            waveSelec = 1;
            break;
        } else if (buttResult == 4) {
            waveSelec = 2;
            break;
        } else if (buttResult == 2) {
            waveSelec = 3;
            break;
        } else if (buttResult == 1) {
            waveSelec = 4;
            break;
        }
    }
    return waveSelec;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void TimerA2_ISR() {
    if (waveType ==  1) {
        if (bitFlip == 0) {
            wheelISR = ADC12MEM0 & 0x0FFF;
            dac_codeISR = wheelISR | 0x3000;
            loByteISR = ((unsigned char)(dac_codeISR & 0x00FF));
            hiByteISR = ((unsigned char)((dac_codeISR & 0xFF00) >> 8));
            DAC_PORT_CS_OUT &= ~DAC_PIN_CS;
            DAC_SPI_REG_TXBUF = hiByteISR;
            while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
            DAC_SPI_REG_TXBUF = loByteISR;
            while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
            DAC_PORT_CS_OUT |= DAC_PIN_CS;
            DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;
            DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;
            bitFlip = 1;
        } else if (bitFlip == 1) {
            dac_codeISR = 0 | 0x3000;
            loByteISR = ((unsigned char)(dac_codeISR & 0x00FF));
            hiByteISR = ((unsigned char)((dac_codeISR & 0xFF00) >> 8));
            DAC_PORT_CS_OUT &= ~DAC_PIN_CS;
            DAC_SPI_REG_TXBUF = hiByteISR;
            while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
            DAC_SPI_REG_TXBUF = loByteISR;
            while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
            DAC_PORT_CS_OUT |= DAC_PIN_CS;
            DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;
            DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;
            bitFlip = 0;
        }
    } else if (waveType == 2) {
        dac_codeISR = waveIter | 0x3000;
        loByteISR = ((unsigned char)(dac_codeISR & 0x00FF));
        hiByteISR = ((unsigned char)((dac_codeISR & 0xFF00) >> 8));
        DAC_PORT_CS_OUT &= ~DAC_PIN_CS;
        DAC_SPI_REG_TXBUF = hiByteISR;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_SPI_REG_TXBUF = loByteISR;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_PORT_CS_OUT |= DAC_PIN_CS;
        DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;
        DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;
        waveIter += 204;
        if (waveIter > 4080) {
            waveIter = 0;
        }

    } else if (waveType == 3) {
        dac_codeISR = waveIter | 0x3000;
        loByteISR = ((unsigned char)(dac_codeISR & 0x00FF));
        hiByteISR = ((unsigned char)((dac_codeISR & 0xFF00) >> 8));
        DAC_PORT_CS_OUT &= ~DAC_PIN_CS;
        DAC_SPI_REG_TXBUF = hiByteISR;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_SPI_REG_TXBUF = loByteISR;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_PORT_CS_OUT |= DAC_PIN_CS;
        DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;
        DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;
        if (bitFlip ==  0) {
            waveIter += 215;
        } else if (bitFlip == 1) {
            waveIter -= 215;
            if (waveIter == 0) {
                bitFlip = 0;
            }
        }
        if (waveIter > 4085) {
            waveIter = 4085;
            bitFlip = 1;
        }
    }
}

void deeCee() {
    volatile unsigned int led = 0;
    volatile float ledV = 0;
    volatile unsigned int wheel = 0;
    volatile float dacV = 0;
    unsigned int dac_code = 0;
    uint8_t hiByte = 0;
    uint8_t loByte = 0;
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "Running DC", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    while(1) {
        led = ADC12MEM1 & 0x0FFF;
        ledV = (led * 0.0008058608059);
        wheel = ADC12MEM0 & 0x0FFF;
        dac_code = wheel | 0x3000;
        dacV = (wheel * 0.0008058608059);
        loByte = ((unsigned char)(dac_code & 0x00FF));
        hiByte = ((unsigned char)((dac_code & 0xFF00) >> 8));
        DAC_PORT_CS_OUT &= ~DAC_PIN_CS;
        DAC_SPI_REG_TXBUF = hiByte;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_SPI_REG_TXBUF = loByte;
        while(!(DAC_SPI_REG_IFG & UCTXIFG)) {}
        DAC_PORT_CS_OUT |= DAC_PIN_CS;
        DAC_PORT_LDAC_OUT &= ~DAC_PIN_LDAC;
        __delay_cycles(10);
        DAC_PORT_LDAC_OUT |= DAC_PIN_LDAC;
    }
}

void squirt() {
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "Running Square", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    waveType = 1;
    timerSquirtSetup();
    _BIS_SR(GIE);
    while(1) {}
}

void sawBoof() {
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "Running Sawtooth", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    waveType = 2;
    timerBoofSetup();
    _BIS_SR(GIE);
    while(1) {}

}

void tryangel() {
    Graphics_clearDisplay(&g_sContext);
    Graphics_drawStringCentered(&g_sContext, "Running Triangle", AUTO_STRING_LENGTH, 48, 48, TRANSPARENT_TEXT);
    Graphics_flushBuffer(&g_sContext);
    waveType = 3;
    timerAngelSetup();
    _BIS_SR(GIE);
    while(1) {}
}

int main(void) {
    uint8_t whaleResult;
    WDTCTL = WDTPW | WDTHOLD;// stop watchdog timer
    configDisplay();
    configWheel();
    configButt();
    configDAC();
    whaleResult = whalecumScreen();
    if (whaleResult == 1) {
        deeCee();
    } else if (whaleResult == 2) {
        squirt();
    } else if (whaleResult == 3) {
        sawBoof();
    } else if (whaleResult == 4) {
        tryangel();
    } else {
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawStringCentered(&g_sContext, "Holy Moly", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "You Uber Broke It", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Good Job Schmuck", AUTO_STRING_LENGTH, 48, 15, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }
    Graphics_clearDisplay(&g_sContext);
    return 0;
}
