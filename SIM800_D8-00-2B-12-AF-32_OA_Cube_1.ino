#include <GyverFilters.h>
#include "LED.h"
#include <EEPROM.h>
#include <GyverPower.h>
#include <TimeLib.h>
#include <powerConstants.h>

#define battery_min 3400 // минимальный уровень заряда батареи
#define sim800_Vcc 12    // пин питания, куда подключен sim800
#define sim800_pow 9    // пин питания, куда подключен sim800
#define ONE_WIRE_BUS 18 //и настраиваем  пин A4 как шину подключения датчиков DS18B20
#define pinGreen 10      //  светодиод ОК пин 10
#define pinRed 11        //  светодиод Error пин 11
#define ds_plus A5 //  питание + датчика DS18B20 пин A5
#define ds_minus A3 //  питание - датчика DS18B20 пин A3
#define name_mac "#D8-00-2B-12-AF-32#OA_Cube#1" // индивидуальный номер для народмона, это правило 
LED green_led(pinGreen, 100, 100);
LED red_led(pinRed, 100, 50);
GMedian<12, int16_t> av_v;

#include <SoftwareSerial.h>
SoftwareSerial SIM800(4, 3); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#include <Wire.h>              // вспомогательная библиотека датчика
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


//=====================================================
//#define DEBUG                                     // Для режима отладки нужно раскомментировать эту строку

#ifdef DEBUG                                        // В случае существования DEBUG
#define DEBUG_PRINT(x)       Serial.print (x)       // Создаем "переадресацию" на стандартную функцию
#define DEBUG_PRINTLN(x)     Serial.println (x)
#else                                               // Если DEBUG не существует - игнорируем упоминания функций
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
//=====================================================

String at = "";
long m;
uint16_t address = 0;
float tempds0; // переменная для температуры
unsigned long Time1 = 0; //таймер для функции detection ()
unsigned long Time2 = 0;
byte modem = 0;            // переменная состояние модема
int8_t _sig = -99;
uint16_t varS; //переменная счетчика отправок
int16_t v; //переменная для хранения вольтажа

struct myStruct {
  int32_t temp_e;
  unsigned long unix_time;
  };
myStruct temp_data;
//myStruct data_eeprom;
int count_send = 0;


void setup() {
  pinMode(sim800_pow, OUTPUT);
  String at = "";
  Serial.begin(9600);  //скорость порта
  SIM800.begin(9600);  //скорость связи с модемом
  pinMode(sim800_Vcc, OUTPUT);
  pinMode(ds_plus, OUTPUT);
  pinMode(ds_minus, OUTPUT);
  digitalWrite(sim800_Vcc, 1);      // подать питание на SIM800
  digitalWrite(sim800_pow, 1);      // подать питание на SIM800
  power.sleepDelay(1400);
  digitalWrite(sim800_pow, 0);      // подать питание на SIM800
  green_led.blink_n(1);
  red_led.blink_n(1);
  Serial.println(name_mac);
  digitalWrite(ds_plus, 1); // подать питание на датчик
  digitalWrite(ds_minus, 0); //
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();   // читаем температуру с датчика
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20  
  power.calibrate(power.getMaxTimeout());
  power.sleepDelay(7000);
  sendATCommand("ATE0V1+CMEE=1;&W", true);  // ATE0V0+CMEE=1;&W
  delay(100);
  do {
    at = sendATCommand("AT", true);  //
    at.trim();                       // Убираем пробельные символы в начале и конце
  } while (at != "OK");
  at = "";
  if (now()< 1704067201) set_data_time();
  //sendATCommand("AT+CSCLK=0", true); //sim800 запрет сна
  
}

void loop() {
  
  byte k = 0;
  if (SIM800.available()) { // если что-то пришло от SIM800 в направлении Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k), delay(1); //пришедшую команду набиваем в переменную at

    if (at.indexOf("SHUT OK") > -1 && modem == 2) {
      DEBUG_PRINTLN(F("S H U T OK"));
      at = "";
      SIM800.println("AT+CREG?"); // Запрос текущего режима и статуса
      delay(100);
      modem = 3;

    } else if (at.indexOf("0,1") > -1 && modem == 3) {
      DEBUG_PRINTLN(F("AT+CREG OK"));
      SIM800.println("AT+CGATT=1"); //Подключение модуля к GPRS
      delay(100);
      modem = 4;

    } else if (at.indexOf("OK") > -1 && modem == 4) {
      DEBUG_PRINTLN(F("AT+CGATT=1 OK"));
      at = "";
      SIM800.println("AT+CSTT=\"internet.ru\""); // APN — Access Point Name, имя точки доступа для GPRS
      delay(100);
      modem = 5;

    } else if (at.indexOf("OK") > -1 && modem == 5) {
      Serial.println(F("internet.ru OK"));
      at = "";
      SIM800.println("AT+CIICR"); // Устанавливает беспроводное подключение GPRS. Может занять некоторое время, так что ее нужно выполнять в цикле и проверять ответ.
      delay(100);
      modem = 6;

    } else if (at.indexOf("OK") > -1 && modem == 6) {
      DEBUG_PRINTLN(F("AT+CIICR OK"));
      at = "";
      SIM800.println("AT+CIFSR"); // Возвращает IP-адрес модуля. Чтобы проверить, подключен ли модуль к интернету.
      delay(100);
      modem = 7;

    } else if (at.indexOf(".") > -1 && modem == 7) {
      Serial.println(at);
      SIM800.println("AT+CIPSTART=\"TCP\",\"narodmon.ru\",\"8283\""); // Создание подключения к народмон
      delay(100);
      modem = 8;

    } else if (at.indexOf("CONNECT OK") > -1 && modem == 8 ) {
      SIM800.println("AT+CIPSEND");
      Serial.println(F("C O N N E C T OK"));
      delay(100);
      modem = 9;

    } else if (at.indexOf(">") > -1 && modem == 9) {  // "набиваем" пакет данными и шлем на сервер
      Serial.println(name_mac); 
      print_eeprom();
      Serial.print(F("#Temp1#"));
      Serial.print(tempds0);
      Serial.print(F("\n#Vbat#"));
      Serial.print(v);
      if  (_sig > -114 && _sig < -46) {
        Serial.print(F("\n#dBm#"));
        Serial.print(_sig);
      }
      Serial.print(F("\n#Count#"));
      Serial.println(varS);
      delay (50);
      SIM800.print(name_mac); // индивидуальный номер для народмона, это правило
      send_eeprom();
      if  (85 > tempds0 && tempds0 > -50) {
        SIM800.print("\n#Temp1#");
        SIM800.print(tempds0);
      }
      SIM800.print("\n#LAT#51.751700"); // 51.751700 
      SIM800.print("\n#LON#39.303800"); // 39.303800
      SIM800.print("\n#Vbat#");
      SIM800.print(av_v.filtered(v));
      if  (_sig > -114 && _sig < -46) {
        SIM800.print("\n#dBm#");
        SIM800.print(_sig);
      }
      SIM800.print("\n#Period#");
      SIM800.print(millis() - Time2);
      SIM800.print("\n#Count#");
      SIM800.print(varS);
      SIM800.print("\n##");      // обязательный параметр окончания пакета данных
      SIM800.print((char)26);
      modem = 10;

    } else if (at.indexOf("OK") > -1 && modem == 10) {
      sendATCommand("AT+CIPSHUT",true); // закрываем пакет
      delay(150);
      digitalWrite(ds_plus, 0);
      digitalWrite(sim800_Vcc, 0); // sim800 включить сон и через 7 сек сам заснет // ;     // отключить питание на SIM800 // sendATCommand("AT+CSCLK=2",true);
      green_led.blink_n(3);
      null_eeprom(), address = 0;
      count_send = 0;
      Serial.println(F("SEND OK"));
      modem = 0;
      delay(150);
      SIM800.end();
      delay(50);
      m = millis()-Time2;
      slee_P();
      Time2 = millis();
      SIM800.begin(9600);
      delay(50);
      digitalWrite(ds_plus, 1);
      digitalWrite(ds_minus, 0); // включить питание на датчик
      digitalWrite(sim800_Vcc, 1);//
      digitalWrite(sim800_pow, 1);      // подать питание на SIM800
      power.sleepDelay(1400);
      digitalWrite(sim800_pow, 0);      // подать питание на SIM800 
      power.sleepDelay(5000);
      varS ++;
      sendATCommand("AT",true); //
      delay(150);
      sendATCommand("AT+CIPSHUT",true); //

    } else if (count_send > 5 && modem > 1 && modem < 9){
      EEPROM.put(address, temp_data);
      red_led.blink_n(5);
      address += sizeof(temp_data); // наращиваем адрес
      //-*-------------------------------------------------//
      Serial.println(F("ERROR SEND"));
      modem = 0;
      delay(50);
      SIM800.print("AT+CIPSHUT"); // закрываем пакет
      delay(150);
      digitalWrite(ds_plus, 0);
      digitalWrite(sim800_Vcc, 0); // sim800 включить сон и через 7 сек сам заснет // ;     // отключить питание на SIM800 // sendATCommand("AT+CSCLK=2",true);
      count_send = 0;
      at = "";
      delay(50);
      slee_P();
      digitalWrite(ds_plus, 1);
      digitalWrite(ds_minus, 0); // включить питание на датчик
      power.sleepDelay(150);
      digitalWrite(sim800_Vcc, 1);// 
      power.sleepDelay(5000);
      sendATCommand("AT",true); //
      delay(150);
      sendATCommand("AT+CIPSHUT",true); //      
      
     } else {
      //Serial.print(count_send);
      //delay(50);
      //Serial.print(" ");
      //delay(50);
      //Serial.print(modem);     // если пришло что-то другое выводим в серийный порт
      //delay(50);
      //Serial.println(" ");
      //delay(50);
      //Serial.print(at);     // если пришло что-то другое выводим в серийный порт
      count_send++;
      at = "";  // очищаем переменную
     }
}

  if (millis() > Time1 + 6767){
    detection();
    Time1 = millis(); // выполняем функцию detection () через интервал (interval)
  }
  
  /*if ((now() > Time3 + 3530) && (modem <= 2)){
    set_data_time();
    Time3 = now(); // выполняем функцию detection () через интервал (interval)
  }
  */
  if (v < battery_min) {
    digitalWrite(sim800_Vcc, 0);
    digitalWrite(ds_plus, 0);
    power.hardwareDisable(PWR_ALL);
    power.sleep(SLEEP_FOREVER);//LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // отключить питание на датчик
  }
}
