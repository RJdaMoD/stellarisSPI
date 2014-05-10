stellarisSPI
============

Turn your Stellaris launchpad into an uart-spi bridge.

This project emerged from the need to replace an atmega328p
used in a chinese clone of the AVR Transistortester (see
 https://www.mikrocontroller.net/articles/AVR_Transistortestester )
which i had toasted with a charged capacitor.
Getting a new avr was no problem, but i had to program it and had no
programmer or spare arduino. So i read around and found that an avr
may be serially programmed via spi, and also found avrdude doing exactly
this with numerous programming hardware.
As i had a Stellaris launchpad lying around which i had gotten cheap
on launch and also did some programming with it, i wondered if it
would be possible to use this as programmer. Since it has a number
of hardware spi interfaces and an debug link with integrated serial
communication, it turned out to be quite easy to implement a bridge
which streams data from the uart to the spi and back.
I also forked avrdude from kcuzner because he already implemented
an interface for avrdude that directly uses available spi interfaces,
and my implementation would be quite similiar. You can find this fork
at https://github.com/RJdaMoD/avrdude .
Of course you may use this also for other spi communications with other
ic's like flash, fpga...
But my transistortester is working again:)


Usage
=====
On connection, the icdi (integrated control and debug interface, i think)
of a launchpad offers a serial connection to the target processor
which your system might offer as ttyACM0 (linux) or COMx (windows), MacOS
should be similiar. You can connect to this interface with the standard
serial connection parameters 115200 baud, 8 bit, no parity and 1 stop bit,
for example with gtkterm/hyperterm or similiar terminal software.
stellarisSPI offers hex- and bin-mode, where the first is the default on
powerup. In hex-mode, for each pair of characters forming a hexadecimal
8bit number the corresponding byte is sent on the spi-interface. The byte
that is read back, is transferred to the host as hexadecimal number too.
The interface may also be operated in bin-mode, where all data is transferred
binary.
The spi-interface is hardcoded to 8bit motorola master mode. CPOL and CPHA
default to zero (which is correct for avr serial flashing), but may be
changed by command. The clockspeed of the spi-interface is derived from
the processor clock and defaults to 115200, but may also be changed.
The used pins are PA2 = SCL, PA4 = MISO and PA5 = MOSI.
The two pins PF1 and PF3 may be used as digital outputs which are controlled
by commands. As they also drive two cathodes of the onboard rgb-led,
their usage may be monitored.
The command interface of stellarisSPI is a simple escape-key interface
where a command character, defaulting to '$', lets the processor interpret
the next character send from the host as command. If this is again the
command character an dthe interface is in bin-mode, this character is
actually sent over the spi. The supported commands are the following:
 'f':	If the next character is '0', '1', '2' or '3', the number is
	interpreted as two bits where CPOL is the lower bit, and CPHA
	the upper bit. The spi-interface is then set according to this
	configuration.
 'F':	If the follwing character is a 'b' or 'B', switch to bin-mode.
	If it is 'h' or 'H', switch to hex-mode.
 'g':	If the next character is a hexadecimal digit, it iss interpreted
	as four bits where the upper two bits form a bit mask, and the lower
	two bits the values for the two output pins PF1 (lower bit) and
	PF3 (upper bit). Thus 'A' means "set PF3 to 1", '4' means "clear PF1"
	etc.
 's':	This changes the speed of the spi-interface. If the next character is
	alpha-numeric, it is interpreted as a speed code where the sequence
	0-9,A-Z,a-z forms indices (A=10,a=36...) that map on the following
	speed table:
		50, 75, 110, 134, 200, 300, 600, 1200, 1800, 2400,   // 0 - 9
		4800, 9600, 19200, 38400, 57600, 115200, 230400,     // A - G
		460800, 500000, 576000, 921600, 1000000, 1152000,    // H - M
		1500000, 2000000, 2500000, 3000000, 3500000, 4000000 // N - S
	If instead a '*' is encountered, the following characters up to the
	next '*' are interpreted as a decimal number which sets the spi-speed.
 'S':	The same as above, but for the uart. As it seems that one can not
	reconfigure the icdi for other serial speeds than 115200, this command
	should not be used.
 'C':	The next character will be set as the command character if it is not
	an already defined command.
If the arguments of the command characters have not the expected form, the
whole character sequence is discarded.

Building
========
The code comes as an eclipse project and was built using the TI driverlib
(though only rom functions are used, so the include files are sufficient)
and the summon arm toolchain. Startup code was taken from
	https://github.com/mroy/stellaris-launchpad-template-gcc
Images are also included as hex and elf, which may be flashed on the
launchpad using openocd or lm4flash.

Licensing
=========
You may do whatever you want the code, but please keep a track to me.
I will not provide any liabilities or warranties.
