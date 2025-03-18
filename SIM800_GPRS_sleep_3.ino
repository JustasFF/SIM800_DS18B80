//#include <EEPROM.h>
#include <GyverPower.h>
#include <powerConstants.h>

#define battery_min 3600 // минимальный уровень заряда батареи
#define sim800_Vcc 12    // пин питания, куда подключен sim800
#define ONE_WIRE_BUS 18 //и настраиваем  пин A4 как шину подключения датчиков DS18B20
#define pinGreen 10      //  светодиод ОК пин 10
#define pinRed 11        //  светодиод Error пин 11
#define ds_plus A5 //  питание + датчика DS18B20 пин A5
#define ds_minus A3 //  питание - датчика DS18B20 пин A3

#include <SoftwareSerial.h>
SoftwareSerial SIM800(4, 3); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#include <Wire.h>              // вспомогательная библиотека датчика
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//=====================================================
#define DEBUG                                     // Для режима отладки нужно раскомментировать эту строку

#ifdef DEBUG                                        // В случае существования DEBUG
#define DEBUG_PRINT(x)       Serial.print (x)       // Создаем "переадресацию" на стандартную функцию
#define DEBUG_PRINTLN(x)     Serial.println (x)
#else                                               // Если DEBUG не существует - игнорируем упоминания функций
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif
//=====================================================


float tempds0; // переменная для температуры
unsigned long Time1 = 0;
unsigned long Time2 = 0;
byte modem = 0;            // переменная состояние модема
int8_t _sig = -99;
static int16_t varS; //переменная счетчика отправок
static int16_t v; //переменная для хранения вольтажа
String name_mac = "#D8-00-2B-12-AF-32#External_12AF322"; // индивидуальный номер для народмона, это правило

void setup() {
  pinMode(pinRed, OUTPUT);
  pinMode(pinGreen, OUTPUT);
  power.autoCalibrate();
  String at = "";
  Serial.begin(9600);  //скорость порта
  SIM800.begin(9600);  //скорость связи с модемом
  pinMode(sim800_Vcc, OUTPUT);
  pinMode(ds_plus, OUTPUT);
  pinMode(ds_minus, OUTPUT);
  blinkLed(pinRed, 500);
  digitalWrite(sim800_Vcc, 1);      // подать питание на SIM800
  Serial.println(F("START"));
  Serial.println((name_mac));
  Serial.println(F("battery_min 3600")), Serial.println(F("ONE_WIRE_BUS 18")), Serial.println(F("pinGreen 10")), Serial.println(F("pinRed 11")), Serial.println(F("ds_plus A5")), Serial.println(("ds_minus A3")); //
  digitalWrite(ds_plus, 1); // подать питание на датчик
  digitalWrite(ds_minus, 0); //
  blinkLed(pinGreen, 500);
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();   // читаем температуру с датчика
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20
  power.sleepDelay(4000);
  do {
    at = sendATCommand("ATE0", true);  //
    at.trim();                       // Убираем пробельные символы в начале и конце
  } while (at != "OK");
  at = "";
  sendATCommand("AT+CFUN=1,1", true);  // 
  sendATCommand("AT+CSCLK=0", true); //sim800 запрет сна
  //sendATCommand("AT+CPIN=1057", true); //  
}

void loop() {
  String at = "";
  byte k = 0;
  if (SIM800.available()) { // если что-то пришло от SIM800 в направлении Ардуино
    while (SIM800.available()) k = SIM800.read(), at += char(k), delay(1); //пришедшую команду набиваем в переменную at

    if (at.indexOf("SHUT OK") > -1 && modem == 2) {
      DEBUG_PRINTLN(F("S H U T OK"));
      SIM800.println("AT+CMEE=1");
      delay(100);
      modem = 3;

    } else if (at.indexOf("OK") > -1 && modem == 3) {
      DEBUG_PRINTLN(F("AT+CMEE=1 OK"));
      SIM800.println("AT+CGATT=1");
      delay(100);
      modem = 4;

    } else if (at.indexOf("OK") > -1 && modem == 4) {
      DEBUG_PRINTLN(F("AT+CGATT=1 OK"));
      SIM800.println("AT+CSTT=\"internet.ru\"");
      delay(100);
      modem = 5;

    } else if (at.indexOf("OK") > -1 && modem == 5) {
      DEBUG_PRINTLN(F("internet.ru OK"));
      SIM800.println("AT+CIICR");
      delay(100);
      modem = 6;

    } else if (at.indexOf("OK") > -1 && modem == 6) {
      DEBUG_PRINTLN(F("AT+CIICR OK"));
      SIM800.println("AT+CIFSR");
      delay(100);
      modem = 7;

    } else if (at.indexOf(".") > -1 && modem == 7) {
      DEBUG_PRINTLN(at);
      SIM800.println("AT+CIPSTART=\"TCP\",\"narodmon.ru\",\"8283\"");
      delay(100);
      modem = 8;

    } else if (at.indexOf("CONNECT OK") > -1 && modem == 8 ) {
      SIM800.println("AT+CIPSEND");
      DEBUG_PRINTLN(F("C O N N E C T OK"));
      delay(100);
      modem = 9;

    } else if (at.indexOf(">") > -1 && modem == 9) {  // "набиваем" пакет данными и шлем на сервер
      DEBUG_PRINTLN(name_mac); 
      DEBUG_PRINT(F("\n#Temp1#"));
      DEBUG_PRINTLN(tempds0);
      DEBUG_PRINT(F("\n#Vbat#"));
      DEBUG_PRINTLN(v);
      if  (_sig > -114 && _sig < -46) {
        DEBUG_PRINT(F("\n#dBm#"));
        DEBUG_PRINTLN(_sig);
      }
      DEBUG_PRINT(F("\n#Count#"));
      DEBUG_PRINTLN(varS);
      delay (50);
      SIM800.print(name_mac); // индивидуальный номер для народмона, это правило
      if  (85 > tempds0 && tempds0 > -50) {
        SIM800.print("\n#Temp1#");
        SIM800.print(tempds0);
      }
      SIM800.print("\n#Vbat#");
      SIM800.print(v);
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
      DEBUG_PRINTLN(F("SEND OK"));
      modem = 0;
      delay(150);
      sendATCommand("AT+CIPSHUT",true); // закрываем пакет
      delay(150);
      digitalWrite(ds_plus, 0);
      digitalWrite(sim800_Vcc, 0); // sim800 включить сон и через 7 сек сам заснет // ;     // отключить питание на SIM800 // sendATCommand("AT+CSCLK=2",true);
      blinkOK();
      SIM800.end();
      delay(50);
      Time2 = millis();
      slee_P();
      SIM800.begin(9600);
      delay(50);
      digitalWrite(ds_plus, 1);
      digitalWrite(ds_minus, 0); // включить питание на датчик
      delay(150);
      digitalWrite(sim800_Vcc, 1);// SIM800.println("AT"); // //sim800 будим
      delay(2000);
      varS = varS + 1;
      sendATCommand("AT",true); //sim800 будим
      //delay(150);
      //sendATCommand("AT+CSCLK=0", true); //sim800 запрет сна
      delay(150);
      sendATCommand("AT+CIPSHUT",true); //

    } else DEBUG_PRINTLN(at);    // если пришло что-то другое выводим в серийный порт
    at = "";  // очищаем переменную
  }

  /*if (Serial.available()) {             //если в мониторе порта ввели что-то
    while (Serial.available()) k = Serial.read(), at += char(k), delay(1);
    SIM800.println(at),at = "";  //очищаем
                        }*/

  if (millis() > Time1 + 7500) {
    detection();
    Time1 = millis(); // выполняем функцию detection () через интервал (interval)
  }
    
  if (v < battery_min) {
    digitalWrite(sim800_Vcc, 0);
    digitalWrite(ds_plus, 0);
    power.hardwareDisable(PWR_ALL);
    power.sleep(SLEEP_FOREVER);//LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // отключить питание на датчик
  }

}

void detection() { // условия проверяемые каждые  сек
  static byte interval;
  sensors.requestTemperatures();   // читаем температуру с датчика
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20
  if (modem <= 2) {
    _sig = Signal();
    v = readVcc();
    DEBUG_PRINT(F(" || Signal : "));
    DEBUG_PRINT(_sig);
    DEBUG_PRINT(F(" || voltage : "));
    DEBUG_PRINT(v);
    DEBUG_PRINT(F(" || "));
  }
  DEBUG_PRINT(F("Modem : ")); DEBUG_PRINT(modem);
  DEBUG_PRINT(F(" || Interval : ")); DEBUG_PRINT(interval);
  DEBUG_PRINT(F(" || Temp : ")); DEBUG_PRINTLN(tempds0);  
  interval--;
  if (interval == 253 ) {
    interval = 0;
    modem = 1; //
  }
  if (modem == 1) {
    SIM800.println("AT+CIPSHUT");
    modem = 2;
  }  
}

void slee_P() { // ЦИКЛ ДЛЯ СНА 15 МИНУТ // 18 циклов нужно
  int16_t n;
  if (v > 4050) {
    n = 44; //15 minutes
  } else if (v <= 4050 && v > 3900) {
    n = 89;//30 minutes
  } else if (v <= 3900 && v > 3750) {
    n = 119;//45 minutes
  } else if (v <= 3750 && v > 3610) {
    n = 179; //59 minutes
  }
  else n = 180;
  delay(50);
  power.hardwareDisable(PWR_ADC); //(PWR_ADC | PWR_UART0)
  //  Timer delays about few minutes between readings
  for (int i = 0; i < n; i++) {
    power.sleepDelay(20*1000ul);//LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  power.hardwareEnable(PWR_ADC);
  //LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  //LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
  //LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
}

int Signal() { // замеряем dBa
  String _signal;
  do {
    _signal = sendATCommand("AT+CSQ", true);    // Запрашиваем информацию о качестве сигнала
  } while (_signal.indexOf("CSQ:") < 0);
  _signal = _signal.substring(8, 10);
  int8_t _signall = _signal.toInt();
  return (_signall + _signall - 114);
}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  DEBUG_PRINTLN(cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    DEBUG_PRINTLN(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  unsigned long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если не пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    blinkFail();
    delay(50);
    digitalWrite(sim800_Vcc, 0);      // - питание на SIM800
    power.sleepDelay(2000);
    digitalWrite(sim800_Vcc, 1);      //  + питание на SIM800
    power.sleepDelay(4000);
    modem = 0;
    digitalWrite(ds_plus, 1); // подать питание на датчик
    SIM800.println("AT+CIPSHUT");
    _resp = "REBOOT";
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

void blinkOK() {                        // Функция индикации OK (3 мигания зеленым индикатором)
  for (int i = 0; i < 3; i++) {
    blinkLed(pinGreen, 300);
    delay(100);
  }
}

void blinkFail() {                      // Функция индикации ошибки (длительное мигание красным)
  blinkLed(pinRed, 1500);
}

void blinkLed(int pin, int _delay) {    // Общая функция для мигания светодиодами

  digitalWrite(pin, HIGH);
  if (_delay > 0) delay(_delay);
  digitalWrite(pin, LOW);
}

long readVcc() { //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин) vbyen
float my_vcc_const = 1.097;   // константа вольтметра
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both
  long result = (high << 8) | low;

  result = my_vcc_const * 1023 * 1000 / result; // расчёт реального VCC
  return result; // возвращает VCC
}
