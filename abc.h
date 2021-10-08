#ifndef ABC_H
#define ABC_H


#include <stdint.h>

uint32_t message_ticker = 0;
uint8_t payload[19];
bool is_big_endian = false;
uint8_t END = 0;
int16_t ROTATE = 0;
uint8_t SKIP = 0;

bool ABC0 = false;
bool ABC1 = false;
bool ABC2 = false;

uint8_t data[12];
uint8_t dataDUPLICATE[12];
uint8_t dataOFFSET[32];


typedef struct {
    uint32_t abc0_count;
    uint32_t abc1_count;
    uint32_t abc2_count;
    uint32_t codeword;
} ABC_Data;


extern ABC_Data g_abc_data;


/*******************************************************************************
 * This function initializes/resets the module to begin processing data.
 *
 * Note: This funciton will be called between successive tests, so ensure
 *       that it clears all state data to begin processing from scratch.
 ******************************************************************************/
void init_abc(void) {
    uint32_t abc_seed = 0;
    ABC_Data.codeword = 0;

    ABC_Data.abc0_count = 0;
    ABC_Data.abc1_count = 0;
    ABC_Data.abc2_count = 0;

    message_ticker = 0;
    fill(payload) = 0;

    uint8_t END = 0;
    int16_t ROTATE = 0;
    uint8_t SKIP = 0;
    is_big_endian = false;
}

/*******************************************************************************
 * This function will be called after each character is received from the UART.
 *
 * This function is responsible for maintaining the fields in g_abc_data.
 *
 * Args:
 *   uint8_t byte -> one character received from the UART
 ******************************************************************************/

bool pass_CRC(uint8_t payload[] , bool ABC0 = false, bool ABC1 = false, bool ABC2 = false) {
    if (ABC0) {
        return xor(payload[5], payload[6], payload[4]) == 0xAA
    }

    if (ABC1) {
        return xor(payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[4]) == 0xAA
    }

    if (ABC2) {
        return xor(payload[5], payload[6], payload[7], payload[8], payload[9], payload[10], payload[11], payload[12], payload[13], payload[14], payload[15], payload[16], payload[17], payload[18], payload[19], payload[4]) == 0xAA
    }
}

void data_process(uint8_t payload[] , bool ABC0 = false, bool ABC1 = false, bool ABC2 = false) {
    if (ABC0) {
        ABC_Data.abc0_count++;
        ABC_Data.codeword++;
    } else if (ABC1) {
        ABC_Data.abc1_count++;

        if (payload[7] == 0) {
            is_big_endian = false;
            END = 0;
        } else {
            is_big_endian = true;
            END = payload[7];
        }

        ROTATE = payload[8] + payload[9];

        SKIP = payload[10];

        is_big_endian = false;
    } else if (ABC2) {
        ABC_Data.abc2_count++;


        for (int i = 8; i < 19; i++) {
            data[i-8] = payload[i];
            dataDUPLICATE[i-8] = payload[i];
        }

        if (ROTATE != 0) {

            for (int i = 0; i < 12; i++) {
                int rotated_i = (-ROTATE) + i;

                if (rotated_i < 0) {
                    rotated_i += 12;
                }

                data[i] = dataDUPLICATE[rotated_i];
            }
        }

        uint8_t dataBYTES[4];

        for (int i = 0; i < 4; i++) {
            while (i == 1) {
                for (int j = 0; j < 8; j++) {
                    dataOFFSET[j] = data[j];
                    dataBYTES[i] += dataOFFSET[j];
                }
            }

            while (i == 2) {
                for (int j = 8; j < 16; j++) {
                    dataOFFSET[j] = data[j + SKIP];
                    dataBYTES[i] += dataOFFSET[j];
                }
            }

            while (i == 3) {
                for (int j = 16; j < 24; j++) {
                    dataOFFSET[j] = data[j + (2 * SKIP)];
                    dataBYTES[i] += dataOFFSET[j];
                }
            }


            while (i == 4) {
                for (int j = 24; j < 32; j++) {
                    dataOFFSET[j] = data[j + (3 * SKIP)];
                    dataBYTES[i] += dataOFFSET[j];
                }
            }
        }





        uint32_t newval;

        if (END == 0) {
            /* Little Endian */
            newval = dataBYTES[0] + dataBYTES[1] + dataBYTES[2] + dataBYTES[3]
        } else {
            for (int i = 0 + END; i < 4 + END; i++) {
                int rotated_i = i;

                if (rotated_i > 4) {
                    rotated_i -= END;
                }

                newval += dataBYTES[rotated_i];
            }
        }

        abc_seed = xor(abc_seed, newval);

    }
}


void process_abc(uint8_t byte) {
    if (message_ticker == 0) {
        fill(payload) = 0;




        ABC0 = false;
        ABC1 = false;
        ABC2 = false;
    }


    message_ticker++;

    uint8_t a_binary = 65;
    uint8_t b_binary = 66;
    uint8_t c_binary = 67;
    uint8_t exclamation_binary = 33;



    if (message_ticker == 1) {
        if (byte == a_binary) {
            payload[0] = byte;
        } else {
            message_ticker = 0;
        }
    }

    if (message_ticker == 2) {
        if (byte == b_binary) {
            payload[1] = byte;
        } else {
            message_ticker = 0;
        }
    }

    if (message_ticker == 3) {
        if (byte == c_binary) {
            payload[2] = byte;
        } else {
            message_ticker = 0;
        }
    }

    if (message_ticker == 4) {
        if (byte == exclamation_binary) {
            payload[3] = byte;
        } else {
            message_ticker = 0;
        }
    }

    if (message_ticker == 5) {

        /* TODO: Implement CRC Check */
        payload[4] = byte;
    }

    if (message_ticker == 6) {
        if (byte == 1 || byte == 4 || byte == 12) {
            payload[5] = byte;
            continue;
        } else {
            message_ticker = 0;
        }
    }

    if (message_ticker == 7) {
        if (byte == 0) {
            ABC0 = true;
            payload[6] = byte;
                if (pass_CRC(payload, ABC0, ABC1, ABC2)) {
                    data_process(payload, ABC0, ABC1, ABC2);
                }
                message_ticker = 0;
                break;
            }
        } else if (byte == 1) {
            ABC1 = true;
            payload[6] = byte;
        } else if (byte == 2) {
            ABC2 = true;
            payload[6] = byte;
        } else {
            message_ticker = 0;
        }
    }

    if (ABC1 == true) {
        if (message_ticker > 7 && message_ticker <= 11) {
            payload[message_ticker] = byte;
        }

            if (!pass_CRC(payload, ABC0, ABC1, ABC2)) {
            message_ticker = 0;
            break;
            } else {
                data_process(payload, ABC0, ABC1, ABC2);
            }
        }
    }

    if (ABC2 == true) {
        while (message_ticker > 7 && message_ticker < 20) {
            payload[message_ticker] = byte;

            if (message_ticker == 19 && !pass_CRC(payload, ABC0, ABC1, ABC2)) {
                message_ticker = 0;
                break;
            } else {
                data_process(payload, ABC0, ABC1, ABC2);
            }
        }
    }
}



#endif

