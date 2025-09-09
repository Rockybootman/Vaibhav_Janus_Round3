char nmeaBuffer[128];  // buffer to store one NMEA sentence
int bufferIndex = 0;
int prev_alt=0;
char* state="";
int peak_alt=0;
void sendCommand(const char *cmd) {
  Serial1.print(cmd);   // send to GNSS
  Serial.print(">> ");  // echo command to Serial Monitor
  Serial.println(cmd);
}

void setup() {
  Serial.begin(115200);   // Serial Monitor
  Serial1.begin(9600);    // L89HA UART

  delay(2000); // wait for GNSS to power up

  Serial.println("Configuring GNSS to output only $GNRMC and $GNGGA...");

  // Disable all NMEA
  sendCommand("$PSTMNMEACFG,0,0*1C\r\n");
  delay(200);

  // Enable only RMC
  sendCommand("$PSTMNMEACFG,RMC,1*29\r\n");
  delay(200);

  // Enable only GGA
  sendCommand("$PSTMNMEACFG,GGA,1*2D\r\n");
  delay(200);

  Serial.println("GNSS configured. Receiving data...");
}

void loop() {
  while (Serial1.available()) {
    char c = Serial1.read();

    if (c == '\n') { // end of NMEA sentence
      nmeaBuffer[bufferIndex] = '\0'; // null terminate
      parseNMEA(nmeaBuffer);
      bufferIndex = 0; // reset buffer
    } else if (c != '\r') { // ignore carriage return
      nmeaBuffer[bufferIndex++] = c;
      if (bufferIndex >= 127) bufferIndex = 127; // prevent overflow
    }
  }
}

// Simple parser for $GNRMC and $GNGGA
void parseNMEA(char *sentence) {
  if (strncmp(sentence, "$GNRMC", 6) == 0) {
    // Format: $GNRMC,time,status,lat,N/S,lon,E/W,speed,course,date,...
    char *token = strtok(sentence, ",");
    int field = 0;
    char time[16] = "", lat[16] = "", lon[16] = "";

    while (token != NULL) {
      field++;
      if (field == 2) strcpy(time, token);
      if (field == 4) strcpy(lat, token);
      if (field == 5) strcat(lat, token);  // N/S
      if (field == 6) strcpy(lon, token);
      if (field == 7) strcat(lon, token);  // E/W
      token = strtok(NULL, ",");
    }

    Serial.print("Time: "); Serial.print(time);
    Serial.print(" | Lat: "); Serial.print(lat);
    Serial.print(" | Lon: "); Serial.println(lon);
  }

  if (strncmp(sentence, "$GNGGA", 6) == 0) {
    // Format: $GNGGA,time,lat,N/S,lon,E/W,fix,satellites,HDOP,altitude,M,...
    char *token = strtok(sentence, ",");
    int field = 0;
    char altitude[16] = "";

    while (token != NULL) {
      field++;
      if (field == 10) strcpy(altitude, token); // altitude above MSL
      token = strtok(NULL, ",");
    }

    Serial.print(" | Altitude (MSL): "); Serial.println(altitude);
    float alt = atof(altitude);
    if (peak_alt == 0) {
    state = "IDLE";
    } else if (alt > prev_alt + 0.5) {
    state = "ASCEND";
    } else if (alt < prev_alt - 0.5) {
    state = "DESCEND";
    } else {
    state = "APOGEE";
    }
    if (strcmp(state, "DESCEND") == 0 && alt <= 0.75 * peak_alt) {
    state = "PAYLOAD DEPLOYED";
    }
    if (alt <= 1.0 && peak_alt != 0) {
    state = "LANDED";
    }
    if (alt > peak_alt) peak_alt = alt;
    
    prev_alt = alt;
    Serial.print("Current state: ");Serial.println(state);


  }
}
