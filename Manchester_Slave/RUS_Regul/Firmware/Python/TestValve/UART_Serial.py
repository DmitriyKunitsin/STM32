import serial
import matplotlib.pyplot as plt
import datetime
# from decimal import Decimal
import numpy as np
from struct import *

SPort = serial.Serial(
    port = 'COM3',
    baudrate = 115200,
    parity = serial.PARITY_NONE,
    stopbits = serial.STOPBITS_ONE,
    bytesize = serial.EIGHTBITS,
    timeout = 0.2
)

data = []
x_axis = []
arr = []
# time = []
press_size = 4
time_size = 8
read_size = 62
plot_time = []

static_point = []
static_data = []

Packet = bytearray()

def toFixed(numObj, digits=0):
    return f"{numObj:.{digits}f}"

SPort.reset_input_buffer()
i = 0
j = 4  
q = 0
window_graph = 100
intWord = 0
with open('Power.txt' , 'w') as file:
    while(i < 10000):
        time_recieve = datetime.datetime.now()
        Frame = SPort.read(read_size).hex()
        if(len(Frame) == 124):
            if(int(Frame[0:4], 16) == 65535):
                print(f'{time_recieve}\t{Frame}')
                
                Press_idx_start = 4
                Time_idx_start = 8

                for j in range(len(Frame[4:]) // 12):
                        Press_idx_end = Press_idx_start + press_size
                        Press = int( (Frame[Press_idx_start:Press_idx_end]), 16 )

                        Time_idx_end = Time_idx_start + time_size
                        Time = int( (Frame[Time_idx_start:Time_idx_end]), 32 )

                        if(Press >= 32767):
                            Press = Press - 65535

                        Press = (Press / 100) * 9.8
                        Press = toFixed(Press, 2)

                        # print(Press_idx_start, '\t', Time_idx_start)
                        # print(f'{Press}\t{Time}\n')

                        data.append(f'{Press}')
                        plot_time.append(Time)

                        Press_idx_start = Press_idx_start + time_size + press_size
                        Time_idx_start = Time_idx_start + time_size + press_size

                
                for q in range(len(data)):
                     data[q] = float(data[q])
                     plot_time[q] = int(plot_time[q])

                if(i >= window_graph):
                     data.pop(0)
                     plot_time.pop(0)

                plt.plot(plot_time, data, 'r-', linewidth = 2)
                plt.grid()
                plt.pause(0.01)
                plt.clf()    

                i = i + 1            

                # for j in range(len(Frame[4:]) // 4):
                #     intWord = int((Frame[j * word_size + 4 : j*word_size+word_size + 4]), 16)
                #     x_tick = 10*i+j

                #     if (intWord >= 32767):
                #         intWord = intWord - 65535

                #     data.append(f'{(intWord/100) * 9.8 }')
                #     plot_time.append(f'{x_tick}')

                #     static_point.append(f'{x_tick}')
                #     static_data.append(f'{(intWord - 3)}')

                #     if(i >= window_graph):
                #         data.pop(0)
                #         plot_time.pop(0)

                #     file.write(f'{x_tick}\t{intWord}\n')
                #     file.flush()

                # for q in range(len(data)):
                #     data[q] = float(data[q])
                #     plot_time[q] = int(plot_time[q])

                # plt.plot(plot_time, data, 'r-', linewidth = 2)
                # plt.grid()
                # plt.pause(0.001)
                # plt.clf()


# plt.plot(static_point, static_data, 'r-', linewidth = 3)
# plt.grid()
# plt.show()


    