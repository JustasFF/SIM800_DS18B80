/*
 * 1. Настройка датчика на 12 бит //sensors.setResolution(insideThermometer, 12);
 * 2. Настрока SIM800:
 * 2.1. Передача времение. AT+CLTS=1;&W /-> AT+CFUN=1,1 
 * 2.2. Отмена повторения кода запроса
 * 2.3. Ответ буквами
 * 2.4. ****** что-то еще
 * 2.5. Сохранить в память

Open / Air / Under / Water, 3 simbol # - Cube/Hexagon/Stivenson/Triangle

*/

//#99-20-36-79-80-41#OW_Hexagon_1 (79601251768)
//#99-20-AD-97-08-14#OW_Hexagon_2
//#D8-A0-2B-12-AF-3D#OWT_1 (79623320270) скошенная коробка с сьемными боковыми панелями 
//#D8-00-2B-12-AF-32#OAS_1 (79304045460) экран Стивенсона - окружная, действующий - годовой абонемент



#include "MemoryFree.h"
#include <Uptime.h>                                          // Library To Calculate Uptime
#include <GyverFilters.h>
#include "LED.h"
#include <EEPROM.h>
#include <GyverPower.h>
#include <TimeLib.h>
#include <powerConstants.h>

#define battery_min 3450 // минимальный уровень заряда батареи
#define sim800_Vcc 12    // пин питания, куда подключен sim800
#define sim800_pow 9    // пин питания, куда подключен sim800
#define ONE_WIRE_BUS 18 //и настраиваем  пин A4 как шину подключения датчиков DS18B20
#define pinGreen 10      //  светодиод ОК пин 10
#define pinRed 11        //  светодиод Error пин 11
#define ds_plus A5 //  питание + датчика DS18B20 пин A5
#define ds_minus A3 //  питание - датчика DS18B20 пин A3
#define name_mac "#99-20-AD-97-99-99#UWH_1" //"#D8-A0-2B-12-AF-3D#OpenWater_1" // #99-20-AD-97-08-14#OA_Cube_3 (79623320270=OpenWater_1)#D8-00-2B-12-AF-32#OA_Cube_1 // #99-20-AD-97-08-14#UW_Hehagon_2#D8-A0-2B-12-AF-3D#OpenWater_1
#define tel "+79202281910"

LED_Y green_led(pinGreen, 100, 50);
LED_Y red_led(pinRed, 100, 100);
GMedian<12, int16_t> av_v;

#include <SoftwareSerial.h>
SoftwareSerial SIM800(4, 3); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#include <Wire.h>              // вспомогательная библиотека датчика
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Uptime uptime; 

//=====================================================
#define DEBUG                                     // Для режима отладки нужно раскомментировать эту строку

#ifdef DEBUG                                        // В случае существования DEBUG
#define DEBUG_PRINT(x)       Serial.print (x)       // Создаем "переадресацию" на стандартную функцию
#define DEBUG_PRINTLN(x)     Serial.println (x)
#define DEBUG_print_eeprom()  print_eeprom()
#else                                               // Если DEBUG не существует - игнорируем упоминания функций
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_print_eeprom()
#endif
//=====================================================

String at = "";
uint16_t address = 0;
float tempds0; // переменная для температуры
unsigned long Time1; //таймер для функции detection ()
unsigned long Time2; //таймер для функции detection ()
unsigned long m;
byte modem = 0;            // переменная состояние модема
int8_t _sig;
uint16_t varS = 1; //переменная счетчика отправок
uint16_t v; //переменная для хранения вольтажа
uint16_t fm; //переменная для freememory
int count_send = 0;

struct myStruct {
  int32_t temp_e;
  unsigned long unix_time;
  };
myStruct temp_data;
//myStruct data_eeprom;



void setup() {
  pinMode(sim800_pow, OUTPUT);
  Serial.begin(115200);  //скорость порта
  SIM800.begin(9600);  //скорость связи с модемом
  pinMode(sim800_Vcc, OUTPUT);
  pinMode(ds_plus, OUTPUT);
  pinMode(ds_minus, OUTPUT);
  digitalWrite(sim800_Vcc, 1);      // подать питание на SIM800
  digitalWrite(sim800_pow, 1);      // подать питание на SIM800
  power.sleepDelay(1400);
  digitalWrite(sim800_pow, 0);      // подать питание на SIM800
  green_led.blink_n(1), red_led.blink_n(1);
  Serial.println(name_mac);
  delay(10);
  digitalWrite(ds_plus, 1); // подать питание на датчик
  digitalWrite(ds_minus, 0); //
  sensors.begin();
  //sensors.setResolution(insideThermometer, 12);
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();   // читаем температуру с датчика
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20  
  power.calibrate(power.getMaxTimeout());
  
  power.sleepDelay(10000);
  do {
    at = sendATCommand("AT", true);  //
    at.trim();                       // Убираем пробельные символы в начале и конце
  } while (at != "OK");
  Time1 = millis();
  Time2 = millis();
  null_eeprom();
  v = readVcc();
  }

void loop()
  
  {
  uptime.calculateUptime();
  byte k = 0;
  if (SIM800.available()) { // если что-то пришло от SIM800 в направлении Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k), delay(1); //пришедшую команду набиваем в переменную at

    if (at.indexOf("SHUT OK") > -1 && modem == 2) {
      DEBUG_PRINTLN(F("S H U T OK"));
      delay(50);
      SIM800.println(F("AT+CREG?"));
      modem = 3;

    } else if (at.indexOf("0,1") > -1 && modem == 3) {
      DEBUG_PRINTLN(F("AT+CREG OK"));
      delay(50);
      SIM800.println(F("AT+CGATT=1"));
      modem = 4;

    } else if (at.indexOf("OK") > -1 && modem == 4) {
      DEBUG_PRINTLN(F("AT+CGATT=1 OK"));
      delay(50);
      SIM800.println(F("AT+CSTT=\"internet.ru\""));
      modem = 5;

    } else if (at.indexOf("OK") > -1 && modem == 5) {
      DEBUG_PRINTLN(F("internet.ru OK"));
      delay(50);
      SIM800.println(F("AT+CIICR"));
      modem = 6;

    } else if (at.indexOf("OK") > -1 && modem == 6) {
      DEBUG_PRINTLN(F("AT+CIICR OK"));
      delay(50);
      SIM800.println(F("AT+CIFSR"));
      modem = 7;

    } else if (at.indexOf(".") > -1 && modem == 7) {
      SIM800.println(F("AT+CIPSTART=\"TCP\",\"narodmon.ru\",\"8283\""));
      
      modem = 8;

    } else if (at.indexOf("CONNECT OK") > -1 && modem == 8 ) {
      DEBUG_PRINTLN(F("C O N N E C T OK"));
      delay(50);
      SIM800.println(F("AT+CIPSEND"));
      modem = 9;

    } else if (at.indexOf(">") > -1 && modem == 9) {  // "набиваем" пакет данными и шлем на сервер
      delay (50);
      SIM800.print(name_mac);
      send_eeprom();
      uptime.calculateUptime();
      String ssend = "";
      if  (85 > tempds0 && tempds0 > -50) {
        ssend +=(F("\n#Temp1#"));
        ssend +=(tempds0);
      }
      ssend +=(F("\n#Vbat#"));
      ssend +=(av_v.filtered(v));
      if  (_sig > -114 && _sig < -46) {
        ssend += (F("\n#dBm#"));
        ssend += (_sig);
      }
      ssend +=(F("\n#Period#"));
      m = (millis() - Time2);
      ssend +=(m);
      ssend +=(F("\n#Count#"));
      ssend +=(varS);
      ssend +=(F("\n#Uptime#"));
      ssend +=(uptime.getTotalSeconds());
      ssend +=(F("\n##")); // обязательный параметр окончания пакета данных
      ssend +=((char)26);
      SIM800.print(ssend);     
      modem = 10;

    } else if (at.indexOf("OK") > -1 && modem == 10) {
      SIM800.print(F("AT+CIPSHUT")); // закрываем пакет
      {
      String pprt = "";
      Serial.println(name_mac); //name_mac
      DEBUG_print_eeprom();
      pprt += (F("\n#Temp1#"));
      pprt += (tempds0);
      pprt += (F("\n#Vbat#"));
      pprt += (v);
      if  (_sig > -114 && _sig < -46) {
        pprt += (F("\n#dBm#"));
        pprt += (_sig);
      }
      pprt += (F("\n#Count#"));
      Serial.print(pprt);
      Serial.println(varS);
      }
      
      Time2 = millis();
      digitalWrite(ds_plus, 0);
      digitalWrite(sim800_Vcc, 0); // sim800 включить сон и через 7 сек сам заснет // ;     // отключить питание на SIM800 // sendATCommand("AT+CSCLK=2",true);
      green_led.blink_n(3);
      null_eeprom(), address = 0;
      count_send = 0;
      digitalClockDisplay();
      delay(50);
      Serial.println(F("SEND OK"));
      delay(50);
      modem = 0;
      SIM800.end();
      delay(50);
      slee_P();
      SIM800.begin(9600);
      delay(50);
      digitalWrite(ds_plus, 1);
      digitalWrite(ds_minus, 0); // включить питание на датчик
      digitalWrite(sim800_Vcc, 1);//
      digitalWrite(sim800_pow, 1);      // подать питание на SIM800
      power.sleepDelay(1400);
      digitalWrite(sim800_pow, 0);      // подать питание на SIM800 
      power.sleepDelay(5000);
      Time1 = millis();
      varS++;
      sendATCommand("AT",true); //      

    } else if (count_send >= 5 && modem > 1 && modem < 10){
      EEPROM.put(address, temp_data);
      red_led.blink_n(5);
      address += sizeof(temp_data); // наращиваем адрес
      //-*-------------------------------------------------//
      digitalClockDisplay();
      delay(50);
      Serial.println(F("ERROR SEND"));
      Serial.print(F("at="));     // если пришло что-то другое выводим в серийный порт
      Serial.println(at);     // если пришло что-то другое выводим в серийный порт
      modem = 0;
      sendATCommand("AT+CIPSHUT",true); // закрываем пакет
      digitalWrite(ds_plus, 0);
      delay(50);
      digitalWrite(sim800_Vcc, 0); // sim800 включить сон и через 7 сек сам заснет // ;     // отключить питание на SIM800 // sendATCommand("AT+CSCLK=2",true);
      delay(50);
      slee_P();
      digitalWrite(ds_plus, 1);
      digitalWrite(ds_minus, 0); // включить питание на датчик
      digitalWrite(sim800_Vcc, 1);//
      digitalWrite(sim800_pow, 1);
      power.sleepDelay(1400);
      digitalWrite(sim800_pow, 1); 
      delay(5000);
      Time1 = millis();
      count_send = 0;
      sendATCommand("AT",true); //      
      
     } else {
      DEBUG_PRINT(F("count_send="));
      DEBUG_PRINT(count_send);
      delay(50);
      DEBUG_PRINT(F(" "));
      delay(50);
      DEBUG_PRINT(F("modem="));     // если пришло что-то другое выводим в серийный порт
      DEBUG_PRINT(modem);     // если пришло что-то другое выводим в серийный порт
      delay(50);
      DEBUG_PRINTLN(F(" "));
      delay(50);
      DEBUG_PRINT(F("at="));     // если пришло что-то другое выводим в серийный порт
      DEBUG_PRINT(at);     // если пришло что-то другое выводим в серийный порт
      count_send ++;
      }
      //at = ""; //clear_string(at);
      clear_string(at);
      }

  if (millis() - Time1 > 12000){
    detection();
    Time1 = millis(); // выполняем функцию detection () через интервал (interval)
    }
  if (v < battery_min) {
    digitalClockDisplay();
    delay(50);
    sendSMS(tel, name_mac);
    delay(500);
    digitalWrite(sim800_Vcc, 0);
    digitalWrite(ds_plus, 0);
    power.hardwareDisable(PWR_ALL);
    power.sleep(SLEEP_FOREVER);//LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // отключить питание на датчик
  }
}
