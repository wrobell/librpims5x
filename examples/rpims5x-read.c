#include <stdio.h>
#include <time.h>

#include "rpims5x.h"

int main() {
    uint16_t pressure, temp;
    time_t t;
    rpims5x_init(); // initialize the library

    while (1) {
        // single read from the sensor
        time(&t);
        rpims5x_read(&pressure, &temp);
        printf("%d (1): %dmbar %.1fC\n", t, pressure, temp / 10.0);
        sleep(1);

        // multiple read from sensor - according to the MS5541C datasheet 4 or
        // 8 values should be read to get 1mbar pressure resolution accuracy
        time(&t);
        rpims5x_read_avg(&pressure, &temp, 4);
        printf("%d (4): %dmbar %.1fC\n", t, pressure, temp / 10.0);
        sleep(1);
    }
    return 0;
}
