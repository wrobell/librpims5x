#if !defined (_RPIMS5X_H_)
#define _RPIMS5X_H_

#include <stdint.h>

/*!
 * Initialize sensor.
 */
int rpims5x_init();

/*!
 * Read pressure and temperature information from sensor.
 */
int rpims5x_read(uint16_t *pressure, uint16_t *temp);

/*!
 * Averaged read pressure and temperature information from sensor.
 *
 * The information is read k-times and averaged (the MS5541C datasheet
 * suggest k to be 4 or 8).
 */
int rpims5x_read_avg(uint16_t *pressure, uint16_t *temp, char k);

#endif /* _RPIMS5X_H_ */

// vim: sw=4:et:ai
