This program implement all API required to send and received Blyss-like RF frame.

The included demo is a "gateway" program.
It take serial command in NEMA format and send it as Blyss frame.
It also check for incoming frames and output them in the same format.

---

Command format (NEMA typed) :
- Rx : $global_channel;key_MSB.key_LSB;channel;status;rolling_code;timestamp\n
- Tx: $global_channel;key_MSB.key_LSB;channel;status\n 