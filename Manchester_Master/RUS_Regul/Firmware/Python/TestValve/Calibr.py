# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

import numpy as np
from numpy.fft import fft, ifft
import matplotlib
import matplotlib.pyplot as plt

f_calibr = open('Calibr.txt', 'r')
calibr_from_txt = f_calibr.read()
f_calibr.close()

calibr_data = calibr_from_txt.split()


press = []
count = []

for i in range(len(calibr_data)):
    if i % 2 == 0:
        press.append(float(calibr_data[i]))
    else:
        count.append(int(calibr_data[i]))


# plt.plot(press)
# plt.show()

press_fft = np.fft.rfft( press )
press_freq = np.absolute(press_fft)[0:20]
x_axis = np.linspace(0, 20, num=20)
plt.stem( x_axis[1:], press_freq[1:])
plt.show()

main = [0]*len(press_fft)
main[0] = press_fft[0]
main[3] = press_fft[3]
main[5] = press_fft[5]
main_sine = np.fft.irfft(main)

plt.plot(main_sine)
plt.plot(press)
plt.show()


print( np.absolute(main[3]) )
print( np.angle(main[3])*180/3.14 )