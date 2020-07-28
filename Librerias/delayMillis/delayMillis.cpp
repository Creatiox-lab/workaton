#include <Arduino.h>
#include <delayMillis.h>

DelayMillis::DelayMillis(){
  _tiempo_inicio = 0;
}

void DelayMillis::empezar(unsigned long delay){
  _delay = delay;
  _tiempo_inicio = millis();
}

bool DelayMillis::finalizado(){
  if( (millis() - _tiempo_inicio) >= _delay ) {
    return true;
  } else {
    return false;
  }
}

void DelayMillis::repetir(){
  _tiempo_inicio = millis();
}