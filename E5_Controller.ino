uint32_t getNumber();
void setupMenu();
void displayLines(char * );
void listenMode();

#define LGFX_USE_V1

#include <vector>
#include <string>
#include <LovyanGFX.hpp>
#include <SPI.h>
#include <BBQ10Keyboard.h> // http://librarymanager/All#BBQ10Keyboard

using namespace std;

BBQ10Keyboard keyboard;
// Stuff for the keyboard
// See https://github.com/solderparty/bbq10kbd_i2c_sw
bool SYM = false; // Is the SYM key being held down
#define _SYM_KEY 19 // Key code
#define CFG_REPORT_MODS 0b01000000
// https://github.com/solderparty/bbq10kbd_i2c_sw#the-configuration-register-reg_cfg--0x02
#define _REG_CFG 0x02
#define _REG_KEY 0x04
#define bbqLimit 16
char bbqBuff[bbqLimit + 1] = {0};
uint8_t bbqIndex = 0;
uint32_t bbqDELAY = 6000, lastKbdInput;
bool hasBBQ10 = false, isBBQing = false;

#define I2C_SCL 39
#define I2C_SDA 38

#include "FT6236.h"
const int i2c_touch_addr = TOUCH_I2C_ADD;
#define get_pos ft6236_pos

#define SPI_MOSI 2 //1
#define SPI_MISO 41
#define SPI_SCK 42
//#define SD_CS 1 //2
#define LCD_CS 37
#define LCD_BLK 45
#define RXD2 18
#define TXD2 17

class LGFX : public lgfx::LGFX_Device {
    // lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_Parallel16 _bus_instance; // 8ビットパラレルバスのインスタンス (ESP32のみ)
  public:
    // コンストラクタを作成し、ここで各種設定を行います。
    // クラス名を変更した場合はコンストラクタも同じ名前を指定してください。
    LGFX(void) {
      { // バス制御の設定を行います。
        auto cfg = _bus_instance.config(); // バス設定用の構造体を取得します。
        // 16位设置
        cfg.port = 0;              // 使用するI2Sポートを選択 (0 or 1) (ESP32のI2S LCDモードを使用します)
        cfg.freq_write = 20000000; // 送信クロック (最大20MHz, 80MHzを整数で割った値に丸められます)
        cfg.pin_wr = 35;           // WR を接続しているピン番号
        cfg.pin_rd = 48;           // RD を接続しているピン番号
        cfg.pin_rs = 36;           // RS(D/C)を接続しているピン番号
        cfg.pin_d0 = 47;
        cfg.pin_d1 = 21;
        cfg.pin_d2 = 14;
        cfg.pin_d3 = 13;
        cfg.pin_d4 = 12;
        cfg.pin_d5 = 11;
        cfg.pin_d6 = 10;
        cfg.pin_d7 = 9;
        cfg.pin_d8 = 3;
        cfg.pin_d9 = 8;
        cfg.pin_d10 = 16;
        cfg.pin_d11 = 15;
        cfg.pin_d12 = 7;
        cfg.pin_d13 = 6;
        cfg.pin_d14 = 5;
        cfg.pin_d15 = 4;
        _bus_instance.config(cfg);              // 設定値をバスに反映します。
        _panel_instance.setBus(&_bus_instance); // バスをパネルにセットします。
      }
      { // 表示パネル制御の設定を行います。
        auto cfg = _panel_instance.config(); // 表示パネル設定用の構造体を取得します。
        cfg.pin_cs = -1;   // CS要拉低
        cfg.pin_rst = -1;  // RST和开发板RST相连
        cfg.pin_busy = -1; // BUSYが接続されているピン番号 (-1 = disable)
        // ※ 以下の設定値はパネル毎に一般的な初期値が設定されていますので、不明な項目はコメントアウトして試してみてください。
        cfg.memory_width = 320;   // ドライバICがサポートしている最大の幅
        cfg.memory_height = 480;  // ドライバICがサポートしている最大の高さ
        cfg.panel_width = 320;    // 実際に表示可能な幅
        cfg.panel_height = 480;   // 実際に表示可能な高さ
        cfg.offset_x = 0;         // パネルのX方向オフセット量
        cfg.offset_y = 0;         // パネルのY方向オフセット量
        cfg.offset_rotation = 0;  // 回転方向の値のオフセット 0~7 (4~7は上下反転)
        cfg.dummy_read_pixel = 8; // ピクセル読出し前のダミーリードのビット数
        cfg.dummy_read_bits = 1;  // ピクセル以外のデータ読出し前のダミーリードのビット数
        cfg.readable = true;      // データ読出しが可能な場合 trueに設定
        cfg.invert = false;       // パネルの明暗が反転してしまう場合 trueに設定
        cfg.rgb_order = false;    // パネルの赤と青が入れ替わってしまう場合 trueに設定
        cfg.dlen_16bit = true;    // データ長を16bit単位で送信するパネルの場合 trueに設定
        cfg.bus_shared = true;    // SDカードとバスを共有している場合 trueに設定(drawJpgFile等でバス制御を行います)
        _panel_instance.config(cfg);
      }
      setPanel(&_panel_instance); // 使用するパネルをセットします。
    }
};
LGFX lcd;
int pos[2] = {0, 0};
int err_code = 0;
int posY = 100;
#include "MainMenu.h"
#include "Keypad.h"

void listenMode() {
  Serial2.println("AT+TEST=RXLRPKT");
}

void hex2array(char *src, size_t sLen) {
  size_t i, n = 0;
  for (i = 0; i < sLen; i += 2) {
    uint8_t x, c;
    Serial.write(src[i + 11]);
    Serial.write(src[i + 12]);
    Serial.write(' ');
    c = src[i + 11] - 48;
    if (c > 9) c -= 55;
    x = c << 4;
    c = src[i + 12] - 48;
    if (c > 9) c -= 55;
    src[n++] = (x + c);
  } src[n] = 0;
  Serial.write('\n');
}

void setup() {
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_BLK, HIGH);
  Serial.begin(115200);
  delay(2000);

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("Serial Txd is on pin: " + String(TX));
  Serial.println("Serial Rxd is on pin: " + String(RX));

  lcd.init();
  lcd.fillScreen(TFT_WHITE);
  lcd.setTextColor(TFT_BLACK);
  lcd.setTextSize(2);
  lcd.setCursor(0, 0);
  lcd.print("Makerfabs ESP32-S3");
  lcd.setCursor(0, 16);
  lcd.print("Parallel TFT with Touch");
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  lcd.setCursor(0, 32);
  //I2C init
  Wire.begin(I2C_SDA, I2C_SCL);
  byte error, address;
  Wire.beginTransmission(i2c_touch_addr);
  error = Wire.endTransmission();
  lcd.setCursor(0, 48);
  if (error == 0) {
    Serial.print("I2C device found at address 0x");
    Serial.print(i2c_touch_addr, HEX);
    Serial.println("  !");
    lcd.print("TOUCH INIT OVER");
  } else {
    Serial.print("Unknown error at address 0x");
    Serial.println(i2c_touch_addr, HEX);
    lcd.print("ERROR:   TOUCH");
    err_code += 1;
  }
  lcd.setCursor(0, 64);
  if (err_code) {
    lcd.print("SOMETHING WRONG");
    while (1) ;
  } else lcd.print("ALL SUCCESS");
  delay(1500);
  lcd.setRotation(3);
  setupMenu();
  String dump = Serial2.readString();
  lcd.setScrollRect(0, posY, lcd.width(), 320 - posY, 0xffff);
  Serial2.println("AT+MODE=TEST");
  delay(100);
  dump = Serial2.readString();
  reconfigP2P();
  dump = Serial2.readString();
  listenMode();
  dump = Serial2.readString();

  Wire.beginTransmission(0x1f);
  error = Wire.endTransmission();
  if (error == 0) {
    Serial.println("BBQ10 Keyboard present!");
    keyboard.begin();
    keyboard.setBacklight(0.5f);
    keyboard.setBacklight2(1.0f);
    uint8_t cfg = keyboard.readRegister8(_REG_CFG);
    Serial.printf(" . cfg: %08x\n", cfg);
    // Report SYM, ALT, KEYCAPS etc
    cfg |= CFG_REPORT_MODS;
    Serial.printf(" . cfg: %08x\n", cfg);
    keyboard.writeRegister(_REG_CFG, cfg);
    hasBBQ10 = true;
  } else {
    Serial.println("Couldn't find keyboard!");
  }
}

void loop() {
  if (hasBBQ10) {
    // If you don't have an OLED it's gonna be fun...
    // Watch the Serial Monitor!
    const int keyCount = keyboard.keyCount();
    if (keyCount == 0 && isBBQing) {
      uint32_t t0 = millis();
      if (t0 - lastKbdInput > bbqDELAY) {
        Serial.println("bbq10 timeout, reset screen.");
        // restoreScreen();
      }
    } else if (keyCount > 0) {
      lastKbdInput = millis();
      //if (!isBBQing && hasOLED) restoreBBQinput();
      // if (hasOLED) oledLastOn = millis();
      const BBQ10Keyboard::KeyEvent key = keyboard.keyEvent();
      if (key.state == BBQ10Keyboard::StateLongPress) {
        if (key.key == _SYM_KEY) SYM = true;
        return;
      } else if (key.state == BBQ10Keyboard::StateRelease) {
        if (key.key == _SYM_KEY) {
          SYM = false;
          return;
        } else if (key.key > 31) {
          if (bbqIndex < bbqLimit) bbqBuff[bbqIndex++] = key.key;
        } else if (key.key == 8) {
          if (SYM) {
            // SYM+Backspace = erase everything
            // Serial.println(" > Full erase!");
            memset(bbqBuff, 0, bbqLimit);
            bbqIndex = 0;
          } else {
            // Erase one char, if any left
            bbqBuff[bbqIndex] = 0; // Not really necessary but belt and suspenders...
            if (bbqIndex > 0) bbqIndex -= 1;
            bbqBuff[bbqIndex] = 0;
          }
          Serial.println(bbqBuff);
          //          if (hasOLED) {
          //            // easiest way to redraw after erase is to draw off-screen. then dump the buffer
          //            oledFill(&oled, 0, 0);
          //            oledWriteString(&oled, 0, 10, 0, "BBQ10", FONT_16x16, 0, 0);
          //            oledWriteString(&oled, 0, 0, 2, ">", FONT_8x8, 0, 0);
          //            if (bbqIndex > 0) oledWriteString(&oled, 0, 8, 2, bbqBuff, FONT_8x8, 0, 0);
          //            oledDumpBuffer(&oled, ucBuffer);
          //          }
        } else if (key.key == 10) {
          // enter = let's go!
          // restoreScreen(); // back to what it was
          // handleCommands(bbqBuff);
          Serial.println("Final input:");
          Serial.println(bbqBuff);
          memset(bbqBuff, 0, bbqLimit);
          bbqIndex = 0;
          return;
        }
        if (key.key > 31) {
          //          if (hasOLED) {
          //            for (uint8_t x = 0; x < 128; x++) ucBuffer[x + 256] = 0;
          //            oledWriteString(&oled, 0, 0, 2, ">", FONT_8x8, 0, 0);
          //            oledWriteString(&oled, 0, 8, 2, bbqBuff, FONT_8x8, 0, 0);
          //            oledDumpBuffer(&oled, ucBuffer);
          //          }
          Serial.printf("BBQ10 Buffer [%d]: %s\n", bbqIndex, bbqBuff);
        }
      }
    }
  }
  if (Serial2.available()) {
    Serial.println("Incoming from S2:");
    char buff[256];
    uint8_t ix = 0;
    while (Serial2.available()) {
      char c = Serial2.read();
      if (c == 10) {
        buff[ix] = 0;
        if (strncmp(buff, "+TEST: RX ", 10) == 0) {
          // Let's decode
          hex2array(buff, strlen(buff) - 12);
        }
        Serial.println(buff);
        displayLines(buff);
        ix = 0;
      } else if (c > 31) buff[ix++] = c;
      delay(10);
    }
    if (ix > 0) {
      buff[ix] = 0;
      Serial.println(buff);
      displayLines(buff);
    }
  }
  if (get_pos(pos)) {
    int tempx = 480 - pos[1];
    int tempy = pos[0];
    pos[0] = tempx;
    pos[1] = tempy;
    Serial.println((String)"x=" + pos[0] + ", y=" + pos[1]);
    bool found = false;
    for (uint8_t i = 0; i < btnCount; i++) {
      menuButton bn = myMenus[i];
      int w = bn.w, bx = bn.tx + w, by = bn.ty + 40;
      Serial.printf(" . Menu %d: %s at %d:%d %d:%d, width: %d\n", i, bn.name, bn.tx, bn.ty, bx, by, bn.w);
      if (pos[0] > bn.tx && pos[0] < bx && pos[1] > bn.ty && pos[1] < by) {
        Serial.printf("Menu #%d matches: %s. %d:%d\n", i, bn.name, bn.tx, bn.ty);
        int w;
        w = strlen(bn.name) * 15 + 10;
        lcd.fillRoundRect(bn.tx, bn.ty, w + 3, 43, 4, TFT_WHITE);
        lcd.fillRoundRect(bn.tx + 3, bn.ty + 3, w, 40, 4, TFT_PURPLE);
        lcd.setTextColor(TFT_WHITE);
        lcd.setCursor(bn.tx + 9, bn.ty + 12);
        lcd.print(bn.name);
        lcd.setCursor(bn.tx + 10, bn.ty + 13);
        lcd.print(bn.name);
        int posx[2];
        while (get_pos(posx)) delay(10);
        lcd.fillRoundRect(bn.tx, bn.ty, w + 3, 43, 4, TFT_WHITE);
        drawMyButton(bn);
        bn.ptr();
        found = true;
        break;
      }
    }
    if (!found) while (get_pos(pos)) delay(10);
  }
}
