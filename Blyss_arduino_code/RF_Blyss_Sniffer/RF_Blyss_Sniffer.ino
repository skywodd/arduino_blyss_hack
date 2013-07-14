/**
 * Blyss protocol sniffer
 */

/* Receiver pinmap */
const byte RF_RX_VCC = 11;
const byte RF_RX_SIG = 12; // don't forget the wire between this pin and D2 !
const byte RF_RX_GND = 10;

/** Frame buffer with double-buffering */
byte RF_BUFFER[7], RF_OLD[7] = {
  0};

/* -------------------------------------------------------- */
/* ----                Blyss Sniffer API               ---- */
/* -------------------------------------------------------- */

/* Time constants */
const unsigned long H_TIME_H = 2800; // Max duration of header (2600 = margin +200)
const unsigned long H_TIME_L = 2200; // Min duration of header (2600 = margin -400)
const unsigned long T_TIME_H = 1200; // Max duration of a 1/3 level (1000 = margin +200)
const unsigned long T_TIME_L = 100;  // Min duration of a 1/3 level (250 = margin -150)
const unsigned long T_TIME_M = 600;  // Average duration of a 1/3 level

/** End of decoding flag */
volatile byte data_decoded = false;

/** Low level registers and bitmask */
volatile uint8_t bitmask;
volatile uint8_t *port;

/** Transmission channels and status enumeration */
enum {
  CH_1 = 8, CH_2 = 4, CH_3 = 2, CH_4 = 1, CH_5 = 3, CH_ALL = 0,
  CH_A = 0, CH_B = 1, CH_C = 2, CH_D = 3
};

/** 
 * Check received rolling code
 *
 * @param code Received rolling code
 * @return True if the rolling code is valid, false otherwise
 */
byte valid_rolling_code(byte code) {
  return !(code != 0x98 && code !=  0xDA && code != 0x1E && code != 0xE6 && code != 0x67);
}

/* ISR routines variables */
unsigned long current_time, relative_time, last_time = 0;
byte bits_counter, bit_complet, last_bit, current_bit;
byte trame_triggered = false;
volatile byte rf_data[7];

/** 
 * Get received frame data if available
 *
 * @param[out] data Pointer to a RF frame-data buffer used to store received frame
 * @return True if a RF frame have been completly received, false otherwise
 */
byte get_data(byte *data) {
  if(data_decoded) {
    for(byte i = 0; i < 7; ++i) data[i] = rf_data[i];
    data_decoded = false;
    return true; 
  }
  return false;
}

/** 
 * ISR RF frame decoding routine
 */
void isr_decoding_routine(void) {

  current_time = micros();
  relative_time = current_time - last_time;
  last_time = current_time;

  if(data_decoded){ 
    return;  
  }

  if(!trame_triggered) {
    if(!(*port & bitmask) && (relative_time > H_TIME_L) && (relative_time < H_TIME_H)) {
      trame_triggered = true;
      bit_complet = false;
      bits_counter = 0;
      return;
    }
  } 
  else {
    if((relative_time < T_TIME_L) || (relative_time > T_TIME_H)) {
      trame_triggered = false;
      return;
    }

    if(relative_time > T_TIME_M)
      current_bit = 'T';
    else
      current_bit = 't';

    if(!bit_complet) {
      last_bit = current_bit;
      bit_complet = true;
    } 
    else {
      bit_complet = false;

      if(last_bit == 't' && current_bit == 'T') {
        bitWrite(rf_data[bits_counter / 8], (7 - bits_counter % 8), 1);
      } 
      else if(last_bit == 'T' && current_bit == 't')  {
        bitWrite(rf_data[bits_counter / 8], (7 - bits_counter % 8), 0);
      } 
      else {
        trame_triggered = false;
        return;
      }

      if(++bits_counter == 52) {
        data_decoded = true;
        trame_triggered = false;
        return;
      }
    }
  }
}

/** 
 * Check if two frame-data buffer are equal
 *
 * @param[in] data_1 Pointer to a RF frame-data buffer
 * @param[in] data_2 Pointer to a RF frame-data buffer
 * @return True if the two buffer are equal, false otherwise
 */
byte data_match(byte *data_1, byte *data_2) {
  for(byte i = 0; i < 7; ++i)
    if(data_1[i] != data_2[i]) return false;
  return true;
}

/** 
 * Decode raw binary data to human readable text outputed on the serial port
 *
 * @param data Pointer to the RF frame-data buffer to decode
 */
void data_analyse(byte *data) {
  Serial.println();
  Serial.print("RF frame : ");
  for(byte i = 0; i < 7; ++i) {
    if(data[i] < 16)
      Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  Serial.print("RF footprint : ");
  Serial.print(data[0], HEX);
  if(data[0] == 0xFE)
    Serial.println(" - OK");
  else
    Serial.println(" - ERROR");

  Serial.print("RF global channel : ");
  byte global_channel = (data[1] & 0xF0) >> 4;
  switch(global_channel) {
  case CH_A:
    Serial.println("CH_A (0)");
    break;
  case CH_B:
    Serial.println("CH_B (1)");
    break;
  case CH_C:
    Serial.println("CH_C (2)");
    break;
  case CH_D:
    Serial.println("CH_D (3)");
    break;
  default :
    Serial.println(global_channel, HEX);
    break;
  }

  Serial.print("RF adress : ");
  byte address = ((data[1] & 0x0F) << 4) | ((data[2] & 0xF0) >> 4);
  if(address < 16)
    Serial.print('0');
  Serial.print(address, HEX);
  Serial.print(' ');
  address = ((data[2] & 0x0F) << 4) | ((data[3] & 0xF0) >> 4);
  if(address < 16)
    Serial.print('0');
  Serial.print(address, HEX);
  Serial.println();

  Serial.print("RF channel : ");
  switch(data[3] & 0x0F) {
  case CH_1:
    Serial.println("CH_1 (8)");
    break;
  case CH_2:
    Serial.println("CH_2 (4)");
    break;
  case CH_3:
    Serial.println("CH_3 (2)");
    break;
  case CH_4:
    Serial.println("CH_4 (1)");
    break;
  case CH_5:
    Serial.println("CH_5 (3)");
    break;
  case CH_ALL:
    Serial.println("CH_ALL (0)");
    break;
  default :
    Serial.print("ERROR (");
    Serial.print(data[3] & 0x0F, HEX);
    Serial.println(")");
    break;
  }

  Serial.print("Light status : ");
  byte light_status = (data[4] & 0xF0) >> 4;
  if(light_status == 1)
    Serial.println("OFF");
  else {
    if(light_status == 0)
      Serial.println("ON");
    else 
      Serial.println(light_status, HEX);
  }

  Serial.print("Rolling code : ");
  byte rolling_code = ((data[4] & 0x0F) << 4) | ((data[5] & 0xF0) >> 4);
  if(rolling_code < 16)
    Serial.print('0');
  Serial.print(rolling_code, HEX);
  if(valid_rolling_code(rolling_code))
    Serial.println(" - OK");
  else
    Serial.println(" - ERROR");

  Serial.print("Timestamp : ");
  byte timestamp = ((data[5] & 0x0F) << 4) | ((data[6] & 0xF0) >> 4);
  if(timestamp < 16)
    Serial.print('0');
  Serial.println(timestamp, HEX);

  Serial.println();
}

/**
 * Copy one frame-data buffer to another one
 *
 * @param[in] src Pointer to the source frame-data buffer
 * @param[out] dst Pointer to the destionation frame-data buffer
 */
inline void data_copy(byte *src, byte *dst) {
  for(byte i = 0; i < 7; ++i) dst[i] = src[i];
}


/* -------------------------------------------------------- */
/* ----            Sniffing example program            ---- */
/* -------------------------------------------------------- */

/** setup() */
void setup() {

  /* Receiver pins as input, power pins as output */
  pinMode(RF_RX_VCC, OUTPUT);
  pinMode(RF_RX_SIG, INPUT);
  pinMode(RF_RX_GND, OUTPUT);

  /* low-level ISR decoding routine setup */
  attachInterrupt(0, isr_decoding_routine, CHANGE);
  
  /* Bitmask and register of RX pin setup */
  bitmask = digitalPinToBitMask(RF_RX_SIG);
  port = portInputRegister(digitalPinToPort(RF_RX_SIG));

  /* Fast powerring tips */
  digitalWrite(RF_RX_VCC, HIGH);
  digitalWrite(RF_RX_GND, LOW);

  /* Activity led as output and low */
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  /* Serial port initialization (for output) */
  Serial.begin(115200);
  Serial.println("Blyss sniffer");
}

/** loop() */
void loop() {
  
  /* Frame echo counter */
  static byte matching_frame = 1;
  
  /* Get RF frame data if available */
  if(get_data(RF_BUFFER)) {
    
    /* Check if the received frame is the same as the old one */
    if(!data_match(RF_BUFFER, RF_OLD)) {
      
      /* If not, decode the new frame buffer */
      data_analyse(RF_BUFFER);
      
      /* Store the new frame buffer (for echo checking) */
      data_copy(RF_BUFFER, RF_OLD);
      matching_frame = 1;
      
    } else {
	
      /* If yes, just tell user for a frame echo */
      Serial.print("Frame echo ");
      Serial.println(++matching_frame, DEC);
    }
  }
}
