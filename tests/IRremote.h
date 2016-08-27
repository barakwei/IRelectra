#pragma once

#ifndef IRremote_h
#define IRremote_h

class IRsend
{
public:
    void sendRaw(unsigned int buf[], int len, int hz);
};

#endif /* IRremote_h */
