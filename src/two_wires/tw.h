#ifndef _I2C_H
#define _I2C_H

#include "../utils/utils.h"


void init_two_wires();

ERROR
write_two_wires_sync(uint8_t local_slave_address, uint8_t local_data[], uint8_t local_data_len);

void  write_two_wires_start(uint8_t local_slave_address,
                            uint8_t local_data[],
                            uint8_t local_data_len);
ERROR write_two_wires_join();

void scan_i2c_addresses();

#endif
