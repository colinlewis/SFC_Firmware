

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
  I2C_BUS_START,
  I2C_BUS_WR_DATA,
  I2C_BUS_WR_COMPLETE,
  I2C_BUS_REPEAT_START,
  I2C_BUS_RD_ADDR,
  I2C_BUS_RD_DATA,
  I2C_BUS_RD_ACK,
  I2C_BUS_RD_STOP,
  I2C_BUS_RD_COMPLETE,
} I2C_BUS_STATE_m;

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

#define I2C2_ADDR1 0x90
#define I2C2_ADDR2 0x92

typedef enum {
  I2C2_MODE_IDLE,
  I2C2_MODE_WRITE,
  I2C2_MODE_WRITE_COMPLETE,
  I2C2_MODE_READ_COMPLETE
} I2C2_MODE_m;

// static data storage for I2C messages
#define I2C2_QUEUE_SIZE  16
typedef struct {
  byte chan;  // ADS1115_chan
  byte wn;    // number of data bytes to write
  byte wi;    // number of data bytes written
  byte wd[8]; // data to write
  byte ra;    // I2C address (LSB = R/Wn)
  byte rn;    // number of data bytes to read
  byte ri;    // number of data bytes read
  byte rd[7]; // response data
} I2C_QUEUE_t;


// ADS1115 channels
typedef enum {
  ADS1115_IN1,
  ADS1115_IN2,
  ADS1115_IN3,
  ADS1115_IN4,
  ADS1115_IN5,
  ADS1115_RTD,
  ADS1115_FLOW_SW,
  ADS1115_MAX_CHAN
} ADS1115_CHAN_m;

EXTERN ADS1115_CHAN_m ads1115_chan;
EXTERN ushort ads1115[ADS1115_MAX_CHAN];

EXTERN byte i2c2QueueRd;
EXTERN byte i2c2QueueWr;
EXTERN I2C_QUEUE_t i2c2Queue[I2C2_QUEUE_SIZE];
EXTERN I2C2_MODE_m i2c2Mode;
EXTERN I2C_BUS_STATE_m i2c2BusState;

// I2C.c prototypes
void I2C2init(void);
void I2C2update(void);
void I2C2process(void);
void I2C2wrQueue(void);

// RTD.c prototypes
short RTDconvert(ushort ad);
void RTDcal(ushort adc100, ushort adc180);
