#
# librpims5x - MS5x family pressure sensors library
#
# Copyright (C) 2013 by Artur Wroblewski <wrobell@pld-linux.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import time
from datetime import datetime

import rpims5x

sensor = rpims5x.Sensor()
while True:
    # single read from sensor
    pressure, temp = sensor.read()
    print('{} (1): {}mbar {}C'.format(datetime.now(), pressure, temp / 10))

    time.sleep(0.5)

    # multiple read from sensor - according to the MS5541C datasheet 4 or 8
    # values should be read to get 1mbar pressure resolution accuracy
    pressure, temp = sensor.read_avg(4)
    print('{} (4): {}mbar {}C'.format(datetime.now(), pressure, temp / 10))

    time.sleep(0.5)

# vim: sw=4:et:ai
