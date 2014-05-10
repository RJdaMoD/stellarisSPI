/*
* Copyright (c) 2014, Roger John
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Roger John nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Roger John ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Roger John BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*
* stellaris_spi.c
*
* Created on: 	03.05.2014
* File:			main.c.
* Author:		Roger John
* Version:		1.0
* Description:	Stellaris uart-spi bridge.
*/

#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_sysctl.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/uart.h"

void uart0_int_handler(void);
void ssi0_int_handler(void);

enum {
	hex, bin
} serialMode = hex;
unsigned char commandChar = '$';
const unsigned long SERIAL_SPEEDS[] = {
/* 0 - 9 */
50, 75, 110, 134, 200, 300, 600, 1200, 1800, 2400,
/* 10 - 19 / A - J*/
4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 500000, 576000,
/* 20 - 28 / K - S*/
921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000 };

long mapSerialSpeed(unsigned char c) {
	long r = -1;
	if (c >= '0' && c <= '9')
		r = c - '0';
	else if (c >= 'A' && c <= 'Z')
		r = c - 'A' + 10;
	else if (c >= 'a' && c <= 'z')
		r = c - 'a' + 10;
	if (r >= 0 && r < sizeof(SERIAL_SPEEDS))
		return SERIAL_SPEEDS[r];
	else
		return r;
}

unsigned long spi_mode(int pol, int phase) {
	if (pol) {
		if (phase)
			return SSI_FRF_MOTO_MODE_3;
		else
			return SSI_FRF_MOTO_MODE_2;
	} else {
		if (phase)
			return SSI_FRF_MOTO_MODE_1;
		else
			return SSI_FRF_MOTO_MODE_0;
	}
}
unsigned char nibbleToHex(unsigned char data) {
	data = data & 0xF;
	if (data > 9)
		return data - 10 + 'A';
	else
		return data + '0';
}
int hexToNibble(unsigned char data) {
	if (data >= '0' && data <= '9')
		return data - '0';
	else if (data >= 'A' && data <= 'F')
		return data - 'A' + 10;
	else if (data >= 'a' && data <= 'f')
		return data - 'a' + 10;
	else
		return -1;
}

// main function.
int main(void) {
	MAP_SysCtlClockSet(
	SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
	MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
	MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	MAP_UARTClockSourceSet(UART0_BASE, UART_CLOCK_SYSTEM);
	MAP_UARTConfigSetExpClk(UART0_BASE, MAP_SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE));
	MAP_IntEnable(INT_UART0);

	MAP_GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	MAP_GPIOPinConfigure(GPIO_PA3_SSI0FSS);
	MAP_GPIOPinConfigure(GPIO_PA4_SSI0RX);
	MAP_GPIOPinConfigure(GPIO_PA5_SSI0TX);
	MAP_GPIOPinTypeSSI(GPIO_PORTA_BASE,
	GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	MAP_SSIClockSourceSet(SSI0_BASE, SSI_CLOCK_SYSTEM);
	MAP_SSIConfigSetExpClk(SSI0_BASE, MAP_SysCtlClockGet(), spi_mode(0, 0),
	SSI_MODE_MASTER, 115200, 8);
	MAP_IntEnable(INT_SSI0);

	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	MAP_GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_3,
	GPIO_STRENGTH_8MA_SC, GPIO_PIN_TYPE_STD);
	MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_3);
	MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_3, 0);

	MAP_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);
	MAP_UARTEnable(UART0_BASE);
	MAP_SSIIntEnable(SSI0_BASE, SSI_RXFF | SSI_RXOR | SSI_RXTO);
	MAP_SSIEnable(SSI0_BASE);
	MAP_IntMasterEnable();

	while (1) {
	}
}
long readNumberFromUART(char endChar) {
	long x = 0;
	char c;
	while ((c = MAP_UARTCharGet(UART0_BASE)) != endChar) {
		if (c >= '0' && c <= '9')
			x = 10 * x + (c - '0');
		else
			return -1;
	}
	return x;
}
void handle_command() {
	long x;
	unsigned char c = MAP_UARTCharGet(UART0_BASE);
	if (c == commandChar) {
		if (serialMode == bin)
			MAP_SSIDataPut(SSI0_BASE, c);
		return;
	}
	switch (c) {
	case 'f':
		x = hexToNibble(MAP_UARTCharGet(UART0_BASE));
		if (x >= 0 && x <= 3)
			HWREG(SSI0_BASE+SSI_O_CR0) = (HWREG(SSI0_BASE+SSI_O_CR0)
					& (~(SSI_CR0_SPH | SSI_CR0_SPO))) | (x << 6);
		break;
	case 'g':
		x = hexToNibble(MAP_UARTCharGet(UART0_BASE));
		if (x >= 0)
			MAP_GPIOPinWrite(GPIO_PORTF_BASE,
					(x & 0x4 ? GPIO_PIN_1 : 0) | (x & 0x8 ? GPIO_PIN_3 : 0),
					(x & 0x1 ? GPIO_PIN_1 : 0) | (x & 0x2 ? GPIO_PIN_3 : 0));
		break;
	case 's':
		c = MAP_UARTCharGet(UART0_BASE);
		if (c == '*')
			x = readNumberFromUART('*');
		else
			x = mapSerialSpeed(c);
		if (x > 0) {
			MAP_SSIDisable(SSI0_BASE);
			MAP_SSIConfigSetExpClk(SSI0_BASE, MAP_SysCtlClockGet(),
					(HWREG(SSI0_BASE+SSI_O_CR0) >> 6) & 3,
					SSI_MODE_MASTER, x, 8);
			MAP_SSIEnable(SSI0_BASE);
		}
		break;
	case 'F':	// switch uart mode between bin and hex
		switch (MAP_UARTCharGet(UART0_BASE)) {
		case 'b':
		case 'B':
			serialMode = bin;
			break;
		case 'h':
		case 'H':
			serialMode = hex;
			break;
		}
		break;
	case 'S':
		c = MAP_UARTCharGet(UART0_BASE);
		if (c == '*')
			x = readNumberFromUART('*');
		else
			x = mapSerialSpeed(c);
		if (x > 0) {
			MAP_UARTDisable(UART0_BASE);
			MAP_UARTConfigSetExpClk(UART0_BASE, MAP_SysCtlClockGet(), x,
					(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
					UART_CONFIG_PAR_NONE));
			MAP_UARTEnable(UART0_BASE);
		}
		break;
	case 'C':
		c = MAP_UARTCharGet(UART0_BASE);
		if (c != 'C' && c != 'f' && c != 'F' && c != 'g' && c != 's'
				&& c != 'S')
			commandChar = c;
		break;
	}
}

void uart0_int_handler(void) {
	unsigned char c;
	int d1, d2;
	unsigned long status = MAP_UARTIntStatus(UART0_BASE, false);
	while (MAP_UARTCharsAvail(UART0_BASE)) {
		c = MAP_UARTCharGet(UART0_BASE);
		if (c == commandChar)
			handle_command();
		else
			switch (serialMode) {
			case bin:
				MAP_SSIDataPut(SSI0_BASE, c);
				break;
			case hex:
				d1 = hexToNibble(c);
				if (d1 < 0)
					break;
				c = MAP_UARTCharGet(UART0_BASE);
				if (c == commandChar) {
					handle_command();
					break;
				}
				d2 = hexToNibble(c);
				if (d2 < 0)
					break;
				MAP_SSIDataPut(SSI0_BASE, (d1 << 4) | d2);
				break;
			}
	}
	MAP_UARTIntClear(UART0_BASE, status);
}
void ssi0_int_handler(void) {
	unsigned long data;
	unsigned long status = MAP_SSIIntStatus(SSI0_BASE, false);
	while (MAP_SSIDataGetNonBlocking(SSI0_BASE, &data)) {
		switch (serialMode) {
		case bin:
			MAP_UARTCharPut(UART0_BASE, (unsigned char) (data & 0xFF));
			break;
		case hex:
			MAP_UARTCharPut(UART0_BASE,
					nibbleToHex((unsigned char) (data >> 4)));
			MAP_UARTCharPut(UART0_BASE, nibbleToHex((unsigned char) (data)));
			break;
		}
	}
	MAP_SSIIntClear(SSI0_BASE, status);
}
