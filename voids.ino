void detection() {                                                      // условия проверяемые каждые  сек
  uptime.calculateUptime();
  static byte interval;
  sensors.requestTemperatures();   // читаем температуру с датчика
  delay(800);
  tempds0 = sensors.getTempCByIndex(0); // опросить датчик DS18B20
  delay(50);
  if  (85 > tempds0 && tempds0 > -50) {
    temp_data.temp_e = tempds0 * 1000;
    temp_data.unix_time = now() - 10800;
    green_led.blink_n(1);
  }
  else {
    red_led.blink_n(1);
  }
  if (modem < 2) {
    set_data_time();
    delay(100);
    v = readVcc();
    delay(100);
    _sig = Signal();
    delay(200);
    Serial.print(F("|| Signal : "));
    Serial.print(_sig);
    Serial.print(F(" || voltage : "));
    Serial.print(v);
    //Serial.print(F(" || freeMemory: "));
    //Serial.print(freeMemory());
    DEBUG_PRINT(F(" || "));

  }
  DEBUG_PRINT(F("Modem : ")); DEBUG_PRINT(modem);
  DEBUG_PRINT(F(" || Count_send : ")); DEBUG_PRINT(count_send);
  DEBUG_PRINT(F(" || Interval : ")); DEBUG_PRINT(interval);
  Serial.print(F(" || Temp : ")); Serial.print(tempds0);
  Serial.print(F(" || Time : ")); digitalClockDisplay();
  interval--;
  if (interval == 253 ) { //1577836801 нужна проверка на дату
    interval = 0;
    modem = 1; //
  }
  if (modem == 1) {
    SIM800.println(F("AT+CIPSHUT"));
    modem = 2;
  }
}

void slee_P() {                                                         // ЦИКЛ ДЛЯ СНА
  uptime.calculateUptime();
  uint32_t n;
  // между снами 60.320 сек + нужный интервал сна = время между отправками
  if (v > 4050) {
    n = 549320; //10 minutes // средний интервал 
  } else if (v <= 4050 && v > 3900) {
    n = 853930;  //15 minutes

  } else if (v <= 3900 && v > 3700) {
    n = 1753930;//30 minutes
  }
  else n = 3533680;
  delay(50);
  power.hardwareDisable(PWR_ADC | PWR_TIMER2 | PWR_TIMER3 | PWR_TIMER4 | PWR_TIMER5 | PWR_UART0 | PWR_UART1 | PWR_UART2 | PWR_UART3 | PWR_I2C | PWR_SPI | PWR_USB); //(PWR_ADC | PWR_TIMER2 | PWR_TIMER3 | PWR_TIMER4 | PWR_TIMER5 | PWR_UART0 | PWR_UART1 | PWR_UART2 | PWR_UART3 | PWR_I2C | PWR_SPI | PWR_USB)
  //  Timer delays about few minutes between readings
  power.sleepDelay(n);//power.sleep(SLEEP_8192MS);
  power.hardwareEnable(PWR_ADC | PWR_TIMER2 | PWR_TIMER3 | PWR_TIMER4 | PWR_TIMER5 | PWR_UART0 | PWR_UART1 | PWR_UART2 | PWR_UART3 | PWR_I2C | PWR_SPI | PWR_USB);
}

int8_t Signal() {                                                          // замеряем dBa
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
  if (waiting) {                             // Если необходимо дождаться ответа...
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
    red_led.blink_n(5);
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
    SIM800.println(F("AT+CIPSHUT"));
    _resp = (F("REBOOT"));
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

long readVcc() {                                                       //функция чтения внутреннего опорного напряжения, универсальная (для всех ардуин) vbyen
  float my_vcc_const = 1.116;   // константа вольтметра
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
  myStruct eeprom_null;
  eeprom_null.temp_e = -128.00;
  eeprom_null.unix_time = 1072915201;
  for (unsigned int i = 0; i < EEPROM.length(); i += sizeof(eeprom_null)) { // Обнуляем EEPROM - приводим в первоначальное состояние
    EEPROM.put(i, eeprom_null);
  }
}

void print_eeprom() {
  String s = "";
  int count = 0;
  myStruct data_eeprom;
  uint16_t add_ress = 0;
  EEPROM.get(0, data_eeprom);
  if (data_eeprom.temp_e != -128.00) {                                            // проверка на наличие данных в 0 адресе EEPROM

    //Serial.print(name_mac + "\n");                                           // Выводим полученное значение в монитор
    //address = 0;                                                             // обнуляем адрес EEPROM
    while (add_ress < EEPROM.length()) {                                     // Перебираем адреса, до тех пор, пока не перебирем все
      EEPROM.get(add_ress, data_eeprom);                                      // Считываем значение
      if (data_eeprom.temp_e != -128.00)                                          // усли есть данные, формируем пает отправки
      {
        s += (F("#Temp1#"));                                            // Выводим полученное значение в 
        s += (data_eeprom.temp_e / 1000.00); //
        s += (F("#"));  //
        s += (data_eeprom.unix_time);  //
        s += (F("\n"));
        count ++;
        }
      if (count == 40) {
        Serial.print(s);  //
        s = "";
        count = 0;
      }
      add_ress += sizeof(data_eeprom);                              // Наращиваем адрес
    }
    if (s) {
      Serial.print(s);
    }
    green_led.blink_n(2), red_led.blink_n(2);
  }
}

void digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(F(" "));
  Serial.print(day());
  Serial.print(F("/"));
  Serial.print(month());
  Serial.print(F("/"));
  Serial.print(year());
  Serial.println();
}

void printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(F(":"));
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void set_data_time() { // установка времени
  uint32_t t;
  String _data_time;
  int16_t years = 0;
  int8_t month = 0;
  int8_t day = 0;
  int8_t hour = 0;
  int8_t minute = 0;
  int8_t second = 0;
  _data_time = sendATCommand("AT+CCLK?", true);    // Запрашиваем время
  _data_time.remove(0, 10);           // Remove the time from the data part
  years = (_data_time.substring(0, 2)).toInt();
  years = years + 2000;
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
  t = now();
  setTime(hour, minute, second, day, month, years); // alternative to above, yr is 2 or 4 digit yr "04/01/01,00:00:06+12" 01/01/2004 00:00:06 //1072908007
  if (varS%4 != 0){
  t = max(t, now());
  setTime(t);
  }
  DEBUG_PRINTLN(F("TIME SET"));

  return 0;
}

void send_eeprom() {
  String s = "";
  int count = 0;
  myStruct data_eeprom;
  uint16_t add_ress = 0;
  EEPROM.get(0, data_eeprom);
  if (data_eeprom.temp_e != -128.00) {                                            // проверка на наличие данных в 0 адресе EEPROM

    while (add_ress < EEPROM.length()) {                                     // Перебираем адреса, до тех пор, пока не перебирем все
      EEPROM.get(add_ress, data_eeprom);                                      // Считываем значение
      if (data_eeprom.temp_e != -128.00)                                          // усли есть данные, формируем пает отправки
      {
        s += (F("\n#Temp1#"));                                            // Выводим полученное значение в
        s += (data_eeprom.temp_e / 1000.00); //
        s += ("#");  //
        s += (data_eeprom.unix_time);  //
        count++;
      }
      if (count == 40) {
        SIM800.print(s);
        count = 0;
        s = "";
      }
      add_ress += sizeof(data_eeprom);                              // Наращиваем адрес
    }
    if (s){
      SIM800.print(s);
      }
    for (int i = 0; i < 3; i++) { //
      red_led.blink_n(3), green_led.blink_n(1);
      }  
    }
}

void sendSMS(String phone, String message) {
  sendATCommand("AT+CMGF=1;&W", true);
  sendATCommand("AT+CMGS=\"" + phone + "\"\r", true);             // Переходим в режим ввода текстового сообщения"AT+CMGS=\"+7xxxxxxxxxx\"\r"
  sendATCommand(message + "\r\n" + (String)((char)26), true);   // После текста отправляем перенос строки и Ctrl+Z
  delay(30000);
}

void clear_string(String &b) {
  char *cls = 0;
  b = cls;
  b = "";
}
