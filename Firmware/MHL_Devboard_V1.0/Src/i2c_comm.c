/*
 * i2c_comm.c
 *
 *  Created on: July 3, 2019
 *      Author: Manolis Lykos
 *
 *  This file contains the implementation and
 *  the source code of the functions used to
 *  interface the I/O Expanders
 */

#include <i2c_comm.h>


#define EXP_OUT_ADDR (0x21)<<1
#define EXP_IN_ADDR	 (0x20)<<1

/*--------------------------- Function Declarations ----------------------------*/
uint16_t receive_data_from_pointer(I2C_HandleTypeDef hi2c, uint8_t DevAddr, uint8_t pointer_register);
uint16_t aligned_data(uint16_t data);

/*--------------------------- Interface Functions-----------------------------*/
/*
 * @brief Initialize I/O expanders by configuring
 * 		  the direction of the I/O's and setting
 * 		  the status of all to LOW
 *
 * @param  I2C Handle
 * @retval 0 on success
 */
int initialize_expanders(I2C_HandleTypeDef hi2c)
{
	uint8_t tmp_buf[3];

	if(HAL_I2C_IsDeviceReady(&hi2c,EXP_OUT_ADDR,10,1000) == HAL_OK)
	{
		/*------------------- Outputs Expander ----------------------*/

		//Set logic level of the two output ports to LOW
		tmp_buf[0] = OUTPUT_PORT_0;
		tmp_buf[1] = 0x00;
		tmp_buf[2] = 0x00;
		HAL_I2C_Master_Transmit(&hi2c, EXP_OUT_ADDR, tmp_buf, 3, 1000);

		HAL_Delay(10);

		//Configure the direction of the two ports as outputs
		tmp_buf[0] = CONFIGURATION_PORT_0;
		tmp_buf[1] = 0x00;
		tmp_buf[2] = 0x00;
		HAL_I2C_Master_Transmit(&hi2c, EXP_OUT_ADDR, tmp_buf, 3, 1000);

		HAL_Delay(10);
	}

		//TODO Push pull or open drain configuration on output register

	if(HAL_I2C_IsDeviceReady(&hi2c,EXP_IN_ADDR,10,1000) == HAL_OK)
	{
		/*------------------- Inputs Expander ----------------------*/

		//Configure the direction of the 2 ports as inputs
		tmp_buf[0] = CONFIGURATION_PORT_1;
		tmp_buf[1] = 0xff;
		tmp_buf[2] = 0xff;
		HAL_I2C_Master_Transmit(&hi2c, EXP_IN_ADDR, tmp_buf, 3, 1000);

		HAL_Delay(10);

		//Enable the interrupts
		tmp_buf[0] = INTERRUPT_MASK_REG_1;
		tmp_buf[1] = 0x00;
		tmp_buf[2] = 0x00;
		HAL_I2C_Master_Transmit(&hi2c, EXP_IN_ADDR, tmp_buf, 3, 1000);

		HAL_Delay(10);

		//TODO ISR for the inputs
	}

	return 0;
}

/*
 * @brief Configure the outputs of the OUTPUTS EXPANDER
 * 		  (set HIGH or LOW)
 * @param  I2C Handle
 * @param  Desired configuration for outputs
 * @retval 0 on success
 */
int configure_outputs(I2C_HandleTypeDef hi2c, uint16_t outputs)
{
	uint8_t tmp_buf[3];

	uint16_t tmp_outputs = aligned_data(outputs);

	tmp_buf[0] = OUTPUT_PORT_1;
	tmp_buf[1] = (uint8_t)(tmp_outputs>>8);
	tmp_buf[2] = (uint8_t) tmp_outputs;
	HAL_I2C_Master_Transmit(&hi2c, EXP_OUT_ADDR, tmp_buf, 3, 1000);
	return 0;
}

/*
 * @brief Enable the ability of the I/O pins to have pull-up/down
 * 		  resistors.
 * 		  Setting '1' connects the PUPD resistors
 * 		  Setting '0' disconnects the PUPD resistors
 * @param  I2C Handle
 * @param  Address of the expander to be configured
 * @param  Desired configuration for the registers
 * @retval 0 on success
 */
int configure_pupd_enable_registers(I2C_HandleTypeDef hi2c, uint8_t DevAddr ,uint16_t pupd_pins)
{
	uint8_t tmp_buf[3];

	tmp_buf[0] = PUPD_ENABLE_REG_1;
	tmp_buf[1] = (uint8_t)(pupd_pins>>8);
	tmp_buf[2] = (uint8_t) pupd_pins;
	HAL_I2C_Master_Transmit(&hi2c, DevAddr, tmp_buf, 3, 1000);
	return 0;
}

/*
 * @brief Configure the pull-up/down resistors for any EXPANDER by writing
 * 		  to the PUPD selection register. Writing "1" means setting a pull-up
 * 		  resistor for the I/O pin, writing "0" means setting a pull-down for
 * 		  the I/O pin.
 * @param  I2C Handle
 * @param  Address of the expander to be configured
 * @param  Desired configuration for the I/O pins
 * @retval 0 on success
 */
int configure_pupd_selection_registers(I2C_HandleTypeDef hi2c, uint8_t DevAddr ,uint16_t pupd_pins)
{
	uint8_t tmp_buf[3];

	tmp_buf[0] = PUPD_SELECTION_REG_1;
	tmp_buf[1] = (uint8_t)(pupd_pins>>8);
	tmp_buf[2] = (uint8_t) pupd_pins;
	HAL_I2C_Master_Transmit(&hi2c, DevAddr, tmp_buf, 3, 1000);
	return 0;
}

/*
 * @brief Set all outputs of the OUTPUTS EXPANDER to LOW
 * @param  I2C Handle
 * @retval 0 on success
 */
int clear_all_outputs(I2C_HandleTypeDef hi2c)
{
	uint8_t tmp_buf[3];

	tmp_buf[0] = OUTPUT_PORT_1;
	tmp_buf[1] = 0x00;
	tmp_buf[2] = 0x00;
	HAL_I2C_Master_Transmit(&hi2c, EXP_OUT_ADDR, tmp_buf, 3, 1000);
	return 0;
}


/*
 * @brief Read all the inputs of the INPUTS EXPANDER
 * @param  I2C Handle
 * @retval Inputs in 16-bit value
 */
uint16_t read_all_inputs(I2C_HandleTypeDef hi2c)
{
	return receive_data_from_pointer(hi2c, EXP_IN_ADDR, INPUT_PORT_1);
}

/*
 * @brief Get the current state of the outputs
 * @param I2C Handle
 * @retval Outputs in 16-bit value
 */
uint16_t read_all_outputs(I2C_HandleTypeDef hi2c)
{
	return aligned_data(receive_data_from_pointer(hi2c, EXP_OUT_ADDR, OUTPUT_PORT_1));
}

/*
 * @brief Get the interrupt mask register for the INPUTS EXPANDER
 * @param I2C Handle
 * @retval Interrupt Mask register in 16-bit value
 */
uint16_t read_interrupt_mask_register(I2C_HandleTypeDef hi2c)
{
	return receive_data_from_pointer(hi2c, EXP_IN_ADDR, INTERRUPT_MASK_REG_1);
}

/*
 * @brief Get the interrupt status for the INPUTS EXPANDER
 * @param I2C Handle
 * @retval Interrupt Status register in 16-bit value
 */
uint16_t read_interrupt_status_register(I2C_HandleTypeDef hi2c)
{
	return receive_data_from_pointer(hi2c, EXP_IN_ADDR, INTERRUPT_STATUS_REG_1);
}


/*--------------------- Communication Handling Functions --------------------*/

/*
 * In order to read data from the slave, we must first send a write command to the
 * address of the slave (R/W bit set to "0") followed by the command byte (pointer
 * register bits) we want to access and then re-establish communication, this time
 * with the R/W bit set to "1" in order to start reception data from the register.
 *
 */
uint16_t receive_data_from_pointer(I2C_HandleTypeDef hi2c, uint8_t DevAddr, uint8_t pointer_register)
{
	uint8_t tmp_buf[2];

	tmp_buf[0] = pointer_register;

	__disable_irq();

	HAL_I2C_Master_Transmit(&hi2c, DevAddr, tmp_buf, 1, 1000);

	//HAL_Delay(50);

	HAL_I2C_Master_Receive(&hi2c, DevAddr, tmp_buf, 2, 1000);

	__enable_irq();

	uint16_t data = (tmp_buf[0] << 8) | tmp_buf[1];

	return data;
}


/*
 * In order for the controls in the computer application to be aligned with the
 * correct header pins, the data need to be handled. This is because the header
 * is connected differently to each expander.
 */
uint16_t aligned_data(uint16_t data)
{
	uint16_t rev;
	uint16_t temp_data;

	temp_data = data;

	for(int i=0;i<16;i++)
	{
		rev <<= 1;

		if((temp_data & 1)==1)
		{
			rev ^= 1;
		}

		temp_data >>= 1;
	}

	return rev;
}
