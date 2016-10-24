#ifndef _GPIO_H_INCLUDED_
#define _GPIO_H_INCLUDED_
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include "GPIOcontrol.h"

/*
#define TEST_PIN_GPIO_BASE	GPIO0_ADDRESS	// Use GPIO0(31) for debugging
#define TEST_PIN_MASK		(1 << 31)
#define TEST_PIN2_MASK		(1 << 26)

#define USERLED3_GPIO_BASE  GPIO1_ADDRESS // GPIO1(24) is user LED 3
#define USERLED3_PIN_MASK   (1 << 24)
*/

static const uint32_t GPIO_SIZE =  0x198;
static const uint32_t GPIO_DATAIN = (0x138 / 4);
static const uint32_t GPIO_CLEARDATAOUT = (0x190 / 4);
static const uint32_t GPIO_SETDATAOUT = (0x194 / 4);
static const uint32_t GPIO_ADDRESSES[4] = {
	0x44E07000,
	0x4804C000,
	0x481AC000,
	0x481AE000,
};

class Gpio{
public:
	Gpio():
	fd(-1),
	oldPin(-1),
	{};

	~Gpio(){
		close(fd);
	}

	/**
	 * Opens a GPIO pin.
	 *
	 * @param pin the GPIO pin ( 0 <= pin < 128)
	 * @param direction one of INPUT or OUTPUT
	 *
	 * @return 0 if success, -1 otherwise;
	 */
	int open(unsigned int pin, unsigned int direction){
		if(pin >= 128){
			return -1;
		}
		if(fd != -1){
			gpio_unexport(oldPin);
			close(fd);
		}
		oldPin = pin;
		// gpio_export can fail if the pin has already been exported.
		// We don't care.
		gpio_export(pin);
		if(gpio_set_dir(pin, direction) < 0){
			return -1;
		}
		fd = ::open("/dev/mem", O_RDWR);
		int bank = pin / 32;
		pin = pin - bank * 32;
		pinMask = 1 << pin;
		uint32_t gpioBase = GPIO_ADDRESSES[bank];
		printf("base: %#x, bank: %d, pin: %d\n", gpioBase, bank, pin);
		gpio = (uint32_t *)mmap(0, GPIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, gpioBase);
		if(gpio == MAP_FAILED){
			fprintf(stderr, "Unable to map GPIO pin %u\n", pin);
			return -2;
		}
		// actually use the memmapped memory,
		// to avoid mode switches later
		int value = read();
		set();
		clear();
		// then reset the to its original value
		write(value);
		return 0;
	}
	
	/**
	 * Read the GPIO value.
	 * @return the GPIO value
	 */
	bool read(){
		return (gpio[GPIO_DATAIN] & pinMask);
	}

	/**
	 * Set the output to 1.
	 */
	void set(){
		gpio[GPIO_SETDATAOUT] = pinMask;
	}

	/** Clear the output
	 */
	void clear(){
		gpio[GPIO_CLEARDATAOUT] = pinMask;
	}

	/**
	 * Write an output value
	 * @param value the value to write
	 */
	void  write(bool value){
		if(value){
			set();
		} else {
			clear();
		}
	}
private:
	int oldPin;
	int fd;	
	uint32_t pinMask;
	uint32_t* gpio;
};
#endif /* _GPIO_H_INCLUDED_ */
