# sleepypi2-sketchbook

This is a demo for uploading sketches to Sleepy Pi 2 from Raspberry Pi without Arduino IDE, but Makefile.

## Usage

1. Follow this instruction [Sleepy-Pi-Setup.sh](https://github.com/ljishen/Sleepy-Pi-Setup) to setup Sleepy Pi 2

1. Install `Arduino-Makefile` using any of [these methods](https://github.com/sudar/Arduino-Makefile/#installation)

1. Clone this repository
   ```bash
   git clone https://github.com/ljishen/sleepypi2-sketchbook.git
   ```

1. Run demo
   ```bash
   cd sleepypi2-sketchbook/ButtonOnOff_CurrentRead3
   make upload
   ```

1. Enjoy!


## Logger

```bash
ttylog -b 9600 -d /dev/ttyS0 -f | tee /path/to/logfile 2>&1
```
See [ttylog](http://ttylog.sourceforge.net/index.html) for more details.
