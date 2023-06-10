#define battery_min 3300     // минимальный уровень заряда батареи
#define sim800_Vcc 2           // пин питания, куда подключен sim800

//------БИБЛИОТЕКИ------
#include <SoftwareSerial.h>
SoftwareSerial ss(5, 6); // RX, TX
#include <DallasTemperature.h> // подключаем библиотеку чтения датчиков температуры
#include <Wire.h>              // вспомогательная библиотека датчика
#define ONE_WIRE_BUS 13 // и настраиваем  пин 13 как шину подключения датчиков DS18B20
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
#include <LowPower.h>          // библиотека сна

String _response = "";                      // Переменная для хранения ответа модуля
float k = 0.8;
float my_vcc_const = 1.080;   // константа вольтметра
float tempds0;

//------МОДУЛЬ ОБРАБОТКИ ОТВЕТА МОДЕМА------
String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  Serial.println(cmd);                          // Дублируем команду в монитор порта
  ss.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    Serial.println(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}
//------МОДУЛЬ ОБРАБОТКИ ОТВЕТА МОДЕМА------

String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!ss.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (ss.available()) {                     // Если есть, что считывать...
    _resp = ss.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    Serial.println("Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}
//------МОДУЛЬ ОБРАБОТКИ ОТВЕТА МОДЕМА------

void setup() {
  ss.begin(9600);
  pinMode(sim800_Vcc, OUTPUT);
  pinMode(A5, OUTPUT);
  pinMode(A4, OUTPUT);
  Serial.begin(9600);
  Serial.println("Starting SIM800 + narodmon"), delay(2000); 
}
  
void loop() {
  
    digitalWrite(A4, 1); // подать питание на датчик
    digitalWrite(A5, 0); // 
    delay(300);          // задержка для стабильности
    digitalWrite(sim800_Vcc, 1);      // подать питание на SIM800
    delay(3000);                      // задержка для стабильности
    int voltage = readVcc();         // считать напряжение питания
    sensors.requestTemperatures();   // читаем температуру с датчика
    tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20
    
    if (ss.available())   {                   // Если модем, что-то отправил...
    _response = waitResponse();                 // Получаем ответ от модема для анализа
    Serial.println(_response);
    
    sendATCommand("AT", true); //
    delay(2000);

    sendATCommand("ATE0", true); // отключаем режим ЭХА
    delay(2000);
    
    do {
    _response = sendATCommand("AT+CMEE=1", true);  // включаем расшифровку ошибок
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК

    do {
    _response = sendATCommand("AT+CGATT=1", true);  // Проверка доступа к услугам пакетной передачи данных
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК
        
    do {
    _response = sendATCommand("AT+CSTT=internet", true);  // Настройка точки доступа
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК
    
    do {
    _response = sendATCommand("AT+CIICR", true);  // Активация контекста
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК
                       
    sendATCommand("AT+CIFSR", true); //запрос IP
        
    do {
    _response = sendATCommand("AT+CIPSTART=TCP,narodmon.ru,8283", true);  // Открытие соединения с удаленным сервером
    if (_response.indexOf("OK") > -1) _response = "OK";  // 
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК
     
    _response = sendATCommand("AT+CIPSEND", true); // Проверка общего статуса
    if( _response.indexOf(">") > -1) {
       delay(300);
       tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20 
       ss.print("#D8-A0-2B-12-AF-3D#OpenWater"); // индивидуальный номер для народмона, это правило
       if (tempds0 > -40 && tempds0 < 54) ss.print("\n#Temp1#"), ss.print(tempds0);       
       ss.print("\n#Vbat#"),  ss.print(readVcc());         
       ss.println("\n##");      // обязательный параметр окончания пакета данных
       ss.println((char)26), delay (500);
       sendATCommand("AT+CIPSHUT",true); // закрываем пакет
       delay (500);
       digitalWrite(A4, 0); // отключить питание на датчик
       digitalWrite(A5, 0); // отключить питание на датчик
       delay(300);          // задержка для стабильности
       digitalWrite(sim800_Vcc, 0);      // отключить питание на SIM800
       delay(300);                       // задержка для стабильности
       sleep_30_min();
    } else ss.print("AT+CIPSHUT"), digitalWrite(sim800_Vcc, 0);      // отключить питание на SIM800
            
    if (readVcc() < battery_min) {
      digitalWrite(A4, 0); // отключить питание на датчик
      delay(300);          // задержка для стабильности
      digitalWrite(sim800_Vcc, 0);      // отключить питание на SIM800
      LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); // вечный сон если акум сел
    }
  }
}

long readVcc() { //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин)
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

void sleep_30_min(){ // ЦИКЛ ДЛЯ СНА 30 МИНУТ
//  Timer delays about 30 minutes between readings
    for (int i = 0; i < 220; i++) {   
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
    }
}
