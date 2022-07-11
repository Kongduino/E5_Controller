struct menuButton {
  void (*ptr)(); // Function pointer
  int16_t tx, ty, w;
  char *name;
};

uint8_t mySF = 12, myTx = 22;
uint16_t myBW = 125;
uint32_t myFreq = 868000000, pingCount = 0;
void drawMyButton(menuButton);

void drawMyButton(menuButton bn) {
  lcd.fillRoundRect(bn.tx + 1, bn.ty + 1, bn.w, 40, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx + 2, bn.ty + 2, bn.w, 40, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx + 3, bn.ty + 3, bn.w, 40, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx, bn.ty, bn.w, 40, 4, TFT_RED);
  lcd.setTextColor(TFT_WHITE);
  lcd.setCursor(bn.tx + 9, bn.ty + 12);
  lcd.print(bn.name);
  lcd.setCursor(bn.tx + 10, bn.ty + 13);
  lcd.print(bn.name);
}

void reconfigP2P() {
  char buff[128];
  sprintf(buff, "AT+TEST=RFCFG,%.3f,SF%d,%d,8,8,%d,OFF,OFF,OFF", (myFreq / 1e6), mySF, myBW, myTx);
  Serial.println(buff);
  displayLines(buff);
  Serial2.println(buff);
}

void handleVER() {
  Serial2.println("AT+VER");
  Serial.println("AT+VER");
}

void handleSF() {
  uint32_t num = getNumber();
  char buff[64];
  setupMenu();
  bool valid = false;
  if (num == -1) sprintf(buff, "You cancelled!");
  else if (num < 7 || num > 12) sprintf(buff, "Incorrect value: %d. Choices are 7~12.", num);
  else {
    sprintf(buff, "SF set to %d", num);
    mySF = num & 0xFF;
    valid = true;
  }
  //posY = 100;
  Serial.println(buff);
  displayLines(buff);
  if (valid) reconfigP2P();
}

void handleBW() {
  uint32_t num = getNumber();
  char buff[64];
  setupMenu();
  bool valid = false;
  if (num == -1) sprintf(buff, "You cancelled!");
  else if (num == 125 || num == 250 || num == 500) {
    sprintf(buff, "BW set to %d", num);
    myBW = num & 0xFFFF;
    valid = true;
  } else sprintf(buff, "Incorrect value: %d. Choices are 125, 250, 500.", num);
  Serial.println(buff);
  displayLines(buff);
  if (valid) reconfigP2P();
}

void handleTX() {
  uint32_t num = getNumber();
  char buff[64];
  setupMenu();
  bool valid = false;
  if (num == -1) sprintf(buff, "You cancelled!");
  else if (num < 14 || num > 22) sprintf(buff, "Incorrect value: %d. Choices are 14~22.", num);
  else {
    sprintf(buff, "Tx Power set to %d", num);
    myTx = num & 0xFF;
    valid = true;
  }
  //posY = 100;
  Serial.println(buff);
  displayLines(buff);
  if (valid) reconfigP2P();
}

void handleFreq() {
  uint32_t num = getNumber();
  char buff[64];
  setupMenu();
  bool valid = false;
  if (num < 1000) num *= 1e6;
  // 868 ----> 868e6
  if (num == -1) sprintf(buff, "You cancelled!");
  else if (num < 860e6 || num > 960e6) sprintf(buff, "Incorrect value: %d. Choices are 860~960.", num);
  else {
    sprintf(buff, "Frequency set to %.3f MHz", (num / 1e6));
    myFreq = num;
    valid = true;
  }
  //posY = 100;
  Serial.println(buff);
  displayLines(buff);
  if (valid) reconfigP2P();
}

void handleCFG() {
  Serial2.println("AT+TEST=RFCFG");
  Serial.println("AT+TEST=RFCFG");
}

void handlePING() {
  char msg[96];
  sprintf(msg, "AT+TEST=TXLRSTR,\"{'UUID':'%08d', 'from':'LoRa-E5', 'cmd':'ping'}\"", pingCount++);
  // Can't send a string with quotes in it, and can't escape the quotes, gggrrrrrr.
  // Might have to send a HEX string instead
  // sprintf(msg, "AT+TEST=TXLRSTR,\"{\\\"UUID\\\":\\\"%08d\\\", \\\"from\\\":\\\"LoRa-E5\\\", \\\"msg\\\":\\\"ping\\\"}\"", pingCount++);
  Serial2.println(msg);
  Serial.println(msg);
  delay(100);
  listenMode();
}

menuButton myMenus[] = {
  {handleVER, 10, 40, 10, "VER"},
  {handleCFG, 70, 40, 10, "CFG"},
  {handleSF, 10, 40, 10, "SF"},
  {handleBW, 10, 40, 10, "BW"},
  {handleFreq, 10, 40, 10, "Freq"},
  {handleTX, 10, 40, 10, "Tx"},
  {handlePING, 140, 40, 10, "PING"},
}; // tx ty and w are assigned at runtime.
int btnCount = 0;

void setupMenu() {
  lcd.fillScreen(TFT_WHITE);
  lcd.setTextSize(3);
  lcd.setCursor(5, 5);
  lcd.setTextColor(TFT_BLACK);
  lcd.print("LoRa-E5 Commander");
  lcd.setTextSize(2);
  btnCount = sizeof(myMenus) / sizeof(menuButton);
  int posXB = 10, posYB = 35;
  for (uint8_t i = 0; i < btnCount; i++) {
    int w;
    w = strlen(myMenus[i].name) * 15 + 10;
    myMenus[i].tx = posXB;
    myMenus[i].ty = posYB;
    myMenus[i].w = w;
    menuButton bn = myMenus[i];
    posXB += (w + 10);
    if (posXB > 400) {
      posXB = 10;
      posYB += 45;
    }
    int bx = bn.tx + w, by = bn.ty + 40 ;
    Serial.printf("Menu %d: %s at %d:%d %d:%d, width: %d\n", i, bn.name, bn.tx, bn.ty, bx, by, w);
    Serial.printf(" . posXB: %d, posYB:%d\n", i, posXB, posYB);
    drawMyButton(bn);
  }
  if (posXB > 10) posY = posYB + 45;
}

void displayLines(char *buff) {
  lcd.setTextColor(TFT_BLACK);
  uint8_t offset = 0;
  while (strlen(buff) - offset > 38) {
    if (posY > 300) {
      for (uint8_t ii = 0; ii < 5; ii++) lcd.scroll(0, -4);
      posY = 300;
    }
    lcd.setCursor(2, posY);
    char c = buff[38]; // Save 39th character
    buff[offset + 38] = 0; // Put a string terminator
    lcd.println(buff + offset);
    posY += 20;
    buff[offset + 38] = c; // Restore 39th char
    offset += 38;
  }
  if (posY > 300) {
    for (uint8_t ii = 0; ii < 5; ii++) lcd.scroll(0, -4);
    posY = 300;
  }
  lcd.setCursor(4, posY);
  lcd.println(buff + offset);
  posY += 20;
}
