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

#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

#ifdef RPIMS5X_DEBUG
#include <stdio.h>
#define DEBUG_LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

#define BCM2708_PERI_BASE  0x20000000
#define GPIO_BASE          (BCM2708_PERI_BASE + 0x200000)
#define CLOCK_BASE         (BCM2708_PERI_BASE + 0x101000)
#define FSEL_OFFSET        0   // 0x0000
#define SET_OFFSET         7   // 0x001c / 4
#define ALT0    2
#define PAGE_SIZE  (4*1024)
#define BLOCK_SIZE (4*1024)
#define GP_CLK0_CTL *(clk_map + 0x1C)
#define GP_CLK0_DIV *(clk_map + 0x1D)
#define OSCILLATOR_FREQ 19000000 // Hz

#define CLOCK_FREQ         32768

#define ERROR_OK      0
#define ERROR_SETUP   -1
#define ERROR_WRITE   -1
#define ERROR_READ    -2

static volatile uint32_t *gpio_map;
static volatile uint32_t *clk_map;

static uint16_t clbw_1 = 0;
static uint16_t clbw_2 = 0;
static uint16_t clbw_3 = 0;
static uint16_t clbw_4 = 0;
static uint16_t clbw_5 = 0;
static uint16_t clbw_6 = 0;
static int spi_fd;

static int gpio_init() {
    int mem_fd;

    uint8_t *gpio_mem;
    uint8_t *clk_mem;

    if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0)
        return ERROR_SETUP;

    // Allocate MAP block
    if ((gpio_mem = malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL)
        return ERROR_SETUP;
    if ((clk_mem = malloc(BLOCK_SIZE + (PAGE_SIZE - 1))) == NULL)
        return ERROR_SETUP;

    // Make sure pointer is on 4K boundary
    if ((uint32_t)gpio_mem % PAGE_SIZE)
        gpio_mem += PAGE_SIZE - ((uint32_t)gpio_mem % PAGE_SIZE);
    if ((uint32_t)clk_mem % PAGE_SIZE)
        clk_mem += PAGE_SIZE - ((uint32_t)clk_mem % PAGE_SIZE);

    gpio_map = (uint32_t*)mmap((caddr_t)gpio_mem, BLOCK_SIZE,
            PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, mem_fd, GPIO_BASE);
    clk_map = (uint32_t *)mmap( (caddr_t)clk_mem, BLOCK_SIZE,
            PROT_READ | PROT_WRITE, MAP_SHARED|MAP_FIXED, mem_fd, CLOCK_BASE);

    // Make sure pointer is on 4K boundary
    if ((uint32_t)gpio_mem % PAGE_SIZE)
        gpio_mem += PAGE_SIZE - ((uint32_t)gpio_mem % PAGE_SIZE);
    if ((uint32_t)clk_mem % PAGE_SIZE)
        clk_mem += PAGE_SIZE - ((uint32_t)clk_mem % PAGE_SIZE);

    return ERROR_OK;
}

static void gpio_clock_start() {
    char gpio = 4;
    int offset;
    int shift;
    int divi, divf;

    // stop clock
    GP_CLK0_CTL = 0x5A000001;
    usleep(10);

    // set ALT0
    offset = FSEL_OFFSET + (gpio / 10);
    shift = (gpio % 10) * 3;
    *(gpio_map + offset) = (*(gpio_map + offset) & ~(7 << shift)) | (4) << shift;

    // set clock frequency
    divi = OSCILLATOR_FREQ / CLOCK_FREQ;
    divf = ((OSCILLATOR_FREQ % CLOCK_FREQ) * 4096) / CLOCK_FREQ;
    GP_CLK0_DIV = 0x5A000000 | (divi & 0xFFF) << 12 | (divf & 0xFFF);
    usleep(10);

    // start clock
    GP_CLK0_CTL = 0x5A000011;
    usleep(10);

    offset = SET_OFFSET;
    *(gpio_map + offset) = 1 << 4;
}

static int sensor_reset() {
    int r;
    r = write(spi_fd, (char[]){0x15, 0x55, 0x40}, 3);
    if (r != 3)
        return ERROR_WRITE;
    return ERROR_OK;
}

static int sensor_calib_read() {
    uint8_t buffer[2];

    uint16_t w1, w2, w3, w4;

    // FIXME: use sensor_value_raw?
    write(spi_fd, (char[]){0x1d, 0x50}, 2);
    read(spi_fd, buffer, 2);
    w1 = buffer[0] << 8 | buffer[1];
    DEBUG_LOG("rpims5x: calibration w1 %d\n", w1);

    write(spi_fd, (char[]){0x1d, 0x60}, 2);
    read(spi_fd, buffer, 2);
    w2 = buffer[0] << 8 | buffer[1];
    DEBUG_LOG("rpims5x: calibration w1 %d\n", w2);

    write(spi_fd, (char[]){0x1d, 0x90}, 2);
    read(spi_fd, buffer, 2);
    w3 = buffer[0] << 8 | buffer[1];
    DEBUG_LOG("rpims5x: calibration w1 %d\n", w3);

    write(spi_fd, (char[]){0x1d, 0xa0}, 2);
    read(spi_fd, buffer, 2);
    w4 = buffer[0] << 8 | buffer[1];
    DEBUG_LOG("rpims5x: calibration w1 %d\n", w4);

    clbw_1 = w1 >> 3 & 0x1fff;
    clbw_2 = ((w1 & 0x7) << 10) | ((w2 >> 6) & 0x3ff);
    clbw_3 = (w3 >> 6) & 0x3ff;
    clbw_5 = ((w2 & 0x3f) << 6) | (w3 & 0x3f);
    clbw_4 = (w4 >> 7) & 0x7ff;
    clbw_6 = w4 & 0x7f;
    DEBUG_LOG("rpims5x: calibration clbw_1 %d\n", clbw_1);
    DEBUG_LOG("rpims5x: calibration clbw_2 %d\n", clbw_2);
    DEBUG_LOG("rpims5x: calibration clbw_3 %d\n", clbw_3);
    DEBUG_LOG("rpims5x: calibration clbw_4 %d\n", clbw_4);
    DEBUG_LOG("rpims5x: calibration clbw_5 %d\n", clbw_5);
    DEBUG_LOG("rpims5x: calibration clbw_6 %d\n", clbw_6);

    return ERROR_OK;
}

// FIXME: static int sensor_value_raw(const char cmd[], uint16_t *w) {
static uint16_t sensor_value_raw(const char cmd[]) {
    uint8_t buffer[2];
    int r;

    r = write(spi_fd, cmd, 2);
    if (r != 2)
        return ERROR_WRITE;

    usleep(35000);
    r = read(spi_fd, buffer, 2);
    if (r != 2)
        return ERROR_READ;

    return buffer[0] << 8 | buffer[1];
}

int rpims5x_init() {
    int r;
    uint8_t mode = SPI_MODE_1;
    uint8_t bits_per_word = 0;
    uint8_t msb = 0;


    r = gpio_init();
    if (r != ERROR_OK)
        return ERROR_SETUP;
    gpio_clock_start();
    DEBUG_LOG("rpims5x: gpio clock started\n");

    spi_fd = open("/dev/spidev0.0", O_RDWR);
    r = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    if (r == -1)
        return ERROR_SETUP;
    r = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits_per_word);
    if (r == -1)
        return ERROR_SETUP;
    r = ioctl(spi_fd, SPI_IOC_WR_LSB_FIRST, &msb);
    if (r == -1)
        return ERROR_SETUP;
    DEBUG_LOG("rpims5x: spi initialized\n");

    r = sensor_reset();
    if (r != ERROR_OK)
        return ERROR_SETUP;

    r = sensor_calib_read();
    if (r != ERROR_OK)
        return ERROR_SETUP;

    // FIXME: close(spi_fd);
    DEBUG_LOG("rpims5x: setup done\n");
    return ERROR_OK;
}

int rpims5x_read(uint16_t *pressure, uint16_t *temp) {
    int32_t d1;
    int32_t d2;
    int32_t ut1;
    int32_t dt;
    int32_t dt2;
    int32_t off;
    int32_t sens;
    int32_t temp_r;

    d2 = sensor_value_raw((char[]){0x0f, 0x20});
    d1 = sensor_value_raw((char[]){0x0f, 0x40});

    ut1 = (clbw_5 << 3) + 10000;
    dt = d2 - ut1;
    *temp = 200 + ((dt * (clbw_6 + 100)) >> 11);
    off = clbw_2 + (((clbw_4 - 250) * dt) >> 12) + 10000;
    sens = (clbw_1 / 2) + (((clbw_3 + 200) * dt) >> 13) + 3000;
    *pressure = (sens * (d1 - off) >> 12) + 1000;
    dt2 = dt - (((dt >> 7) * (dt >> 7)) >> 3);
    temp_r = 200 + (dt2 * (clbw_6 + 100) >> 11);

    DEBUG_LOG("rpims5x: d1 %d\n", d1);
    DEBUG_LOG("rpims5x: d2 %d\n", d2);
    DEBUG_LOG("rpims5x: pressure %d\n", *pressure);
    DEBUG_LOG("rpims5x: temp %d %d\n", *temp, temp_r);

    *temp = temp_r;

    return ERROR_OK;
}

int rpims5x_read_avg(uint16_t *pressure, uint16_t *temp, char k) {
    int i;
    uint16_t pv;
    uint16_t tv;
    uint32_t sum_pv;
    uint32_t sum_tv;

    sum_pv = 0;
    sum_tv = 0;
    for (i = 0; i < k; i++) {
        rpims5x_read(&pv, &tv);
        sum_pv += pv;
        sum_tv += tv;
    }

    *pressure = sum_pv / k;
    *temp = sum_tv / k;
}

// vim: sw=4:et:ai
