#!/usr/bin/env python
import time
import os.path

# You need the following dts overlay:
# fragment@2 {
# 	target-path = "/soc/pwm@7e20c000";
#         __overlay__ {
# 		    pinctrl-names = "default";
# 		    pinctrl-0 = <&pwm0_1_gpio13>;
# 		    status = "okay";
# 	};
# };


period = 40000                  # In ns so 25khz.
prefix = "/sys/devices/platform/soc/fe20c000.pwm/pwm/pwmchip0/"


def to_file(fname: str, val: int):
    with open(os.path.join(prefix, fname), "w") as f:
        f.write(f"{val}\n")


def duty_cycle(pct: int):
    to_file("pwm1/duty_cycle", int(pct / 100.0 * period))


to_file("export", 1)
to_file("pwm1/period", period)
to_file("pwm1/enable", 1)
while True:
    # get CPU temp
    file = open("/sys/class/thermal/thermal_zone0/temp")
    temp = float(file.read()) / 1000.00
    temp = float('%.2f' % temp)
    print(temp)
    file.close()

    if(temp > 60):
        duty_cycle(100)
    elif(temp > 50):
        duty_cycle(75)
    elif(temp > 30):
        duty_cycle(50)
    else:
        duty_cycle(0)

    time.sleep(1)
