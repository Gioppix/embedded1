#ifndef _I2C_H
#define _I2C_H

#include "../utils/utils.h"

typedef enum { ALL_GOOD = 0, NO_START_ACK, NO_DATA_ACK, ARBITRATION_LOST } TWO_WIRES_ERR;

void init_two_wires();

TWO_WIRES_ERR
write_two_wires_sync(uint8_t local_slave_address, uint8_t local_data[], uint8_t local_data_len);

void scan_i2c_addresses();

#endif
