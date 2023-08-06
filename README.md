# A simulator for disease spread

This is a disease spread simulator based on Jacob Sorber's [video](https://www.youtube.com/watch?v=zyQXhPUM4hc).

Some differences:

1. This version is implemented in C (instead of C++)
2. This version uses [raylib](https://www.raylib.com/) (instead of GTK).
3. This version implements "No restriction" and "Stay at home" type of policies.


## Usage

`
source setenv.sh
sh build.sh
./simulator
`

![example](https://github.com/rzavalet/DiseaseSimulator/blob/main/simulation.gif)

## Dependencies

This simulator requires the [raylib](https://www.raylib.com/) library. Please see the setenv.sh script.


## Parameters

These are some of the parameters that you can play with:

- POPULATION\_SIZE
- INFECTION\_DURATION
- INFECTION\_PROBABILITY
- NORMAL\_FATALITY\_RATE
- SATURATED\_FATALITY\_RATE
- INFECTION\_PROXIMITY
- SATURATION\_THRESHOLD
