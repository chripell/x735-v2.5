#!/usr/bin/env python
import RPi.GPIO as GPIO
import time

servo = 13

GPIO.setmode(GPIO.BCM)
GPIO.setwarnings(False)
GPIO.setup(servo, GPIO.OUT)
pwm = GPIO.PWM(servo, 25000)
pwm.start(50)

while True:
    # get CPU temp
    file = open("/sys/class/thermal/thermal_zone0/temp")
    temp = float(file.read()) / 1000.00
    temp = float('%.2f' % temp)
    file.close()

    if(temp > 30):
        pwm.ChangeDutyCycle(40)

    if(temp > 50):
        pwm.ChangeDutyCycle(50)

    if(temp > 55):
        pwm.ChangeDutyCycle(75)

    if(temp > 60):
        pwm.ChangeDutyCycle(90)

    if(temp > 65):
        pwm.ChangeDutyCycle(100)

    if(temp < 30):
        pwm.ChangeDutyCycle(0)

    time.sleep(1)
