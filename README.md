IRelectra
=========

Electra A/C IR Encoder for IRremote

The project can be used to control Electra A/C that are common is Israel.<br>
It doesn't has all the features that exist in the original remote, but only a subset.<br>
You can control: <br>
<ul>
<li> Whether to turn the A/C ON or OFF (A bit tricky). </li>
<li> A/C mode: cool, heat, dry, fan or auto. </li>
<li> Fan speed: low, medium, high or auto. </li>
<li> Swing: on/off </li>
<li> Sleep mode: on/off (raise temperature by one degree every 3 hours, turn off A/C after 8 hours) </li>
</ul>

Other features like timers or "I feel" are not supported.

IRelectra uses IRremote to do the heavy lifting.

Usage:

```cpp
#include "IRremote.h"
#include "IRelectra.h"
  
void loop()
{
  IRsend irsend(D3);
  IRelectra e(&irsend);
  e.SendElectra(POWER_OFF, MODE_COOL, FAN_LOW, 24, SWING_ON, SLEEP_OFF);
}
```

The code has been tested on [Spark core](www.spark.io) and is agnostic to IRremote implementation as long as it implements the sendRaw method as declared [here](https://github.com/shirriff/Arduino-IRremote/blob/master/IRremote.h).
The demo code uses the IRremote implementation for Spark core from [here](https://github.com/qwertzguy/Spark-Core-IRremote/), the deme code might need to change according to the implementation you're using.


Big thanks for Chris from AnalysIR (http://www.analysir.com/)
