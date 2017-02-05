#pragma once

#ifndef IRremote_h
#define IRremote_h

class IRsend
{
public:
    virtual void sendRaw(unsigned int buf[], int len, int hz) = 0;
};

#endif /* IRremote_h */
