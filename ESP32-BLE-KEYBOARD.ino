/**
 * This example turns the ESP32 into a Bluetooth LE keyboard that writes the words, presses Enter, presses a media key and then Ctrl+Alt+Delete
 */
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


void words_display()
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
  display.print(millis() / 1000);
  display.print(" s");

//  display.setCursor(0, 20);
//  display.print("Author: ");
//  display.print("Nick");
//  display.print("Author: ");
//  display.print("Nick");
//  
//  display.setCursor(0, 30);
//  display.print("Author: ");
//  display.print("Nick");
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
}

void loop() {
  if(bleKeyboard.isConnected()) {
    words_display();
    display.display();
  }
  else{
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
      }
      else
      {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
        display.setCursor(75, 20);
        display.print("↑");
        display.display();
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
//  if(bleKeyboard.isConnected()) {
//    words_display();
//    display.display();
//    
//    //if the button is pressed
//    if (digitalRead(KEY1) == LOW) {
//      //Send an ASCII '0',
//      
//      bleKeyboard.write(48);
//      delay(300);
//      Serial.println("0");
//      display.setCursor(0, 20);
//      display.print("key1");
//      display.display();
//    }
//   bleKeyboard.setDelay(10);//设置每个按键事件之间的延迟
//   if (digitalRead(KEY2) == LOW) {
//     //Send an ASCII '1',
//     bleKeyboard.write(49);
//     delay(300);
//     display.setCursor(25, 20);
//     display.print("key2");
//     display.display();
//   }
//   bleKeyboard.setDelay(10);//设置每个按键事件之间的延迟
//   if (digitalRead(KEY3) == LOW) {
//     //Send an ASCII '2',
//     bleKeyboard.write(50);
//     delay(300);
//     display.setCursor(50, 20);
//     display.print("key3");
//     display.display();
//   }


   //
   // Below is an example of pressing multiple keyboard modifiers 
   // which by default is commented out.
    /*
    Serial.println("Sending Ctrl+Alt+Delete...");
    bleKeyboard.press(KEY_LEFT_CTRL);
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.press(KEY_DELETE);
    delay(100);
    bleKeyboard.releaseAll();
    */
//  }
//  else{
//    //设置光标位置
//    // 清除屏幕
//  display.clearDisplay();
//
//  // 设置字体颜色,白色可见
//  display.setTextColor(WHITE);
//
//  //设置字体大小
//  display.setTextSize(1.5);
//  
//  display.setCursor(0, 0);
//  display.print("Bluetooth not connected");
//  display.display();
//  }

}
