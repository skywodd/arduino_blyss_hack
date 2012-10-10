/* -------------------------------------------------------- */
/* ----             Blyss gateway program              ---- */
/* -------------------------------------------------------- */

/* Frame format (NEMA typed) */
/* Rx: $global_channel;key_MSB.key_LSB;channel;status;rolling_code;timestamp\n */
/* Tx: $global_channel;key_MSB.key_LSB;channel;status\n */

/* Transmitter pinmap */
const byte RF_TX_VCC = 5;
const byte RF_TX_SIG = 4;
const byte RF_TX_GND = 3;

/* Receiver pinmap */
const byte RF_RX_VCC = 11;
const byte RF_RX_SIG = 12; // don't forget the wire between this pin and D2 !
const byte RF_RX_GND = 10;

/** Transmission channels and status enumeration */
enum {
  OFF, ON, 
  CH_1 = 8, CH_2 = 4, CH_3 = 2, CH_4 = 1, CH_5 = 3, CH_ALL = 0,
  CH_A = 0, CH_B = 1, CH_C = 2, CH_D = 3
};

/* RF signal usage macro */
#define SIG_HIGH() digitalWrite(RF_TX_SIG, HIGH)
#define SIG_LOW() digitalWrite(RF_TX_SIG, LOW)

/** End of decoding flag */
volatile byte data_decoded = false;

/** Low level registers and bitmask */
volatile uint8_t bitmask;
volatile uint8_t *port;

/** Frame buffer with single-buffering */
byte RF_BUFFER_RX[7],  RF_BUFFER_TX[7];

/** Serial buffer (for incoming commands) */
char SERIAL_BUFFER[12];

/** setup() */
void setup() {

  /* Receiver pins as input, power pins as output */
  pinMode(RF_RX_VCC, OUTPUT);
  pinMode(RF_RX_SIG, INPUT);
  pinMode(RF_RX_GND, OUTPUT);

  /* Transmitter pins as output */
  pinMode(RF_TX_VCC, OUTPUT);
  pinMode(RF_TX_SIG, OUTPUT);
  pinMode(RF_TX_GND, OUTPUT);

  /* Fast powerring tips */
  digitalWrite(RF_TX_VCC, HIGH);
  digitalWrite(RF_TX_GND, LOW);
  digitalWrite(RF_RX_VCC, HIGH);
  digitalWrite(RF_RX_GND, LOW);

  /* low-level ISR decoding routine setup */
  attachInterrupt(0, isr_decoding_routine, CHANGE);
  
  /* Bitmask and register of RX pin setup */
  bitmask = digitalPinToBitMask(RF_RX_SIG);
  port = portInputRegister(digitalPinToPort(RF_RX_SIG));

  /* Serial port initialization */
  Serial.begin(115200);
  Serial.println("BOOT");

  /* Kill Tx RF signal for now */
  SIG_LOW();
}

/** loop() */
void loop() {

  /* Get RF frame data if available */
  if(get_data(RF_BUFFER_RX)) {

    /* Decode and output the frame data */
    data_analyse(RF_BUFFER_RX);
  }

  /* Get serial command if available */
  if(Serial.available() >= 11 + 2) {
    
    /* Check if command start with $ header */
    if(Serial.read() == '$') {
      char global_channel, channel, status;
      byte address[2];

      /* Get whole command text */
      for(byte i = 0; i < 11; ++i)
        SERIAL_BUFFER[i] = Serial.read();
      SERIAL_BUFFER[11] = '\0'; // Finalize ASCII string
      
      /* Check for newline (end of command) */
      if(Serial.read() != '\n') return;
      
      /* Extract command fields */
      if(sscanf(SERIAL_BUFFER, "%c;%x.%x;%c;%c", &global_channel, address, address + 1, &channel, &status) != 5) return;
      
      /* Compute RF key */
      byte RF_KEY[3];
      RF_KEY[0] = address[0] >> 4;
      RF_KEY[1] = ((address[0] & 0x0F) << 4) | address[1] >> 4;
      RF_KEY[2] = address[1] << 4;
      set_key(RF_BUFFER_TX, RF_KEY, false);

      /* Compute RF channel */
      switch(channel) {
      case '1':
        set_channel(RF_BUFFER_TX, CH_1);
        break;
      case '2':
        set_channel(RF_BUFFER_TX, CH_2);
        break;
      case '3':
        set_channel(RF_BUFFER_TX, CH_3);
        break;
      case '4':
        set_channel(RF_BUFFER_TX, CH_4);
        break;
      case '5':
        set_channel(RF_BUFFER_TX, CH_5);
        break;
      case 'A':
        set_channel(RF_BUFFER_TX, CH_ALL);
        break;
      default:
        return;
        break;
      }

      /* Compute global RF channel */
      switch(global_channel) {
      case 'a':
        set_global_channel(RF_BUFFER_TX, CH_A);
        break;
      case 'b':
        set_global_channel(RF_BUFFER_TX, CH_B);
        break;
      case 'c':
        set_global_channel(RF_BUFFER_TX, CH_C);
        break;
      case 'd':
        set_global_channel(RF_BUFFER_TX, CH_D);
        break;
      default:
        set_global_channel(RF_BUFFER_TX, global_channel);
        break;
      }

      /* Compute virtual switch state */
      set_status(RF_BUFFER_TX, (status == 'P') ? ON : (status == 'p') ? OFF : status);

      /* Insert rolling code and token into frame-data buffer */
      generate_rolling_code(RF_BUFFER_TX);
      generate_token(RF_BUFFER_TX);

      /* Send RF frame (disable receiver ISR routine if necessary) */
      if(data_decoded) {
        send_command(RF_BUFFER_TX);
      } else {
        data_decoded = true; // Disable receiver ISR routine (avoid corrupted frame)
        send_command(RF_BUFFER_TX);
        data_decoded = false;
      }
    }
  }
}

