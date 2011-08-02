

// from Sect 24 of the PIC32 ref manual:
// 1. Turn on the module by setting ON bit (I2CxCON<15>) to ‘1’.
// 1. Assert a Start condition on SDAx and SCLx.
// 2. Send the I2C device address byte to the slave with a write indication.
// 3. Wait for and verify an Acknowledge from the slave.
// 4. Send the serial memory address high byte to the slave.
// 5. Wait for and verify an Acknowledge from the slave.
// 6. Send the serial memory address low byte to the slave.
// 7. Wait for and verify an Acknowledge from the slave.
// 8. Assert a Repeated Start condition on SDAx and SCLx.
// 9. Send the device address byte to the slave with a read indication.
// 10. Wait for and verify an Acknowledge from the slave.
// 11. Enable master reception to receive serial memory data.
// 12. Generate an ACK or NACK condition at the end of a received byte of data.
// 13. Generate a Stop condition on SDAx and SCLx.

typedef enum {
  I2C1_BUS_START,
  I2C1_BUS_WR_DATA,
  I2C1_BUS_WR_COMPLETE,
  I2C1_BUS_REPEAT_START,
  I2C1_BUS_RD_ADDR,
  I2C1_BUS_RD_DATA,
  I2C1_BUS_RD_ACK,
  I2C1_BUS_RD_STOP,
  I2C1_BUS_RD_COMPLETE,
  I2C1_BUS_ERROR
} I2C1_BUS_STATE_m;

// Interrupts from I2C controller
// 1. Start Condition
// - 1 BRG (Baud Rate Generator) time after falling edge of SDA
// 2. Repeated Start Sequence
// - 1 BRG time after falling edge of SDA
// 3. Stop Condition
// - 1 BRG time after the rising edge of SDA
// 4. Data transfer byte received
// - 8th falling edge of SCL
// (After receiving eight bits of data from slave)
// 5. During SEND ACK sequence
// - 9th falling edge of SCL
// (After sending ACK or NACK to slave)
// 6. Data transfer byte transmitted
// - 9th falling edge of SCL
// (Regardless of receiving ACK from slave)
// 7. During a slave-detected Stop
// - When slave sets P bit

typedef enum {
  I2C1_MODE_IDLE,
  I2C1_MODE_WRITE,
  I2C1_MODE_WRITE_COMPLETE,
  I2C1_MODE_READ_COMPLETE,
  I2C1_MODE_ERROR,
  I2C1_MODE_ERR_START,
  I2C1_MODE_ERR_STOP
} I2C1_MODE_m;

// static data storage for I2C messages
#define I2C1_QUEUE_SIZE  16
typedef struct {
  byte src;     // type of transfer
  byte wn;      // number of data bytes to write
  byte wi;      // number of data bytes written
  byte wd[16];  // data to write
  byte ra;      // I2C address (LSB = R/Wn)
  byte rn;      // number of data bytes to read
  byte ri;      // number of data bytes read
  byte rd[16];  // response data
  byte postDelay; // msec to delay after operation completes
} I2C1_QUEUE_t;


typedef enum {
  I2C1_NONE,
  I2C1_RTC_READ,
  I2C1_RTC_WRITE,
  I2C1_EEPROM_READ,
  I2C1_EEPROM_WRITE,
  I2C1_TC74_READ,
  I2C1_TC74_WRITE
} mI2C1_SOURCE;


EXTERN byte i2c1QueueRd;
EXTERN byte i2c1QueueWr;
EXTERN I2C1_QUEUE_t i2c1Queue[I2C1_QUEUE_SIZE];
EXTERN I2C1_MODE_m i2c1Mode;
EXTERN I2C1_BUS_STATE_m i2c1BusState;
EXTERN byte i2c1Delay;


// I2C1.c prototypes
void I2C1init(void);
void I2C1update(void);
void I2C1process(void);
void I2C1wrQueue(void);
