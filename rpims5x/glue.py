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

"""
MS5541C pressure sensor library.
"""

import ctypes as ct

class Sensor(object):
    """
    Pressure sensor communication interface.
    """
    def __init__(self):
        """
        Create pressure sensor communication interface.
        """
        self._lib = ct.CDLL('librpims5x.so.0')
        self._lib.rpims5x_init()
        self._p_value = ct.c_ushort()
        self._t_value = ct.c_ushort()


    def read(self):
        """
        Read pressure and temperature from sensor.
        """
        px = self._p_value
        tx = self._t_value
        self._lib.rpims5x_read(ct.byref(px), ct.byref(tx))
        return px.value, tx.value


    def read_avg(self, n):
        """
        Read average pressure and temperature values from sensor.

        The MS5541C datasheet recommends 4 or 8 reads for stable 1mbar
        pressure resolution.

        :Parameters:
         n
            Amount of reads.
        """
        px = self._p_value
        tx = self._t_value
        self._lib.rpims5x_read_avg(ct.byref(px), ct.byref(tx), 4)
        return px.value, tx.value


# vim: sw=4:et:ai
