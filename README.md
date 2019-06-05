# scan

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

```
python3 scan.py | gl/build/scan
```

## Task List

 * [gl] Ensure that existing code works on pi
 * [gl] Implement mesh generation when new depths are retrieved
 * [gl] Generate normals for mesh - maybe hardcoded?
 * [gl] Implement mouselook or arrow-look controls


 * [py] Ensure that existing code works on pi
 * [py] Add servo control and loop for each angle
