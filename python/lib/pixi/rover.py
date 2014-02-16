from __future__ import print_function

from pixitools.pixi import pixiGpioWritePin, pwmWritePin
from pixitools.pixix import globalSpi

spi = globalSpi()

forwards =  0
reverse  =  1

motorGpio = 2
motorGpioPin = 0
pwmBr = 4
pwmFr = 5
pwmBl = 6
pwmFl = 7

def enableMotor (enable = True):
	pixiGpioWritePin (spi, motorGpio, motorGpioPin, enable)

def pwmSet (pin, value):
#	print ('pwmWritePin', pin, hex (value))
	pwmWritePin (spi, pin, value)

motorFlags = [0, 0x8000] # forward, reverse

def moveRover (leftSide, rightSide, speed):
	pwm = (speed * 1023 / 100) & 0x000003ff
	print ('moveRover', leftSide, rightSide, speed, pwm, hex (pwm))
	# each side must be synchronised, but each side the motors are opposed:
	fl = pwm + motorFlags[    leftSide ]
	bl = pwm + motorFlags[not leftSide ]
	fr = pwm + motorFlags[    rightSide]
	br = pwm + motorFlags[not rightSide]

	pwmSet (pwmFr, fr)
	pwmSet (pwmBl, bl)
	pwmSet (pwmBr, br)
	pwmSet (pwmFl, fl) # Must write to this pin last!

def moveRoverX (speedL, speedR):
	pwmL = (abs(speedL) * 1023 / 100) & 0x000003ff
	pwmR = (abs(speedR) * 1023 / 100) & 0x000003ff
	print ('moveRoverX', speedL, speedR, hex (pwmL), hex (pwmR))
	# each side must be synchronised, but each side the motors are opposed:
	fl = pwmL + motorFlags[speedL < 0]
	bl = pwmL + motorFlags[speedL > 0]
	fr = pwmR + motorFlags[speedR < 0]
	br = pwmR + motorFlags[speedR > 0]

	pwmSet (pwmFr, fr)
	pwmSet (pwmBl, bl)
	pwmSet (pwmBr, br)
	pwmSet (pwmFl, fl) # Must write to this pin last!

def moveForward  (speed):
	moveRover (forwards, forwards, speed)
def moveBackward (speed):
	moveRover (reverse , reverse , speed)
def turnLeft     (speed):
	moveRover (reverse , forwards, speed)
def turnRight    (speed):
	moveRover (forwards, reverse , speed)
