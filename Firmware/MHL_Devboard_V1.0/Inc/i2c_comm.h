/*
 * i2c_comm.h
 *
 *  Created on: July 3, 2019
 *      Author: Manolis Lykos
 *
 *  This file contains definitions of the commands
 *  and functions needed to control the I/O Expanders
 */

#ifndef I2C_COMM_H_
#define I2C_COMM_H_



#endif /* I2C_COMM_H_ */


#include <stdint.h>
#include <main.h>


/*--------------- Expander Commands ----------------*/
#define INPUT_PORT_0 0x00
#define INPUT_PORT_1 0x01

#define OUTPUT_PORT_0 0x02
#define OUTPUT_PORT_1 0x03

#define CONFIGURATION_PORT_0 0x06
#define CONFIGURATION_PORT_1 0x07

#define OUTPUT_DRIVE_STRENGTH_0_0 0x40
#define OUTPUT_DRIVE_STRENGTH_0_1 0x41
#define OUTPUT_DRIVE_STRENGTH_1_0 0x42
#define OUTPUT_DRIVE_STRENGTH_1_1 0x43

#define PUPD_ENABLE_REG_0 0x46
#define PUPD_ENABLE_REG_1 0x47

#define PUPD_SELECTION_REG_0 0x48
#define PUPD_SELECTION_REG_1 0x49

#define INTERRUPT_MASK_REG_0 0x4a
#define INTERRUPT_MASK_REG_1 0x4b

#define INTERRUPT_STATUS_REG_0 0x4c
#define INTERRUPT_STATUS_REG_1 0x4d

#define OUTPUT_PORT_CONFIG_REG 0x4f



/*-------------- Interface Functions ----------------*/
int initialize_expanders(I2C_HandleTypeDef hi2c);
int configure_outputs(I2C_HandleTypeDef hi2c, uint16_t outputs);
int clear_all_outputs(I2C_HandleTypeDef hi2c);
int read_input(I2C_HandleTypeDef hi2c ,uint8_t num);

int configure_pupd_enable_registers(I2C_HandleTypeDef hi2c, uint8_t DevAddr ,uint16_t pupd_pins);
int configure_pupd_selection_registers(I2C_HandleTypeDef hi2c, uint8_t DevAddr ,uint16_t pupd_pins);

uint16_t read_all_inputs(I2C_HandleTypeDef hi2c);
uint16_t read_all_outputs(I2C_HandleTypeDef hi2c);
uint16_t read_interrupt_status_register(I2C_HandleTypeDef hi2c);
uint16_t read_interrupt_mask_register(I2C_HandleTypeDef hi2c);
