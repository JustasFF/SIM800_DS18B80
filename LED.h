class LED_Y {
  public:
    LED_Y(byte pin, uint16_t prd_w, uint16_t prd_s) {
      _pin = pin;
      _prd_w = prd_w;
      _prd_s = prd_s;
      pinMode(_pin, OUTPUT);
    }
    void blink_n(byte _count) {
      for (int i = 0; i < _count; i++) {
        _tmr_w = millis();
        while ((millis() - _tmr_w) <= _prd_w) {
          digitalWrite(_pin, true);
        }
        _tmr_s = millis();
          while ((millis() - _tmr_s) <= _prd_s) {
            digitalWrite(_pin, false);
          }
      }
    }
    
  private:
    byte _pin;
    uint32_t _tmr_w;
    uint32_t _tmr_s;
    uint16_t _prd_w;
    uint16_t _prd_s;
    bool _flag;
};
