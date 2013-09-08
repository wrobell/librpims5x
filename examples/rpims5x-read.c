/*
 * librpims5x - MS5x family pressure sensors library
 *
 * Copyright (C) 2013 by Artur Wroblewski <wrobell@pld-linux.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
