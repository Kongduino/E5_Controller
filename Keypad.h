#include <string> // std::string
#include <sstream> // std::stringstream

struct digitButton {
  int16_t tx, ty, w;
  char name;
};

digitButton myDigits[] = {
  {10, 40, 10, '7'},
  {10, 40, 10, '8'},
  {10, 40, 10, '9'},
  {10, 40, 10, '4'},
  {10, 40, 10, '5'},
  {10, 40, 10, '6'},
  {10, 40, 10, '1'},
  {10, 40, 10, '2'},
  {10, 40, 10, '3'},
  {10, 40, 10, '0'},
  {10, 40, 10, 'x'},
  {10, 40, 10, 'o'},
  {10, 40, 10, '?'},
  {10, 40, 10, '<'},
}; // tx ty are assigned at runtime.
int digitCount = 0;

void drawMyDigit(digitButton bn) {
  lcd.fillRoundRect(bn.tx + 1, bn.ty + 1, bn.w, 40, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx + 2, bn.ty + 2, bn.w, 50, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx + 3, bn.ty + 3, bn.w, 50, 4, TFT_BROWN);
  lcd.fillRoundRect(bn.tx, bn.ty, bn.w, 50, 4, TFT_RED);
  lcd.setTextColor(TFT_WHITE);
  lcd.setCursor(bn.tx + 14, bn.ty + 11);
  lcd.print(bn.name);
  lcd.setCursor(bn.tx + 15, bn.ty + 12);
  if (bn.name == '?') lcd.print("000");
  else lcd.print(bn.name);
}

uint32_t getNumber() {
  digitCount = sizeof(myDigits) / sizeof(digitButton);
  stringstream ss;
  lcd.fillScreen(TFT_WHITE);
  lcd.setTextSize(3);
  int posXB = 250, posYB = 40;
  uint8_t i;
  for (i = 0; i < digitCount; i++) {
    int w = 50;
    myDigits[i].tx = posXB;
    myDigits[i].ty = posYB;
    myDigits[i].w = w;
    if (myDigits[i].name == '?') myDigits[i].w = 80;
    digitButton bn = myDigits[i];
    posXB += (bn.w + 10);
    if (posXB > 400) {
      posXB = 250;
      posYB += 55;
    }
    int bx = bn.tx + w, by = bn.ty + 40 ;
    Serial.printf("Menu %d: %c at %d:%d %d:%d, width: %d\n", i, bn.name, bn.tx, bn.ty, bx, by, w);
    Serial.printf(" . posXB: %d, posYB:%d\n", i, posXB, posYB);
    drawMyDigit(bn);
  }
  while (true) {
    if (get_pos(pos)) {
      int tempx = 480 - pos[1];
      int tempy = pos[0];
      pos[0] = tempx;
      pos[1] = tempy;
      Serial.println((String)"x=" + pos[0] + ", y=" + pos[1]);
      bool found = false;
      for (uint8_t i = 0; i < digitCount; i++) {
        digitButton bn = myDigits[i];
        int w = bn.w, bx = bn.tx + w, by = bn.ty + 40;
        Serial.printf(" . Button %d: %c at %d:%d %d:%d, width: %d\n", i, bn.name, bn.tx, bn.ty, bx, by, bn.w);
        if (pos[0] > bn.tx && pos[0] < bx && pos[1] > bn.ty && pos[1] < by) {
          Serial.printf("Button #%d matches: %c. %d:%d\n", i, bn.name, bn.tx, bn.ty);
          lcd.fillRoundRect(bn.tx, bn.ty, w + 3, 53, 4, TFT_WHITE);
          lcd.fillRoundRect(bn.tx + 3, bn.ty + 3, w, 50, 4, TFT_PURPLE);
          lcd.setTextColor(TFT_WHITE);
          lcd.setCursor(bn.tx + 14, bn.ty + 14);
          lcd.print(bn.name);
          lcd.setCursor(bn.tx + 15, bn.ty + 15);
          lcd.print(bn.name);
          int posx[2];
          while (get_pos(posx)) delay(10);
          lcd.fillRoundRect(bn.tx, bn.ty, w + 3, w + 3, 4, TFT_WHITE);
          drawMyDigit(bn);
          if (bn.name == 'x') return -1;
          if (bn.name == 'o') {
            Serial.printf("Returning %d\n", atoi(ss.str().c_str()));
            return atoi(ss.str().c_str());
          }
          if (bn.name == '?') ss << "000";
          else if (bn.name == '<') {
            // backspace: remove last char
            string foo(ss.str());
            if (foo.size() > 0) {
              // pop_back on an empty string is... fun?
              foo.pop_back();
              ss.str(foo);
              ss.seekp (0, ss.end);
            }
          } else ss << bn.name;
          lcd.fillRect(240, 0, 170, 31, TFT_WHITE);
          lcd.setTextColor(TFT_PURPLE);
          lcd.setCursor(250, 10);
          lcd.print(ss.str().c_str());
          // bn.ptr(bn.name);
          found = true;
          break;
        }
      }
      if (!found) while (get_pos(pos)) delay(10);
    }
  }
}
