# Run from Notepad++ by F5:

# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

''' Отработка интерфейса регулятора РУС '''

#####################
# Установки программы
#####################
#SerialPortHWID = 'A10MDEWMA'    # Стенд РУС, адаптер 498_01
# SerialPortHWID = 'A906GWQUA'    # БИ в КО-5
SerialPortHWID = '1A86:7523'    # CH340 у Никитоса
#SerialPortHWID = '0403:6001'   

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
#import keys
# Добавить путь запущенного файла в список поиска, чтобы импортировать файлы из той же папки
CurrDir = os.path.dirname( os.path.abspath( __file__ ) )
sys.path.append( CurrDir )
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

#RUS_Regul.Query_GetID()
# False
# True

class DataValve:
    def __init__( self, Datetime, aVals ):
        self.Datetime   = Datetime
        # self.aVals      = aVals
        self.SpeedSet   = aVals[0]
        self.SpeedMeas  = aVals[1]
        self.Volatge    = aVals[2]
        self.Current    = aVals[3]
        self.AngleSet   = aVals[4]
        self.AngleMeas  = aVals[5]
        self.CNTValue   = aVals[7]
        self.Pressure   = aVals[8]
        
class SaverLAS_Valve( SaverLAS ):    # Сохранение накопленных данных в файл LAS (регулятор)
    def __init__( self, WorkDir ):
        SaverLAS.__init__( self, 'Valve', 0, WorkDir )
        self.atData = []    # [DataValve] - 9 fields
    def SaveCB( self, FromIndex ):
        if FromIndex == 0:  # данные еще не выгружались - произвести первоначальное сохранение через библиотеку lasio
            # for i in range( len( self.atData[0].aVals ) ):
                # self.FileLAS.append_curve( f'Tag{i}', [ d.aVals[i] for d in self.atData ], descr = f'Valve Tag {i}' )
            # self.FileLAS.append_curve( f'SpeedSet',     self.atData[0].aVals[0], descr = f'СпиидСет' )
            # self.FileLAS.append_curve( f'SpeedMeas',    self.atData[0].aVals[1], descr = f'СпиидМеазз' )
            # self.FileLAS.append_curve( f'Вольтаж',      self.atData[0].aVals[2], descr = f'Вольтаж' )
            self.FileLAS.append_curve( f'Выставленная скорость%',     self.atData[0].SpeedSet,    descr = f'Выставленная скорость' )
            self.FileLAS.append_curve( f'Измеренная скорость',    self.atData[0].SpeedMeas,   descr = f'Измеренная скорость' )
            self.FileLAS.append_curve( f'Напряжение +12В',      self.atData[0].Volatge,     descr = f'Напряжение' )
            self.FileLAS.append_curve( f'Ток потребления',      self.atData[0].Current,     descr = f'Ток потребления мотора' )
            self.FileLAS.append_curve( f'Выставленный угол',      self.atData[0].AngleSet,     descr = f'Выставленный угол' )
            self.FileLAS.append_curve( f'Измеренный угол',      self.atData[0].AngleMeas,     descr = f'Измеренный угол' )
            self.FileLAS.append_curve( f'Значение счетчика таймера',      self.atData[0].CNTValue,     descr = f'Значение счетчика таймера' )
            self.FileLAS.append_curve( f'Давление',      self.atData[0].Pressure,     descr = f'Давление' )
        else:               # данные уже выгружались - произвести добавление данных в конец открытого файла через обычный файловый вывод
            # for Tag in self.atData[ FromIndex ].aVals:
            # for Tag in self.atData[ FromIndex ].aVals[0], self.atData[ FromIndex ].aVals[1], self.atData[ FromIndex ].aVals[2]:
            for Tag in self.atData[ FromIndex ].SpeedSet, self.atData[ FromIndex ].SpeedMeas, \
            self.atData[ FromIndex ].Volatge, self.atData[ FromIndex ].Current, self.atData[ FromIndex ].AngleSet, \
            self.atData[ FromIndex ].AngleMeas, self.atData[ FromIndex ].CNTValue, self.atData[ FromIndex ].Pressure:
                self.FileTxt.write( f'\t{Tag:.5f}' )

SaverLAS_Valve = SaverLAS_Valve( CurrDir + '\\Logs' )



def Polling( Plotter, Saver ):
    if False:       # Get ID
        print( '\n*** Get ID...' )
        if RUS_Regul.Query_GetID() != True:
            print( 'Fail!' )
            return
        print( 'Ok!' )
    
    if False:        # Калибровка по потреблению
        AnswerPacket = RUS_Regul.Query( 0xCC )
        
    if False:       # Калибровка по давлению (min/max)
        AnswerPacket = RUS_Regul.Query( 0xCD )
        
    if True:       # Выставление угла поврота и скорости вращения
        speed = 100
        Angle = 10
        print( f'\n*** Rotate by {Angle}°...' )
        AnswerPacket = RUS_Regul.Query( 0xCB, pack( '=fh', speed, Angle ) ) 
        
    if False:       # Выставление скорости вращения (вращение в одну сторону)
        speed = -100
        print( f'\n*** Speed {speed}...' )
        AnswerPacket = RUS_Regul.Query( 0xFA, pack('=f', speed))
    
    if False:
        press = 105  # Выставление давления от минимального до максимального в атмосферах
        print(f'Удерживать давление {press} атмосфер')
        AnswerPacket = RUS_Regul.Query( 0xCE, pack('=l', press) )
        
    if True:
        Plotter.Label = [ 'Current Position', 'Press' ]
        Plotter.PlotInterval = 0.5        # [s]
        Plotter.Window = 1000              # [counts per window]
        Plotter.Marker = '', ''           
        Plotter.Linestile = 'solid', 'solid'

        calibr = open('Calibr.txt', 'w')
        i = 0 
        while(i < 10000):
            AnswerPacket = RUS_Regul.Query( 0xBE )
            Parse = '=fffHHfffffffffBBH'
            print( AnswerPacket ) 
            print( f'Receive size = {len(AnswerPacket)}, Parse size = {calcsize(Parse)}' )
            # ( SpeedSet, SpeedMeas ) = unpack( Parse, AnswerPacket )
            # print( f'SpeedMeas = {SpeedMeas}' )
            t = unpack( Parse, AnswerPacket )
            print('Выставленная скорость:\t\t',     t[0],        '%\n',
                  'Измеренная скорость:\t\t',       t[1],        'об/мин\n',
                  'Выставленный угол:\t\t',         t[2],        '°\n',
                  'Текущее положение:\t\t',         (t[3]/100),  '°\n',
                  'Счета таймера:\t\t\t',           t[4],        '\n',
                  'Давление:\t\t\t',                t[5],        'атм\n',
                  'Напряжение пиатния:\t\t',        t[6],        'В\n',
                  'Ток потребления:\t\t',           t[7],        'А\n',
                  'Калибровочный ноль\t\t',         t[8],        '°\n',
                  'Режим работы\t\t\t',             t[14])
                  
            Plotter.Append( [t[3]/100], [t[5]] )

            calibr.write( f'{t[5]:0.3f}\t{t[4]}\n' )
            calibr.flush()
 
       
            DT = datetime.datetime.now()
            Saver.atData.append( DataValve( DT, t ) )   # [DataValve]
            time.sleep(0.1)
            i = i + 1
    
        return
    
'''
    if False:       # Speed Get
        AnswerPacket = RUS_Regul.Query( 0xCA )
        
    if False:        # Calibration Set
        AnswerPacket = RUS_Regul.Query( 0xCC )
i = 0
while(i < 1000):
    print( '\n*** Get ID... at', datetime.datetime.now() )
    if RUS_Regul.Query_GetID() != True:
        print( 'Fail!' )
    print( 'Ok!' )
    time.sleep(0.5)
    i = i + 1
    
    
                xaxis.append(i)
            SpeedMeas.append(t[1])
            
            for elem in SpeedMeas:
                StringSpeedMeas += str(elem)
            fp = open('figure.txt', 'w')
            fp.write(str(xaxis))
            fp.write(f'{StringSpeedMeas}, \n')
            fp.close()
                
            fig = plt.figure();
            ax = fig.add_subplot(1,1,1)

            def animate(i):
                data = open('figure.txt', 'r',).read()
                lines = data.split('\n')

                for line in lines:
                    xaxis, SpeedMeas = line.split(',')
                    xaxis.append(float(x))
                    SpeedMeas.append(float(y))

            ax.clear()
            ax.plot(xaxis, SpeedMeas)
            plt.xlabel('data')
            plt.ylabel('price')
            plt.title('graph in real time')

            anim = animation.FuncAnimation(fig, animate, interval = 1000)
            
            if(i == 0):
                plt.show()
                
            SpeedMeas = []
xaxis = []
StringSpeedMeas = ''
    
    '''
'''
class Plotter:
    def __init__( self, Caption = 'Test Plotter' ):
        plt.rcParams['toolbar'] = 'None'    # убрать кнопки
        self.Fig = plt.figure( figsize = ( 10, 5 ), num = Caption )
        self.Fig.patch.set_visible( False )      # убрать рамку?
        self.Fig.tight_layout()                  # убрать зазор между графиками и окном?
        plt.axis('off')
        self.ax = self.Fig.add_subplot( )
        self.ax.set_position( ( 0.05, 0.05, 0.9, 0.9 ) )
        self.Label      = 'Test'
        self.Marker     = 'o'
        self.Linestile  = 'dashed'
        self.PlotInterval = 1.0
        self.Window = 100
        self.aData = []
        self.LegendLocation = 'upper left'

    def Append( self, Data ):
        self.aData.append( Data )
        # print( self.aData )
        
    def Plot( self ):
        self.ax.clear()
        ( TimeWindow, TimeDataSize ) = ( self.Window, len( self.aData ) )
        if( TimeDataSize > TimeWindow ):
            self.ax.set_xlim( TimeDataSize - TimeWindow, TimeDataSize )
            
        self.ax.plot( self.aData, label=self.Label, marker=self.Marker, linestyle=self.Linestile )
        self.ax.legend( loc = self.LegendLocation )

        plt.draw()
        plt.pause( self.PlotInterval )
'''

class Plotter2:
    def __init__( self, Caption = 'Test Plotter' ):
        plt.rcParams['toolbar'] = 'None'    # убрать кнопки
        self.Fig = plt.figure( figsize = ( 10, 5 ), num = Caption )
        self.Fig.patch.set_visible( False )      # убрать рамку?
        self.Fig.tight_layout()                  # убрать зазор между графиками и окном?
        plt.axis('off')
        self.Fig, ( self.ax1, self.ax2 ) = plt.subplots( nrows = 2, ncols = 1, sharex = True )
        self.ax1.set_position( ( 0.05, 0.55, 0.9, 0.4 ) )
        self.ax2.set_position( ( 0.05, 0.05, 0.9, 0.4 ) )
        self.Label      = 'Test1', 'Test2'
        self.Marker     = 'o', '_'
        self.Linestile  = 'dashed', 'solid'
        self.PlotInterval = 1.0
        self.Window = 100
        self.aData1 = []
        self.aData2 = []
        self.LegendLocation = 'upper left'

    def Append( self, Data1, Data2 ):
        self.aData1.append( Data1 )
        self.aData2.append( Data2 )
        # print( self.aData )
        
    def Plot( self ):
        self.ax1.clear()
        ( TimeWindow, TimeDataSize ) = ( self.Window, len( self.aData1 ) )
        if( TimeDataSize > TimeWindow ):
            self.ax1.set_xlim( TimeDataSize - TimeWindow, TimeDataSize )
        self.ax2.clear()
        ( TimeWindow, TimeDataSize ) = ( self.Window, len( self.aData2 ) )
        if( TimeDataSize > TimeWindow ):
            self.ax2.set_xlim( TimeDataSize - TimeWindow, TimeDataSize )
            
        self.ax1.plot( self.aData1, label=self.Label[0], marker=self.Marker[0], linestyle=self.Linestile[0] )
        self.ax2.plot( self.aData2, label=self.Label[1], marker=self.Marker[1], linestyle=self.Linestile[1] )
        self.ax1.legend( loc = self.LegendLocation )
        self.ax2.legend( loc = self.LegendLocation )
        # self.ax2.sharex( self.ax1 )

        plt.draw()
        plt.pause( self.PlotInterval )


ValvePlotter = Plotter2( Caption = 'Valve Plotter' )

PollThread = threading.Thread( target = Polling, args = ( ValvePlotter, SaverLAS_Valve, ) )
PollThread.start()

SaveTimestamp = datetime.datetime.now()

while PollThread.is_alive():
    # Отрисовать график
    ValvePlotter.Plot()
    
    '''# Выгрузить данные в LAS
    Timestamp = datetime.datetime.now()
    if ( Timestamp - SaveTimestamp ) > 5.0:
        print( f'Saving to { SaverLAS_Valve.FileName }...' )
        SaverLAS_Valve.Save()
        SaveTimestamp = Timestamp
        # time.sleep( 5.0 )
    '''
    
print( '##### Try to close process...' )
PollThread.join()
print( '##### Process closed.' )

# Оставить консоль на экране до 'any key'
input( '\nPress \'Enter\' to exit' )
quit()
