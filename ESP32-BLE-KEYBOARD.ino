/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
#include <SPI.h>
#include <ESP32Encoder.h>
#include <BleKeyboard.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

// 引入驱动OLED0.91所需的库
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // 设置OLED宽度,单位:像素
#define SCREEN_HEIGHT 32 // 设置OLED高度,单位:像素
// 自定义重置引脚,虽然教程未使用,但却是Adafruit_SSD1306库文件所必需的
#define OLED_RESET 19
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BleKeyboard bleKeyboard;
ESP32Encoder encoder;

#define BUTTON_PIN_BITMASK 0x8000 // 2^14 in hex  深度睡眠唤醒pin
RTC_DATA_ATTR int bootCount = 0;

//触摸唤醒
#if CONFIG_IDF_TARGET_ESP32
  #define THRESHOLD   40      /* Greater the value, more the sensitivity */
#else //ESP32-S2 and ESP32-S3 + default for other chips (to be adjusted) */
  #define THRESHOLD   5000   /* Lower the value, more the sensitivity */
#endif
touch_pad_t touchPin;

int comment = 0;
//按键配置
#define EC11_A_PIN 25  //旋转编码器
#define EC11_B_PIN 26   //旋转编码器
#define EC11_K_PIN 17  //旋转编码器
#define KEY1 15  //按键1
#define KEY2 13  //按键2
#define KEY3 4   //按键3
#define PIXEL_PIN    5 
#define PIXEL_COUNT 3  // 灯珠的数量
#define BRIGHTNESS 100

uint8_t up[3]={0};
uint8_t enablelock=0;
int lastEncoderValue = 0;
uint32_t new_time = 0;
uint32_t time1 = 0;

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

/*******RGB部分*********/
int lasttime[2];
uint8_t Effet_type=1;
struct RGB_LED
{
   uint8_t enable=0;
   uint8_t bright=250;
   uint8_t r,g,b;
   int last_time;  
} LED[3];

void LED_DISPLAY(uint8_t effet_type){
 switch(effet_type){
  case 1://

 if(LED[2].enable){
    LED_Breathe(2);
 }
 else strip.setPixelColor(2,0, 0,0);
 if(LED[1].enable){
    LED_Breathe(1);
 }
 else strip.setPixelColor(1,0, 0,0);
  if(LED[0].enable){
    LED_Breathe(0);
 }
 else strip.setPixelColor(0,0, 0,0);
 
 break;
  case 2:
  if(LED[0].enable)  Breathe_all(0);
  if(LED[1].enable)  Breathe_all(1);
  if(LED[2].enable)  Breathe_all(2);
  if((LED[0].enable==0)&&(LED[1].enable==0)&&(LED[2].enable==0))
  for(int i=0;i<3;i++)strip.setPixelColor(i,0, 0,0);
   break;
  
 }
  }
void LED_Breathe(uint8_t num){
  switch(num){
   case 0: 
     LED[0].r=0;
     LED[0].g=10;
     LED[0].b=5+LED[0].bright;
     breathe_time();
     strip.setPixelColor(0,LED[0].r, LED[0].g,LED[0].b);
     break;
   case 1:
     LED[1].r=0;
     LED[1].g=10;
     LED[1].b=5+LED[1].bright;
     breathe_time();
     strip.setPixelColor(1,LED[1].r, LED[1].g,LED[1].b);
     break;
   case 2:  
     LED[2].r=0;
     LED[2].g=10;
     LED[2].b=5+LED[2].bright;
     breathe_time();
     strip.setPixelColor(2,LED[2].r, LED[2].g,LED[2].b);
      break;
  }

  
  }
  void breathe_time(){
    if(LED[0].enable&&(up[0]==0)){
      if((millis()-LED[0].last_time)>=1){
        LED[0].bright-=1;
        if(LED[0].bright<=20){ LED[0].bright=250;LED[0].enable=0;}
        LED[0].last_time=millis();
      }
    }
    if(LED[1].enable&&(up[1]==0)){
      if((millis()-LED[1].last_time)>=1){
        LED[1].bright-=1;
        if(LED[1].bright<=20){ LED[1].bright=250;LED[1].enable=0;}
        LED[1].last_time=millis();
      }
    }
    if(LED[2].enable&&(up[2]==0)){
      if((millis()-LED[2].last_time)>=1){
        LED[2].bright-=1;
        if(LED[2].bright<=20){ LED[2].bright=250;LED[2].enable=0;}
        LED[2].last_time=millis();
      }
    }
    }

void Breathe_all(uint8_t LED_NUM){
     LED[LED_NUM].r=0;
     LED[LED_NUM].g=5+LED[LED_NUM].bright;
     LED[LED_NUM].b=10;
     breathe_time();
  for(int i=0;i<3;i++)  strip.setPixelColor(i,LED[LED_NUM].r, LED[LED_NUM].g,LED[LED_NUM].b);

  }
/*******oled部分*********/
#define LOGO_HEIGHT   32
#define LOGO_WIDTH    32
static const unsigned char PROGMEM logo_bmp[] =
{ 0x00,0x07,0x80,0x00,0x00,0x07,0xE0,0x00,0x00,0x07,0xF0,0x00,0x00,0x07,0xF8,0x00,
0x00,0x07,0xFC,0x00,0x00,0x07,0xFF,0x00,0x00,0x07,0xFF,0x80,0x00,0x07,0xBF,0xC0,
0x01,0x07,0xDF,0xE0,0x07,0xC7,0xCF,0xE0,0x07,0xE7,0x9F,0xE0,0x07,0xFF,0xFF,0xC0,
0x03,0xFF,0xFF,0x00,0x01,0xFF,0xFE,0x00,0x00,0x7F,0xF8,0x00,0x00,0x3F,0xF0,0x00,
0x00,0x3F,0xF0,0x00,0x00,0x7F,0xFC,0x00,0x01,0xFF,0xFE,0x00,0x03,0xFF,0xFF,0x00,
0x07,0xFF,0xFF,0xC0,0x07,0xE7,0x9F,0xE0,0x07,0xC7,0xCF,0xE0,0x01,0x07,0xDF,0xE0,
0x00,0x07,0xBF,0xC0,0x00,0x07,0xFF,0x80,0x00,0x07,0xFF,0x00,0x00,0x07,0xFC,0x00,
0x00,0x07,0xF8,0x00,0x00,0x07,0xF0,0x00,0x00,0x07,0xE0,0x00,0x00,0x07,0x80,0x00 };


void inittestscrolltext(void) {
  display.clearDisplay();

  display.drawBitmap(
    0,0,logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);

  display.setTextSize(1.7); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(32, 0);
  display.println(F("BLE KEYBOARD"));
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(BLACK, WHITE);
  display.setCursor(32, 17);
  display.println(F("By GEEKDREAM"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(800);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(800);
  display.stopscroll();
//  delay(1000);
//  display.startscrolldiagright(0x00, 0x07);
//  delay(2000);
//  display.startscrolldiagleft(0x00, 0x07);
//  delay(2000);
//  display.stopscroll();
  delay(1000);
}

void display_Connected(uint32_t new_time)
{
  // 清除屏幕
  display.clearDisplay();

  // 设置字体颜色,白色可见
  display.setTextColor(WHITE);

  //设置字体大小
  display.setTextSize(1.5);

  //设置光标位置
  display.setCursor(0, 0);
  display.print("Bluetooth connected");

  display.setCursor(0, 10);
  display.print("Run time: ");
  //打印自开发板重置以来的秒数：
  display.print(new_time / 1000);
  display.print(" s");
  display.display();
  if (new_time>30000)  //300s休眠
  {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0); //1 = High, 0 = Low
//    touchSleepWakeUpEnable(T2,THRESHOLD);
//    touchSleepWakeUpEnable(T9,THRESHOLD);

    //If you were to use ext1, you would use it like
    //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  
    //Go to sleep now
    display.clearDisplay();
    display.display();
    Serial.println("Going to sleep now");
     //设置光标位置
    // 清除屏幕
    
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }
}

void  display_noConnected(uint32_t new_time)
{
    //设置光标位置
    // 清除屏幕
  display.clearDisplay();

  // 设置字体颜色,白色可见
  display.setTextColor(WHITE);

  //设置字体大小
  display.setTextSize(1.5);
  
  display.setCursor(0, 0);
  display.print("Bluetooth not connected");
  display.display();
  if (new_time>10000)      //无连接30s休眠
  {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_15,0); //1 = High, 0 = Low
//    touchSleepWakeUpEnable(T2,THRESHOLD);
//    touchSleepWakeUpEnable(T9,THRESHOLD);

    //If you were to use ext1, you would use it like
    //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  
    //Go to sleep now
    display.clearDisplay();
    display.display();
    Serial.println("Going to sleep now");
     //设置光标位置
    // 清除屏幕
    
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
  }
}
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

/*
Method to print the touchpad by which ESP32
has been awaken from sleep
*/
void print_wakeup_touchpad(){
  touchPin = esp_sleep_get_touchpad_wakeup_status();

  #if CONFIG_IDF_TARGET_ESP32
    switch(touchPin)
    {
      case 0  : Serial.println("Touch detected on GPIO 4"); break;
      case 1  : Serial.println("Touch detected on GPIO 0"); break;
      case 2  : Serial.println("Touch detected on GPIO 2"); break;
      case 3  : Serial.println("Touch detected on GPIO 15"); break;
      case 4  : Serial.println("Touch detected on GPIO 13"); break;
      case 5  : Serial.println("Touch detected on GPIO 12"); break;
      case 6  : Serial.println("Touch detected on GPIO 14"); break;
      case 7  : Serial.println("Touch detected on GPIO 27"); break;
      case 8  : Serial.println("Touch detected on GPIO 33"); break;
      case 9  : Serial.println("Touch detected on GPIO 32"); break;
      default : Serial.println("Wakeup not by touchpad"); break;
    }
  #else
    if(touchPin < TOUCH_PAD_MAX)
    {
      Serial.printf("Touch detected on GPIO %d\n", touchPin); 
    }
    else
    {
      Serial.println("Wakeup not by touchpad");
    }
  #endif
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  
  ESP32Encoder::useInternalWeakPullResistors = UP;
  encoder.attachSingleEdge(EC11_A_PIN, EC11_B_PIN);
  pinMode(EC11_K_PIN, INPUT_PULLUP);
  
  // pinMode(4, INPUT_PULLUP);//touch0
  pinMode(KEY1, INPUT_PULLUP);
  pinMode(KEY2, INPUT_PULLUP);
  pinMode(KEY3, INPUT_PULLUP);
  // pinMode(27, INPUT_PULLUP);
  // pinMode(33, INPUT_PULLUP);
  // pinMode(32, INPUT_PULLUP);//touch9
  Wire.begin(/*SDA*/21,/*SCL*/22);
  // 初始化OLED并设置其IIC地址为 0x3C
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  bleKeyboard.begin();
  inittestscrolltext();//初始化显示logo单元

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();
  print_wakeup_touchpad();

}

void loop() {

  new_time = millis()-time1;
//  Serial.println(new_time);
  if(bleKeyboard.isConnected()) {
    display_Connected(new_time);
    
  }
  else{
    display_noConnected(new_time);
  }
  if (digitalRead(KEY1) == LOW)
  {
    delay(5);
    if (digitalRead(KEY1) == LOW)
    {
      if (bleKeyboard.isConnected())
      {
//        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          bleKeyboard.write(48);
          display.setCursor(0, 20);
          display.print("key1");
          display.display();
          LED[2].enable=1;
          time1=millis();
      }
    }
    while (digitalRead(KEY1) == LOW)
      ;
  }
  if (digitalRead(KEY2) == LOW)
  {
    delay(5);
    if (digitalRead(KEY2) == LOW)
    {
      if (bleKeyboard.isConnected())
      {
//        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          bleKeyboard.write(49);
          display.setCursor(25, 20);
          display.print("key2");
          display.display();
          LED[1].enable=1;
          time1=millis();
      }
    }
    while (digitalRead(KEY2) == LOW)
      ;
  }
  if (digitalRead(KEY3) == LOW)
  {
    delay(5);
    if (digitalRead(KEY3) == LOW)
    {
      if (bleKeyboard.isConnected())
      {
//        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          bleKeyboard.write(50);
          display.setCursor(50, 20);
          display.print("key3");
          display.display();
          LED[0].enable=1;
          time1=millis();
      }
    }
    while (digitalRead(KEY3) == LOW)
      ;
  }
//  旋转编码器
  if (lastEncoderValue != encoder.getCount())
  {
    int now_count = encoder.getCount();
    if (bleKeyboard.isConnected())
    {
      if (now_count > lastEncoderValue)
      {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
        display.setCursor(75, 20);
        display.print("↓");
        display.display();
        time1=millis();
      }
      else
      {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        display.setCursor(75, 20);
        display.print("↑");
        display.display();
        time1=millis();
      }
    }
    lastEncoderValue = now_count;
    Serial.print("Encoder value: ");
    Serial.println(lastEncoderValue);
  }
 
  if (digitalRead(EC11_K_PIN) == LOW)
  {
    delay(5);
    if (digitalRead(EC11_K_PIN) == LOW)
    {
      if (bleKeyboard.isConnected())
      {
//        bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);
          bleKeyboard.write(48);
          time1=millis();
      }
    }
    while (digitalRead(EC11_K_PIN) == LOW)
      ;
  }

  LED_DISPLAY(Effet_type);
  if(millis()-lasttime[0]>=10){
   strip.show();
   lasttime[0]=millis();
  }
  if(millis()-lasttime[1]>=200){
   if(!(KEY1||KEY2||KEY3)){//change led effect
   Effet_type++;
  if(Effet_type>2) Effet_type=1;
   }
   lasttime[1]=millis();
  }  


}
