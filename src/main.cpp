/**
 * ----------------------------------------------------------------------------
 * PROJECT: ESP32-S3 Smart Vehicle Warning System (Full Features)
 * HARDWARE: ESP32-S3 + 64x64 HUB75 + MQ-3 + Buzzer + MOSFET
 * VERSION: 2.1 (Added Speed & Size Control)
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FS.h>
#include <LittleFS.h>
#include <OpenFontRender.h>

// ============================================================================
// 1. REMOTEXY CONFIGURATION
// ============================================================================
#define REMOTEXY_MODE__ESP32CORE_BLE
#include <BLEDevice.h>
#include <RemoteXY.h>

#define REMOTEXY_BLUETOOTH_NAME "Smart_Warning"

#pragma pack(push, 1)  
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] =   // 216 bytes V19 
  { 255,104,0,66,0,209,0,19,0,0,0,83,109,97,114,116,86,101,104,105,
  99,108,101,95,87,97,114,110,105,110,103,32,0,29,1,106,200,1,1,5,
  0,12,4,41,98,14,199,31,26,65,108,99,111,104,111,108,32,84,101,115,
  116,101,114,0,84,114,105,97,110,103,108,101,32,76,105,103,104,116,0,66,
  117,122,122,101,114,32,65,108,97,114,109,0,77,97,116,114,105,120,32,66,
  114,105,103,104,116,110,101,115,115,0,80,114,101,115,101,116,32,77,101,115,
  115,97,103,101,115,0,67,117,115,116,111,109,32,77,101,115,115,97,103,101,
  0,84,101,120,116,32,67,111,108,111,114,0,84,101,120,116,32,83,112,101,
  101,100,0,84,101,120,116,32,83,105,122,101,0,1,84,96,12,12,7,31,
  24,226,158,161,0,1,10,96,12,12,7,31,24,226,172,133,0,7,2,135,
  102,9,69,0,79,26,101,67,4,81,98,9,5,2,26,66 };
  
struct {
  // input variables
  uint8_t optionSelector_01; // from 0 to 8 (9 items)
  uint8_t button_02; // =1 if button pressed, else =0, from 0 to 1
  uint8_t button_01; // =1 if button pressed, else =0, from 0 to 1
  char edit_01[101]; // string UTF8, Increased to 100 for safety

  // output variables
  char value_01[66]; // string UTF8

  // other variable
  uint8_t connect_flag;  // =1 if connected

} RemoteXY;   
#pragma pack(pop)

// ============================================================================
// 2. HARDWARE PIN DEFINITIONS
// ============================================================================
#define PIN_SHARED_CTRL 2   // LOW=酒測(Sensor ON), HIGH=三角燈(Light ON)
#define PIN_BUZZER      42  // 蜂鳴器 PWM
#define PIN_MQ3         1   // 酒精感測器 ADC
const int PIN_RGB_BUILTIN = 38;   // Built-in RGB LED (NeoPixel)

// HUB75 Pins
#define R1 4
#define G1 41
#define B1 5
#define R2 6
#define G2 40
#define B2 7
#define A_PIN 15
#define B_PIN 48
#define C_PIN 16
#define D_PIN 47
#define E_PIN 39 
#define LAT 21
#define OE 18
#define CLK 17

#define RES_X 64
#define RES_Y 64

MatrixPanel_I2S_DMA *dma_display = nullptr;
OpenFontRender render;
uint8_t *fontBuffer = nullptr;

// ============================================================================
// 3. LOGIC VARIABLES
// ============================================================================
int alcoholMode = 1;   // 0:OFF, 1:mg/L, 2:PPM
bool triangleState = false; 
int buzzerVol = 0;     
int brightness = 60;   
int presetIndex = 0;   
bool useCustomText = false;
int colorIndex = 0;    

// **字體變數**
int textSpeed = 2;     // 1 (Slow) ~ 10 (Fast)
int fontSize = 48;     // 8 ~ 60, Default 48

// 狀態偵測 (防塞車)
uint8_t prev_selector = 255; 
uint8_t prev_btn1 = 0;
uint8_t prev_btn2 = 0;

// 跑馬燈變數
float currentScrollX = -RES_X;
unsigned long lastDrawTime = 0;
int canvasWidth = 0;
bool updateCanvasNeeded = true; 

// 內建警語
const char* preset_msgs[] = {
    "注意! 前方車禍 Slow Down!",
    "車輛拋錨 請閃避 Breakdown!",
    "臨時停車 請繞道 Temp.",
    "前方施工 請減速 Road Work.",
    "前方塞車 小心追撞 Traffic Jam.",
    "濃霧小心 開霧燈 Foggy!",
    "保持車距 Keep Distance.",
    "緊急求救 請幫忙 S.O.S.",
    "系統測試 System Check..."
};

// 顏色表
const char* colorNames[] = {"Rainbow", "White", "Gray", "Red", "Orange", "Yellow", "Green", "Blue", "Indigo", "Violet"};
uint16_t colorMap[] = {
    0xFFFF, 0xFFFF, 0x8410, 0xF800, 0xFD20, 0xFFE0, 0x07E0, 0x001F, 0x4810, 0x780F
};

// *** HARDWARE CALIBRATION ***
// Waveshare panels often have a manufacturing quirk where the bottom half
// (Rows 32-63) is shifted by 1 pixel.
int bottomHalfOffset = 1;     // Try 1, -1, or 0 to align the image.

// ============================================================================
// 4. PSRAM CANVAS CLASS
// ============================================================================
class PSRAMCanvas : public Adafruit_GFX {
public:
    uint16_t* buffer;
    PSRAMCanvas(int16_t w, int16_t h) : Adafruit_GFX(w, h) {
        buffer = (uint16_t*) ps_calloc(w * h, sizeof(uint16_t));
    }
    ~PSRAMCanvas() { if(buffer) free(buffer); }
    
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        // Custom Draw: Applies the "Hardware Fix" automatically
        // Shift bottom half of screen if calibration is set
        if (y >= 32) x = x + bottomHalfOffset;
        if (x >= 0 && x < _width && y >= 0 && y < _height) {
            buffer[y * _width + x] = color;
        }
    }
    void fillScreen(uint16_t color) override { 
        if(buffer) memset(buffer, color, _width * _height * sizeof(uint16_t));
    }
};
PSRAMCanvas *bigCanvas = nullptr;

// ============================================================================
// 5. HELPER FUNCTIONS
// ============================================================================
uint16_t colorHSV(long hue, uint8_t sat, uint8_t val) {
    uint8_t r, g, b;
    unsigned char region, remainder, p, q, t;
    if (sat == 0) { r = val; g = val; b = val; }
    else {
        region = hue / 43; remainder = (hue - (region * 43)) * 6;
        p = (val * (255 - sat)) >> 8;
        q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
        t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
        switch (region) {
            case 0: r = val; g = t; b = p; break;
            case 1: r = q; g = val; b = p; break;
            case 2: r = p; g = val; b = t; break;
            case 3: r = p; g = q; b = val; break;
            case 4: r = t; g = p; b = val; break;
            default: r = val; g = p; b = q; break;
        }
    }
    return dma_display->color565(r, g, b);
}

void preRenderText() {
    if (bigCanvas) { delete bigCanvas; bigCanvas = nullptr; }
    
    // 設定字體大小
    render.setFontSize(fontSize);
    
    String txt = useCustomText ? String(RemoteXY.edit_01) : String(preset_msgs[presetIndex]);
    if (txt.length() == 0) txt = " ";

    int textW = render.getTextWidth(txt.c_str());
    canvasWidth = textW + 20;

    bigCanvas = new PSRAMCanvas(canvasWidth, RES_Y);
    if (!bigCanvas->buffer) return;

    render.setDrawer(*bigCanvas);
    render.setFontColor(0xFFFF); 
    
    // 垂直置中計算：(螢幕高 - 字體高) / 2 + 微調
    int yPos = ((RES_Y - fontSize) / 2) - 4; 
    render.setCursor(0, yPos); 
    render.printf(txt.c_str());
    
    uint16_t userColor = colorMap[colorIndex];
    for (int y = 0; y < RES_Y; y++) {
        for (int x = 0; x < canvasWidth; x++) {
            uint16_t pixel = bigCanvas->buffer[y * canvasWidth + x];
            if (pixel != 0) { 
                if (colorIndex == 0) { // Rainbow
                    int hue = (int)(x * 0.5) % 255;
                    bigCanvas->buffer[y * canvasWidth + x] = colorHSV(hue, 255, 255);
                } else { 
                    bigCanvas->buffer[y * canvasWidth + x] = userColor;
                }
            }
        }
    }
    currentScrollX = -RES_X; 
    updateCanvasNeeded = false;
}

// ============================================================================
// 6. MENU HANDLER (Updated for Speed & Size)
// ============================================================================
void handleRemoteXYInput() {
    bool btn1_click = (RemoteXY.button_01 == 1 && prev_btn1 == 0);
    bool btn2_click = (RemoteXY.button_02 == 1 && prev_btn2 == 0);
    prev_btn1 = RemoteXY.button_01;
    prev_btn2 = RemoteXY.button_02;

    bool selector_changed = (RemoteXY.optionSelector_01 != prev_selector);
    prev_selector = RemoteXY.optionSelector_01;
    bool need_ui_update = selector_changed || btn1_click || btn2_click;

    static unsigned long lastSensorUpdate = 0;
    bool time_to_update_sensor = (millis() - lastSensorUpdate > 500);

    switch (RemoteXY.optionSelector_01) {
        
        // 0: Alcohol
        case 0:
            if (btn1_click) alcoholMode = (alcoholMode - 1 + 3) % 3; 
            if (btn2_click) alcoholMode = (alcoholMode + 1) % 3;
            if (need_ui_update) {
                if (alcoholMode == 0) strcpy(RemoteXY.value_01, "OFF");
                else if (alcoholMode == 1) strcpy(RemoteXY.value_01, "mg/L");
                else strcpy(RemoteXY.value_01, "PPM");
                
                if (alcoholMode > 0) {
                    digitalWrite(PIN_SHARED_CTRL, LOW); triangleState = false;
                } else {
                    digitalWrite(PIN_SHARED_CTRL, HIGH); triangleState = true;
                    sprintf(RemoteXY.edit_01, "Sensor OFF");
                }
            }
            if (alcoholMode > 0 && time_to_update_sensor) {
                int adc = analogRead(PIN_MQ3);
                // 公式， 演算法
                // Calculate sensor voltage (ADC to V conversion, with voltage divider compensation)
                float sensor_volt = (adc / 4095.0) * 3.3 * 1.50 + 0.00;
                // Convert voltage to alcohol using linear mapping (no clamps)
                float ppm = exp((sensor_volt + 1.53338f) / 0.95387f) - 9.84144f;
                if (ppm < 0) ppm = 0;
                // Convert ppm to mg/L
                float mgL = (ppm / 500.0f);


                if (alcoholMode == 1) sprintf(RemoteXY.edit_01, "%.3f mg/L", mgL);
                else sprintf(RemoteXY.edit_01, "%d PPM", (int)(ppm));
                lastSensorUpdate = millis();
            }
            break;

        // 1: Triangle
        case 1:
            if (btn1_click || btn2_click) triangleState = !triangleState;
            if (need_ui_update) {
                if (triangleState) {
                    strcpy(RemoteXY.value_01, "ON"); digitalWrite(PIN_SHARED_CTRL, HIGH); alcoholMode = 0; 
                } else {
                    strcpy(RemoteXY.value_01, "OFF"); digitalWrite(PIN_SHARED_CTRL, LOW); 
                }
                sprintf(RemoteXY.edit_01, "Light: %s", triangleState ? "Active" : "OFF");
            }
            break;

        // 2: Buzzer
        case 2:
            if (btn1_click && buzzerVol > 0) buzzerVol -= 10;
            if (btn2_click && buzzerVol < 100) buzzerVol += 10;
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "%d%%", buzzerVol);
                sprintf(RemoteXY.edit_01, "Volume Setting");
                ledcWrite(0, map(buzzerVol, 0, 100, 0, 255)); 
            }
            break;

        // 3: Brightness
        case 3:
            if (btn1_click && brightness > 0) brightness -= 10;
            if (btn2_click && brightness < 255) brightness += 10;
            if (brightness > 255) brightness = 255;
            if (brightness < 0) brightness = 0;
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "%d", brightness);
                sprintf(RemoteXY.edit_01, "LED Brightness");
                dma_display->setBrightness8(brightness);
            }
            break;

        // 4: Preset
        case 4:
            if (btn1_click) { presetIndex = (presetIndex - 1 + 9) % 9; updateCanvasNeeded = true; useCustomText = false; }
            if (btn2_click) { presetIndex = (presetIndex + 1) % 9; updateCanvasNeeded = true; useCustomText = false; }
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "Mode %d", presetIndex + 1);
                snprintf(RemoteXY.edit_01, 101, "%s", preset_msgs[presetIndex]); 
            }
            break;

        // 5: Custom
        case 5:
            if (btn1_click) { useCustomText = false; updateCanvasNeeded = true; }
            if (btn2_click) { useCustomText = true; updateCanvasNeeded = true; }
            if (need_ui_update) {
                strcpy(RemoteXY.value_01, useCustomText ? "ON" : "OFF");
            }
            break;

        // 6: Color
        case 6:
            if (btn1_click) { colorIndex = (colorIndex - 1 + 10) % 10; updateCanvasNeeded = true; }
            if (btn2_click) { colorIndex = (colorIndex + 1) % 10; updateCanvasNeeded = true; }
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "%s", colorNames[colorIndex]);
                sprintf(RemoteXY.edit_01, "Color Setting");
            }
            break;

        // --- 7: TEXT SPEED (NEW!) ---
        case 7:
            if (btn1_click && textSpeed > 1) textSpeed--;
            if (btn2_click && textSpeed < 10) textSpeed++;
            
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "Speed: %d", textSpeed);
                sprintf(RemoteXY.edit_01, "1(Slow) - 10(Fast)");
            }
            break;

        // --- 8: TEXT SIZE (NEW!) ---
        case 8:
            if (btn1_click && fontSize > 8) {
                fontSize -= 4; 
                updateCanvasNeeded = true; // 大小改變需重繪
            }
            if (btn2_click && fontSize < 60) {
                fontSize += 4;
                updateCanvasNeeded = true; // 大小改變需重繪
            }
            
            if (need_ui_update) {
                sprintf(RemoteXY.value_01, "%d px", fontSize);
                sprintf(RemoteXY.edit_01, "Default: 48px");
            }
            break;
    }
}

// ============================================================================
// 7. SETUP & LOOP
// ============================================================================
void setup() {
    Serial.begin(115200);
    
    pinMode(PIN_SHARED_CTRL, OUTPUT);
    digitalWrite(PIN_SHARED_CTRL, LOW);
    // Initialize Built-in RGB LED
    // Most S3 RGBs are "Smart LEDs" that need a signal, not just HIGH/LOW.
    // neopixelWrite(PIN, Red, Green, Blue) is a built-in ESP32 function.
    // We send 0,0,0 to force it OFF at startup.
    neopixelWrite(PIN_RGB_BUILTIN, 0, 0, 0);

    ledcSetup(0, 2000, 8); 
    ledcAttachPin(PIN_BUZZER, 0);

    if (!psramInit()) Serial.println("PSRAM Fail");
    if (!LittleFS.begin(false)) Serial.println("FS Fail");
    
    File fontFile = LittleFS.open("/font.ttf", "r");
    if (fontFile) {
        size_t size = fontFile.size();
        fontBuffer = (uint8_t*) ps_calloc(size, sizeof(uint8_t));
        fontFile.read(fontBuffer, size);
        fontFile.close();
        render.loadFont(fontBuffer, size);
    }

    HUB75_I2S_CFG mxconfig(RES_X, RES_Y, 1);
    mxconfig.gpio.r1 = R1; mxconfig.gpio.g1 = G1; mxconfig.gpio.b1 = B1;
    mxconfig.gpio.r2 = R2; mxconfig.gpio.g2 = G2; mxconfig.gpio.b2 = B2;
    mxconfig.gpio.a = A_PIN; mxconfig.gpio.b = B_PIN; mxconfig.gpio.c = C_PIN;
    mxconfig.gpio.d = D_PIN; mxconfig.gpio.e = E_PIN;
    mxconfig.gpio.lat = LAT; mxconfig.gpio.oe = OE; mxconfig.gpio.clk = CLK;
    mxconfig.double_buff = true;
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M; 

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(brightness);

    RemoteXY_Init();
    preRenderText();
}

void loop() {
    RemoteXY_Handler();      
    handleRemoteXYInput();   

    if (updateCanvasNeeded) {
        static unsigned long lastChange = 0;
        if (millis() - lastChange > 200) { 
            preRenderText();
            lastChange = millis();
        }
    }

    if (millis() - lastDrawTime > 20) { 
        dma_display->fillScreen(0);
        if (bigCanvas && bigCanvas->buffer) {
            for (int y = 0; y < RES_Y; y++) {
                uint16_t* rowPtr = bigCanvas->buffer + (y * canvasWidth);
                for (int x = 0; x < RES_X; x++) {
                    int srcX = (int)currentScrollX + x;
                    if (srcX >= 0 && srcX < canvasWidth) {
                        uint16_t c = rowPtr[srcX];
                        if (c > 0) dma_display->drawPixel(x, y, c);
                    }
                }
            }
        }
        dma_display->flipDMABuffer();
        // **使用 Text Speed 變數**
        currentScrollX += textSpeed; 
        if (currentScrollX > canvasWidth) currentScrollX = -RES_X;
        lastDrawTime = millis();
    }
}