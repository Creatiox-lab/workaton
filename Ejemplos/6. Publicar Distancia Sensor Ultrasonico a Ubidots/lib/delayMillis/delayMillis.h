#ifndef _DELAY_MILLIS_
#define _DELAY_MILLIS_

class DelayMillis{
  public:
    DelayMillis();  // Constructor
    
    void empezar(unsigned long delay);
    bool finalizado();
    void repetir();

  private:
    unsigned long _delay;
    unsigned long _tiempo_inicio;
};

#endif