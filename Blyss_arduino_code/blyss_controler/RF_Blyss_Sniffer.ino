/* -------------------------------------------------------- */
/* ----                Blyss Sniffer API               ---- */
/* -------------------------------------------------------- */

/* Time constants */
const unsigned long H_TIME_H = 2800; // Max duration of header (2600 = margin +200)
const unsigned long H_TIME_L = 2200; // Min duration of header (2600 = margin -400)
const unsigned long T_TIME_H = 1200; // Max duration of a 1/3 level (1000 = margin +200)
const unsigned long T_TIME_L = 100;  // Min duration of a 1/3 level (250 = margin -150)
const unsigned long T_TIME_M = 600;  // Average duration of a 1/3 level

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

  if(data[0] != 0xFE) return;

  Serial.write('$');
  byte global_channel = (data[1] & 0xF0) >> 4;
  switch(global_channel) {
  case CH_A:
    Serial.write('a');
    break;
  case CH_B:
    Serial.write('b');
    break;
  case CH_C:
    Serial.write('c');
    break;
  case CH_D:
    Serial.write('d');
    break;
  default :
    Serial.print(global_channel, HEX);
    break;
  }
  Serial.write(';');

  byte address = ((data[1] & 0x0F) << 4) | ((data[2] & 0xF0) >> 4);
  if(address < 16)
    Serial.write('0');
  Serial.print(address, HEX);
  Serial.write('.');
  address = ((data[2] & 0x0F) << 4) | ((data[3] & 0xF0) >> 4);
  if(address < 16)
    Serial.write('0');
  Serial.print(address, HEX);
  Serial.write(';');

  switch(data[3] & 0x0F) {
  case CH_1:
    Serial.write('1');
    break;
  case CH_2:
    Serial.write('2');
    break;
  case CH_3:
    Serial.write('3');
    break;
  case CH_4:
    Serial.write('4');
    break;
  case CH_5:
    Serial.write('5');
    break;
  case CH_ALL:
    Serial.write('A');
    break;
  default :
    Serial.write('E');
    break;
  }
  Serial.write(';');

  byte light_status = (data[4] & 0xF0) >> 4;
  if(light_status == 1)
    Serial.write('p');
  else {
    if(light_status == 0)
      Serial.write('P');
    else 
      Serial.print(light_status, HEX);
  }
  Serial.write(';');

  byte rolling_code = ((data[4] & 0x0F) << 4) | ((data[5] & 0xF0) >> 4);
  if(valid_rolling_code(rolling_code))
    Serial.write('V');
  else
    Serial.write('E');
  Serial.write(';');

  byte timestamp = ((data[5] & 0x0F) << 4) | ((data[6] & 0xF0) >> 4);
  if(timestamp < 16)
    Serial.write('0');
  Serial.print(timestamp, HEX);
  Serial.write('\n');
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
