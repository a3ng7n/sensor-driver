# sensor-driver

# Pre-reqs

`sudo apt install g++ make socat`

# compiling

- `make clean` / `make distclean`
- `make`

# running

1. `socat PTY,link=/tmp/ttyDRIVER,raw,echo=0 PTY,link=/tmp/ttySIM,raw,echo=0`
2. (in another terminal) `./bin/sim`
3. (in another terminal) `./bin/run_driver`

# result

![example output](./output.png)

# Notes

The structure of the code is broken up into two parts; `drive.cpp` and `sim.cpp`. The classes `SensorDriver` and `SensorSim` within them intentionally look similar.
They are both composed of an `IOInterface`, some number of `MessageCoder`'s, and a handful of action/accessor functions.

The functions within `SensorDriver` are fairly low-level, but are _mostly_ stateless. The intent would be some state-knowlegable application is instantiates/controls/uses `SensorDriver`.
The driver itself doesn't require state knowledge, I believe; although the functions `init`/`shutdown` were written with a "running"/"not running" state in mind, they could be folded into the action/accessor functions. I didn't do this mostly for simplicity and readability's sake, and because I didn't have a great plan to.

The high level behavior of `SensorSim` comes from the [EPSON G370 IMU](./g370_datasheet.pdf); there's roughly two modes of operation:

1. request-response: you send a command, and receive some result, and;
2. automatic broadcast: the gyro emits data at some prescribed rate

There's a very thin notion of "registers" but there's little-to-no implementation behind them; they're effectively just switch statement cases.
