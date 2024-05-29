void detection() {                                                      // условия проверяемые каждые  сек
  static byte interval;
  sensors.requestTemperatures();   // читаем температуру с датчика
  delay(800);
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20
  delay(50);
  temp_data.temp_e = tempds0 * 1000;
  temp_data.unix_time = now() - 10800;
  delay(50);
  green_led.blink_n(1);
  delay(50);
  if (modem < 2) {
    _sig = Signal();
    delay(100);
    set_data_time();
    delay(100);
    v = readVcc();
    Serial.print(F("|| Signal : "));
    Serial.print(_sig);
    Serial.print(F(" || voltage : "));
    Serial.print(v);
    DEBUG_PRINT(F(" || "));
  }
  DEBUG_PRINT(F("Modem : ")); DEBUG_PRINT(modem);
  DEBUG_PRINT(F(" || Interval : ")); DEBUG_PRINT(interval);
  Serial.print(F(" || Temp : ")); Serial.print(tempds0);
  Serial.print(F(" || Time : ")); digitalClockDisplay();
  interval--;
  if (interval == 253 ) { //1577836801 нужна проверка на дату
    interval = 0;
    modem = 1; //
  }
  if (modem == 1) {
    SIM800.println("AT+CIPSHUT");
    modem = 2;
  }
}

void slee_P() {                                                         // ЦИКЛ ДЛЯ СНА
  long n;
  if (v > 4100) {
    n = 282500; //5 minutes
  } else if (v <= 4100 && v > 3900) {
    n = 599000;//10 minutes
  } else if (v <= 3900 && v > 3700) {
    n = 850000;//15 minutes
  }
  else n = 1680000;
  delay(50);
  power.hardwareDisable(PWR_ADC); //(PWR_ADC | PWR_UART0)
  //  Timer delays about few minutes between readings
  power.sleepDelay(n - m);//power.sleep(SLEEP_8192MS);
  //power.sleepDelay(m);
  power.hardwareEnable(PWR_ADC);
  Time2 = millis();
}

int Signal() {                                                          // замеряем dBa
  String _signal;
  do {
    _signal = sendATCommand("AT+CSQ", true);    // Запрашиваем информацию о качестве сигнала
  } while (_signal.indexOf("CSQ:") < 0);
  delay(50);
  _signal = _signal.substring(8, 10);
  delay(50);
  int8_t _signall = _signal.toInt();
  delay(50);
  return (_signall + _signall - 114);
}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  DEBUG_PRINTLN(cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();    // ... ждем, когда будет передан ответ
  DEBUG_PRINTLN(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

String waitResponse() {                                                 // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                                                    // Переменная для хранения результата
  unsigned long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если не пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    red_led.blink_n(3);
    delay(50);
    digitalWrite(sim800_Vcc, 0);      // - питание на SIM800
    delay(2000);
    digitalWrite(sim800_Vcc, 1);      //  + питание на SIM800
    digitalWrite(sim800_pow, 1);      // подать питание на SIM800
    delay(1400);
    digitalWrite(sim800_pow, 0);      // подать питание на SIM800
    power.sleepDelay(7000);
    modem = 0;
    digitalWrite(ds_plus, 1); // подать питание на датчик
    SIM800.println("AT+CIPSHUT");
    _resp = "REBOOT";
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

/*void blinkOK() {                                                       // Функция индикации OK (3 мигания зеленым индикатором)
  for (int i = 0; i < 3; i++) {
    blinkLed(10, 50);
  }
}

void blinkLed(int pin, int _delay) {                                   // Общая функция для мигания светодиодами

  digitalWrite(pin, HIGH);
  if (_delay > 0) delay(_delay);
  digitalWrite(pin, LOW);
  delay(_delay);
}*/

long readVcc() {                                                       //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин) vbyen
  float my_vcc_const = 1.098;   // константа вольтметра
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

void null_eeprom() {
  int value = 255;
  for (int i = 0; i < EEPROM.length(); i++) { // Обнуляем EEPROM - приводим в первоначальное состояние
    EEPROM.update(i, value);
  }
}

void print_eeprom() {
  myStruct data_eeprom;
  uint16_t add_ress = 0;
  EEPROM.get(0, data_eeprom);
  if (data_eeprom.temp_e != -1) {                                            // проверка на наличие данных в 0 адресе EEPROM

    //Serial.print(name_mac + "\n");                                           // Выводим полученное значение в монитор
    //address = 0;                                                             // обнуляем адрес EEPROM
    while (add_ress < EEPROM.length()) {                                     // Перебираем адреса, до тех пор, пока не перебирем все
      EEPROM.get(add_ress, data_eeprom);                                      // Считываем значение
      if (data_eeprom.temp_e != -1)                                          // усли есть данные, формируем пает отправки
      {
        DEBUG_PRINT("#Temp1#");                                            // Выводим полученное значение в
        DEBUG_PRINT(data_eeprom.temp_e / 1000.00); //
        DEBUG_PRINT("#");  //
        DEBUG_PRINT(data_eeprom.unix_time);  //
        DEBUG_PRINT("\n");  //
        green_led.blink_n(3);
      }
      add_ress += sizeof(data_eeprom);                              // Наращиваем адрес
    }
    //Serial.println("##");                                                   // Выводим полученное значение в монитор
    //null_eeprom();
    //address = 0;
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void set_data_time() { // установка времени
  String _data_time;
  int16_t year = 0;
  int8_t month = 0;
  int8_t day = 0;
  int8_t hour = 0;
  int8_t minute = 0;
  int8_t second = 0;
  _data_time = sendATCommand("AT+CCLK?", true);    // Запрашиваем время
  _data_time.remove(0, 10);           // Remove the time from the data part
  year = (_data_time.substring(0, 2)).toInt();
  year = year + 2000;
  _data_time.remove(0, 3);           // Remove the time from the data part
  month = (_data_time.substring(0, 2)).toInt();
  _data_time.remove(0, 3);           // Remove the time from the data part
  day = (_data_time.substring(0, 2)).toInt();
  _data_time.remove(0, 3);           // Remove the time from the data part
  hour = (_data_time.substring(0, 2)).toInt();
  _data_time.remove(0, 3);           // Remove the time from the data part
  minute = (_data_time.substring(0, 2)).toInt();
  _data_time.remove(0, 3);           // Remove the time from the data part
  second = (_data_time.substring(0, 2)).toInt();
  setTime(hour, minute, second, day, month, year); // alternative to above, yr is 2 or 4 digit yr
  DEBUG_PRINTLN("TIME SET OK");

  return 0;
}

void send_eeprom() {
  myStruct data_eeprom;
  uint16_t add_ress = 0;
  EEPROM.get(0, data_eeprom);
  if (data_eeprom.temp_e != -1) {                                            // проверка на наличие данных в 0 адресе EEPROM

    while (add_ress < EEPROM.length()) {                                     // Перебираем адреса, до тех пор, пока не перебирем все
      EEPROM.get(add_ress, data_eeprom);                                      // Считываем значение
      if (data_eeprom.temp_e != -1)                                          // усли есть данные, формируем пает отправки
      {
        SIM800.print("\n#Temp1#");                                            // Выводим полученное значение в
        SIM800.print(data_eeprom.temp_e / 1000.00); //
        SIM800.print("#");  //
        SIM800.print(data_eeprom.unix_time);  //
        red_led.blink_n(1);
      }
      add_ress += sizeof(data_eeprom);                              // Наращиваем адрес
    }
    //address = 0;
  }
}
