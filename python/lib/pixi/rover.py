from __future__ import print_function

from pixitools.pixi import pixiGpioWritePin, pwmWritePin
import logging

log = logging.getLogger(__name__)
info = log.info
debug = log.debug

forwards =  0
reverse  =  1

motorGpio = 2
motorGpioPin = 0
pwmBr = 4
pwmFr = 5
pwmBl = 6
pwmFl = 7

motorEnabled = False

def enableMotor (enable = True):
	info ("Setting motor enable to %s", enable)
	pixiGpioWritePin (motorGpio, motorGpioPin, enable)
	global motorEnabled
	motorEnabled = enable

def pwmSet (pin, value):
#	print ('pwmWritePin', pin, hex (value))
	pwmWritePin (pin, value)

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

from threading import Timer

def stopMotion():
	enableMotor (False)

failsafe = None

def setMotion (x, y, enableFullMotion):
	info ("Move rover request %f, %f", x, y)
	if not motorEnabled:
		enableMotor (True)
	global failsafe
	if failsafe:
		failsafe.cancel();
	failsafe = Timer (1, stopMotion)
	failsafe.start()
	x = int (x)
	y = int (y)
	if enableFullMotion:
		return fullMotion (x, y)
	else:
		return limitedMotion (x, y)

def fullMotion (x, y):
	"Provides entirely independent left/right tracks"
	speedL = y + x
	speedR = y - x
	speedL = min (100, (max (-100, speedL)))
	speedR = min (100, (max (-100, speedR)))
	moveRoverX (speedL, speedR)
	return speedL, speedR

def limitedMotion (x, y):
	"Provides purely forward/reverse or rotational motion"
	speedX = abs (x)
	speedY = abs (y)
	if speedY > speedX:
		# forward/backward
		if y < 0:
			moveBackward (speedY)
		else:
			moveForward (speedY)
		return y, y
	else:
		# rotation
		if x < 0:
			turnLeft (speedX)
		else:
			turnRight (speedX)
		return x, -x
