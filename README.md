# scan

This project was created for a high school final project. It implements a simple renderer and
an ultrasonic depth-scanning script for use with a Raspberry Pi. Note that the renderer does
not seem to work properly when run on the raspberry pi, despite my intention for such to be possible.

## Structure

 * **scan.py** - Raspberry pi-specific depth-scanning script.
 * **gl** - Cross-platform rendering application from depths on stdin.

## Building

```
cd gl
mkdir build
cd build
cmake ..
make
```

## Running


On a pi (will not work properly because the viewing app is broken on the pi:)

```
python3 scan.py | gl/build/scan
```

Elsewhere:

```
gl/build/scan
```

(Just type in values for it to use.)
