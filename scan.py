import RPi.GPIO as GPIO
import time
import sys
GPIO.setmode(GPIO.BCM)

indlevel=0

def task(name, resname):
    def func_decorator(func):
        def func_wrapper(*args, **kwargs):
            global indlevel
            print("%s[Begin %s]" % (("\t" * indlevel), name), file=sys.stderr)
            indlevel += 1
            result = func(*args, **kwargs)
            indlevel -= 1
            print("%s[End %s=%s]" % (("\t" * indlevel), resname, result), file=sys.stderr)
            return result
        return func_wrapper
    return func_decorator

PIN_PING=24
PIN_SERVO=13

@task("Collecting Sample", "cm")
def getDistance():
    GPIO.setup(PIN_PING, GPIO.OUT)
    GPIO.output(PIN_PING, False)
    time.sleep(2)
    GPIO.output(PIN_PING, True)
    time.sleep(0.000005)
    GPIO.output(PIN_PING, False)
    GPIO.setup(PIN_PING, GPIO.IN)

    while GPIO.input(PIN_PING)==0:
        pulse_start = time.time()

    while GPIO.input(PIN_PING)==1:
        pulse_end = time.time()

    pulse_dur = pulse_end - pulse_start
    return pulse_dur * 34000.0 / 2

@task("Collecting Multiple Samples", "cm")
def getLongDistance():
    NUMPOLLS=10
    total=0
    for i in range(NUMPOLLS):
        total += getDistance()
    return total / NUMPOLLS

@task("Poll All Data", "complete")
def pollData():
    GPIO.setup(PIN_SERVO, GPIO.OUT)
    pwm = GPIO.PWM(PIN_SERVO, 1000)

    for i in range(0, 100):
        #print(getLongDistance())
        pwm.start(i)
        time.sleep(0.3)
        pwm.stop()

        # TODO: Step servo

    return true

pollData()
