#include "ArduCAM.h"

#include "util_gpio.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(arducam_spi, CONFIG_ARDUCAM_MODULE_LOG_LEVEL);

uint32_t pinCsn;

bool arducam_spiInit(const struct arducam_conf *config)
{
  LOG_DBG("arducam_spiInit");  
  pinCsn = config->pinCsn;
  init_gpio();
  return arducam_spiInit_pd(config);
}

void arducam_spiStop(const struct arducam_conf *config)
{
  arducam_spiStop_pd(config);
}


//Assert CS signal
void arducam_CS_LOW(void)
{
  LOG_DBG("arducam_CS_LOW");
  gpio_util_pin_low(pinCsn);
}

//Disable CS signal
void arducam_CS_HIGH(void)
{
  LOG_DBG("arducam_CS_HIGH");
  gpio_util_pin_high(pinCsn);
}

//Write ArduChip internal registers
void arducam_write_reg(uint8_t addr, uint8_t data)
{
  arducam_bus_write(addr | 0x80, data);
}

//Read ArduChip internal registers
uint8_t arducam_read_reg(uint8_t addr)
{
  uint8_t data;
  data = arducam_bus_read(addr & 0x7F);
  return data;
}

//Set corresponding bit
void arducam_set_bit(uint8_t addr, uint8_t bit)
{
  uint8_t temp;
  temp = arducam_read_reg(addr);
  arducam_write_reg(addr, temp | bit);
}

//Clear corresponding bit
void arducam_clear_bit(uint8_t addr, uint8_t bit)
{
  uint8_t temp;
  temp = arducam_read_reg(addr);
  arducam_write_reg(addr, temp & (~bit));
}

//Get corresponding bit status
uint8_t arducam_get_bit(uint8_t addr, uint8_t bit)
{
  uint8_t temp;
  temp = arducam_read_reg(addr);
  temp = temp & bit;
  return temp;
}

//Reset the FIFO pointer to ZERO
void arducam_flush_fifo(void)
{
  arducam_write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

//Send capture command
void arducam_start_capture(void)
{
  arducam_write_reg(ARDUCHIP_FIFO, FIFO_START_MASK);
}

//Clear FIFO Complete flag
void arducam_clear_fifo_flag(void)
{
  arducam_write_reg(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);
}

//Read FIFO single
uint8_t arducam_read_fifo(void)
{
  uint8_t data;
  data = arducam_bus_read(SINGLE_FIFO_READ);
  return data;
}

//Read Write FIFO length
uint32_t arducam_read_fifo_length(void)
{
  uint32_t len1, len2, len3, length = 0;
  len1 = arducam_read_reg(FIFO_SIZE1);
  len2 = arducam_read_reg(FIFO_SIZE2);
  len3 = arducam_read_reg(FIFO_SIZE3) & 0x7f;
  length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
  return length;
}

//Send read fifo burst command
//Support ArduCAM Mini only
void arducam_set_fifo_burst()
{
    uint8_t cmd = BURST_FIFO_READ;
    arducam_spiWrite(cmd);
}
