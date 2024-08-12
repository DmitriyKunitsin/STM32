# Run from Notepad++ by F5:

# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

''' Отработка интерфейса регулятора РУС '''

#####################
# Установки программы
#####################
#SerialPortHWID = 'A10MDEWMA'    # Стенд РУС, адаптер 498_01
# SerialPortHWID = 'A906GWQUA'    # БИ в КО-5
#SerialPortHWID = '1A86:7523'    # CH340 у Никитоса
SerialPortHWID = '0403:6001'   # Преобразовтель USB-AB


#from math import pi, sin, cos, atan2, sqrt
import matplotlib
import matplotlib.pyplot as plt
#import matplotlib.lines as lines
#from matplotlib.widgets import Button, Slider, RadioButtons, CheckButtons
#import numpy as np
import threading
#import queue
from struct import *
#from datetime import datetime, timedelta
import time
#import copy
import os
import sys
import datetime
import matplotlib.pyplot as plt
import matplotlib.animation as animation

import tkinter as tk
from tkinter import *
import tkinter.ttk as ttk
from tkinter.messagebox import *


#import keys
# Добавить путь запущенного файла в список поиска, чтобы импортировать файлы из той же папки
CurrDir = os.path.dirname( os.path.abspath( __file__ ) )
sys.path.append( CurrDir )
from GUI import *
from SKLP import *
#from RUS_Utils import RVect, RVectPlot, SaverLAS
from RUS_Utils import SaverLAS

try:
    SKLP_Serial.PrintListCOM()
    Interface = SKLP_Serial( Baud = 115200, Timeout = 0.1, PortAutoHwid = SerialPortHWID, Name = 'SKLPM' )
except ValueError as Ex:
    print( Ex )
    quit()
Interface.bLogEnable = True
print( Interface.Status )

# Инициализировать модули на интерфейсе
RUS_Regul = SKLP_Module( Address = 0x1C, Interface = Interface )
RUS_Regul_Weight_Sensor = SKLP_Module(Address = 0x1D, Interface = Interface)

LAS_Names = ['Speed Set', 'Speed Meas', 'Angle Set', 'Cur Position', 'CNT Value', 'Press', 
            'Press ADC code', 'Current Press', 'Target Press', 'Power Supply', 'Current', 'Calibr Null',
            'Min Angle', 'Max Angle', 'Min Press', 'Max Press', 'WorkZone', 'WorkState',
            'WorkMode', 'Error flag', 'Logical flag']
            
            
LAS_Descr = ['Измеренная скорсоть', 'Выставленная скорость', 'Выставленный угол', 'Текущее положение', 'Счета таймера', 'Давление', 
            'Давление - код АЦП', 'Мгновенное давление', 'Выставленное давление', 'Напряжение питания', 'Ток потребления', 'Калибровочный ноль',
            'Мин. угол', 'Макс. угол', 'Мин. давление', 'Макс. давление', 'Рабочая область', 'Состояние',
            'Режим работы', 'Флаг ошибок', 'Флаг логики']            
 
class DataValve:
    def __init__( self, Datetime, aVals ):
        self.Datetime   = Datetime
        self.aVals      = aVals
        # self.SpeedSet   = aVals[0]
        # self.SpeedMeas  = aVals[1]
        # self.Volatge    = aVals[2]
        # self.Current    = aVals[3]
        # self.AngleSet   = aVals[4]
        # self.AngleMeas  = aVals[5]
        # self.CNTValue   = aVals[7]
        # self.Pressure   = aVals[8]
        
class SaverLAS_Valve( SaverLAS ):    # Сохранение накопленных данных в файл LAS (регулятор)
    def __init__( self, WorkDir ):
        SaverLAS.__init__( self, 'Valve', 0, WorkDir )
        self.atData = []    # [DataValve] - 9 fields
    def SaveCB( self, FromIndex ):
        if FromIndex == 0:  # данные еще не выгружались - произвести первоначальное сохранение через библиотеку lasio
            for i in range( len( self.atData[0].aVals ) ):
                self.FileLAS.append_curve( f'{LAS_Names[i]}', [ d.aVals[i] for d in self.atData ], descr = f'{LAS_Descr[i]}' )
            # self.FileLAS.append_curve( f'SpeedSet',     self.atData[0].aVals[0], descr = f'СпиидСет' )
            # self.FileLAS.append_curve( f'SpeedMeas',    self.atData[0].aVals[1], descr = f'СпиидМеазз' )
            # self.FileLAS.append_curve( f'Вольтаж',      self.atData[0].aVals[2], descr = f'Вольтаж' )
            # self.FileLAS.append_curve( f'Выставленная скорость%',     self.atData[0].SpeedSet,    descr = f'Выставленная скорость' )
            # self.FileLAS.append_curve( f'Измеренная скорость',    self.atData[0].SpeedMeas,   descr = f'Измеренная скорость' )
            # self.FileLAS.append_curve( f'Напряжение +12В',      self.atData[0].Volatge,     descr = f'Напряжение' )
            # self.FileLAS.append_curve( f'Ток потребления',      self.atData[0].Current,     descr = f'Ток потребления мотора' )
            # self.FileLAS.append_curve( f'Выставленный угол',      self.atData[0].AngleSet,     descr = f'Выставленный угол' )
            # self.FileLAS.append_curve( f'Измеренный угол',      self.atData[0].AngleMeas,     descr = f'Измеренный угол' )
            # self.FileLAS.append_curve( f'Значение счетчика таймера',      self.atData[0].CNTValue,     descr = f'Значение счетчика таймера' )
            # self.FileLAS.append_curve( f'Давление',      self.atData[0].Pressure,     descr = f'Давление' )
        else:               # данные уже выгружались - произвести добавление данных в конец открытого файла через обычный файловый вывод
            for Tag in self.atData[ FromIndex ].aVals:
                self.FileTxt.write( f'\t{Tag:.5f}' )
            # for Tag in self.atData[ FromIndex ].aVals[0], self.atData[ FromIndex ].aVals[1], self.atData[ FromIndex ].aVals[2]:
            # for Tag in self.atData[ FromIndex ].SpeedSet, self.atData[ FromIndex ].SpeedMeas, \
            # self.atData[ FromIndex ].Volatge, self.atData[ FromIndex ].Current, self.atData[ FromIndex ].AngleSet, \
            # self.atData[ FromIndex ].AngleMeas, self.atData[ FromIndex ].CNTValue, self.atData[ FromIndex ].Pressure:
                # self.FileTxt.write( f'\t{Tag:.5f}' )

SaverLAS_Valve = SaverLAS_Valve( CurrDir + '\\Logs' )
SaveTimestamp = datetime.datetime.now()

#объявление массивов для записи данных 
x_axis = []
speed_set = []
speed_meas = []
angle_set = []
current_pos = []
TIM_CNT = []
pressure = []
power_supply = []
consumption = []
calibr_null = []
WorkMode = []
weight = []

#test


# Объявление имен для заполнения из запроса
ParseNames = [ErrorFlagValue, LogicFlagValue, speedSetValue, speedMeasValue, angleSetValue,
              CurPosValue, CNTValue, PressValue,]


temp_value = 0
weightSensorHere = 0

def Polling_Button():
    
    i = 0
    
    windowGraph = 150
    plt.figure(figsize=[8,6])
    plt.gcf().set_facecolor('#C0C0C0')
    plt.subplots_adjust(wspace = 0.3, hspace = 0.6)

    
    while(i < 10000):
        AnswerPacket = RUS_Regul.Query( 0x14 )
        Parse = '=BHbHHHHhlhHBBHHHHHHBB'
        t = unpack( Parse, AnswerPacket )


        ErrorFlagValue["text"] = f'{t[0]}'
        LogicFlagValue["text"] = f'{t[1]}'
        speedSetValue["text"] = f'{t[2]}'
        speedMeasValue["text"] = f'{t[3]/100}'
        angleSetValue["text"] = f'{t[4]/100}'
        CurPosValue["text"] = f'{t[5]/100}'
        CNTValue["text"] = f'{t[6]}'
        PressValue["text"] = f'{t[7]/100}'
        PressCodeValue["text"] = f'{t[8]}'
        PressCurValue["text"] = f'{t[9]/100}'
        temp_value = t[10]/100
        PowerBoardValue["text"] = f'{t[11]/100}'
        CurrentValue["text"] = f'{t[12]/100}'
        # CalibrNullValue["text"] = f'{t[13]/100}'

        # Результаты калибровки
        MinAngleValue["text"] = f'{t[14]/100}'
        MaxAngleValue["text"] = f'{t[15]/100}'
        MinPressValue["text"] = f'{t[16]/100}'
        MaxPressValue["text"] = f'{t[17]/100}'
        WorkzoneValue["text"] = f'{t[18]/100}'
        WorkstateValue["text"] = f'{t[19]}'
        WorkmodeValue["text"] = f'{t[20]}'

        # AnswerPacket = RUS_Regul_Weight_Sensor.Query(0x14)
        # Parse = '=BHbHHHHhlhHBBHHHHHHBB'
        # ParseAnswer = unpack( Parse, AnswerPacket )
      
        # WeightSensorValue["text"] = f'{round(ParseAnswer[7]/100+13.11, 2)}'
        
        x_axis.append(datetime.datetime.now())
        # x_axis.append(i)
       
        current_pos.append(t[5]/100)
        pressure.append(t[9]/100)
        consumption.append(t[12]/100)
        

        # weight.append(ParseAnswer[7]/100 + 13.11)
        
        
        if(i >= windowGraph):   
            x_axis.pop(0)
            current_pos.pop(0)
            pressure.pop(0)
            consumption.pop(0)
            # weight.pop(0)
                        
        plt.subplot(3,2,1)

        plt.plot(x_axis, current_pos, color = 'green', linewidth = 3)
        plt.grid(True)
        plt.xlabel("Время", fontsize=16)
        plt.ylabel("Угол, °", fontsize=16)
        plt.title("Текущий угол", fontsize=20)
        
        plt.subplot(3,2,2)
        plt.plot(x_axis, consumption, color = 'red', linewidth = 3)
        plt.grid(True)
        plt.xlabel("Время", fontsize=16)
        plt.ylabel("Ток, А", fontsize=16)
        plt.title("Ток потребления", fontsize=20)
        
        plt.subplot(3,1,2)
        plt.plot(x_axis, pressure, color = 'blue', linewidth = 3)
        plt.grid(True)
        plt.xlabel("Время", fontsize=16)
        plt.ylabel("Давление, атм", fontsize=16)
        plt.title("Мгновенное давление", fontsize=20) 
        
        # plt.subplot(3,1,3)
        # plt.plot(x_axis, weight, color = 'purple', linewidth = 3)
        # plt.grid(True)
        # plt.xlabel("Время", fontsize=16)
        # plt.ylabel("Вес, кгс", fontsize=16)
        # plt.title("Усилие поршня", fontsize=20) 

 
        window.update_idletasks()
        plt.pause(0.01)
        plt.clf()

   
        DT = datetime.datetime.now()
        SaverLAS_Valve.atData.append( DataValve( DT, t ) )   # [DataValve]
        
        
        Timestamp = datetime.datetime.now()
        global SaveTimestamp
        if ( Timestamp - SaveTimestamp ) > datetime.timedelta( seconds=5.0):
            print( f'Saving to { SaverLAS_Valve.FileName }...' )
            SaverLAS_Valve.Save()
            SaveTimestamp = Timestamp
        i = i + 1
            
    return


def set_speed(): # Выставит скорость вращения
    if(setspeed_input.get() == ''):
        showerror(title="Error", message="Введите скорость")
    else:
        speed = float(setspeed_input.get())
        AnswerPacket = RUS_Regul.Query( 0x72, pack('=f', speed))
    return 0

def send_speed_and_angle(): # Выставить угол вращения
    if(SetSpeedAndAngle_input_speed.get() == '' or SetSpeedAndAngle_input_Angle.get() == ''):
        showerror(title = "Error", message = "Обнаружены пустые поля")
    else:
        speed = float(SetSpeedAndAngle_input_speed.get())
        angle = int(SetSpeedAndAngle_input_Angle.get())
        AnswerPacket = RUS_Regul.Query( 0x71, pack( '=hf', angle, speed ) ) 
    # print(f'Выставленная скорость: {speed} \nВыставленный угол: {angle}')

def start_calibr_consumption(): # Запустить калибровку по потреблению
    RUS_Regul.Query( 0x64 )

def start_calibr_pressure(): # Запустить калибровку по давлению
    RUS_Regul.Query( 0x63 )
    
def get_id_command():
    RUS_Regul.Query( 0x01 )
    
def power_on_motor_command():
    RUS_Regul.Query( 0x60, pack('=B', 0x01) )
    
def power_off_motor_command():
    RUS_Regul.Query( 0x60, pack('=B', 0x00) )

def overlap_set_command():
    percent = int(Overlap_Set_Entry.get())
    RUS_Regul.Query( 0x62, pack('=h', percent) )

def pressure_stabilization_command():
    targetPress = int(HoldPressure_Entry.get())
    RUS_Regul.Query( 0x65, pack('=H', targetPress ) )


StartMeasureButton['command'] = Polling_Button
GetID['command'] = get_id_command
PowerMotorOn['command'] = power_on_motor_command
PowerOffMotor['command'] = power_off_motor_command
SetSpeed['command'] = set_speed
SetSpeedAndAngle['command'] = send_speed_and_angle
StartCalibrationConsumption['command'] = start_calibr_consumption
StartCalibrationPressure['command'] = start_calibr_pressure
Overlap_Set_Button['command'] = overlap_set_command
HoldPressure_Button['command'] = pressure_stabilization_command

    
print( '##### Try to close process...' )
# PollThread.join()
print( '##### Process closed.' )

window.mainloop()

# Оставить консоль на экране до 'any key'
input( '\nPress \'Enter\' to exit\n' )
quit()
