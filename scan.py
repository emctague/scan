import RPi.GPIO as GPIO
import time
import sys
GPIO.setmode(GPIO.BCM)

indlevel=0

def step(n, inverse = False):
    STEPPERS = [13, 19, 26, 6]
    for s in STEPPERS:
        GPIO.setup(s, GPIO.OUT)
    for i in range(0, n):
        ind = i % 4
        indd= (i + 1) % 4
        if inverse:
            ind = (4 - i) % 4
            indd = (3 - i) % 4

        GPIO.output(STEPPERS[ind], True)
        GPIO.output(STEPPERS[indd], True)
        time.sleep(0.05)
        GPIO.output(STEPPERS[ind], False)
        GPIO.output(STEPPERS[indd], False)

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

PIN_TRIG=23
PIN_ECHO=24

@task("Collecting Sample", "cm")
def getDistance():
    GPIO.setup(PIN_TRIG, GPIO.OUT)
    GPIO.setup(PIN_ECHO, GPIO.IN)
    GPIO.output(PIN_TRIG, False)
    time.sleep(2)
    GPIO.output(PIN_TRIG, True)
    time.sleep(0.000005)
    GPIO.output(PIN_TRIG, False)

    while GPIO.input(PIN_ECHO)==0:
        pulse_start = time.time()

    while GPIO.input(PIN_ECHO)==1:
        pulse_end = time.time()

    pulse_dur = pulse_end - pulse_start
    return pulse_dur * 17150.0

@task("Collecting Multiple Samples", "cm")
def getLongDistance():
    NUMPOLLS=3
    total=0
    for i in range(NUMPOLLS):
        total += getDistance()
    return total / NUMPOLLS

NUM_ANGLES=50
NUM_STEPS=30

@task("Poll All Data", "complete")
def pollData():
    for i in range(0, NUM_ANGLES):
        print(int(getLongDistance() * 100) / 100.0)
        sys.stdout.flush()
        step(NUM_STEPS)
    return True

@task("Return to Start Position", "complete")
def returnStart():
    step(NUM_ANGLES * NUM_STEPS, True)
    return True


pollData()
step(360 * 5, True)
