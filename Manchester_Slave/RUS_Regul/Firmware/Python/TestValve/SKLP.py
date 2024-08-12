""" Установка PySerial - не входит в пакет Литвинова!
- если есть инет:
%LOOCH_PYTHON_DIR%\python.exe -m pip install pyserial
- если нету инета, скачать и выполнить
https://files.pythonhosted.org/packages/07/bc/587a445451b253b285629263eb51c2d8e9bcea4fc97826266d186f96f558/pyserial-3.5-py2.py3-none-any.whl
%LOOCH_PYTHON_DIR%\python.exe -m pip install %LOOCH_PYTHON_DIR%\pyserial-3.5-py2.py3-none-any.whl
- если есть лучевский обрезанный интернет и работает заплатка на сервере:
%LOOCH_PYTHON_DIR%\python -m pip install pyserial -i http://opo.looch.ru:4040/root/pypi/+simple --trusted-host opo.looch.ru
"""

'''
Библиотека обмена с модулями СКЛ:
- SKLP                  SKLP, базовый класс интерфейса обмена
- SKLP_Serial           SKLP, транспорт через COM-порт
- SKLP_Emul             SKLP, виртуальный транспорт через очереди
Библиотека модулей СКЛ:
- SKLP_Module           Базовый класс
- SKLP_ModuleInclin     Инклинометр 173/174/РУС
- SKLP_ModuleGK         ВИКПБ.ГК
- SKLP_Module_RUSPump   РУС, электропривод насоса
- SKLP_Module_RUSTele   РУС, телеметрия
- SKLP_Module_GGP_SADC  ГГКП, счетчик гаммы
- SKLP_Module_MUP       МУП
'''

from struct import *
import serial
import serial.tools.list_ports
import queue
import struct
import threading

# Интерфейс обмена по SKLP, Master/Slave
class SKLP:
    ''' Базовый класс интерфейса обмена по SKLP. Транспорт должен быть определен в наследующем классе. '''
    __aCRC8_Table = [
    0x00,0xBC,0x01,0xBD,0x02,0xBE,0x03,0xBF,0x04,0xB8,0x05,0xB9,0x06,0xBA,0x07,0xBB,
    0x08,0xB4,0x09,0xB5,0x0A,0xB6,0x0B,0xB7,0x0C,0xB0,0x0D,0xB1,0x0E,0xB2,0x0F,0xB3,
    0x10,0xAC,0x11,0xAD,0x12,0xAE,0x13,0xAF,0x14,0xA8,0x15,0xA9,0x16,0xAA,0x17,0xAB,
    0x18,0xA4,0x19,0xA5,0x1A,0xA6,0x1B,0xA7,0x1C,0xA0,0x1D,0xA1,0x1E,0xA2,0x1F,0xA3,
    0x20,0x9C,0x21,0x9D,0x22,0x9E,0x23,0x9F,0x24,0x98,0x25,0x99,0x26,0x9A,0x27,0x9B,
    0x28,0x94,0x29,0x95,0x2A,0x96,0x2B,0x97,0x2C,0x90,0x2D,0x91,0x2E,0x92,0x2F,0x93,
    0x30,0x8C,0x31,0x8D,0x32,0x8E,0x33,0x8F,0x34,0x88,0x35,0x89,0x36,0x8A,0x37,0x8B,
    0x38,0x84,0x39,0x85,0x3A,0x86,0x3B,0x87,0x3C,0x80,0x3D,0x81,0x3E,0x82,0x3F,0x83,
    0x40,0xFC,0x41,0xFD,0x42,0xFE,0x43,0xFF,0x44,0xF8,0x45,0xF9,0x46,0xFA,0x47,0xFB,
    0x48,0xF4,0x49,0xF5,0x4A,0xF6,0x4B,0xF7,0x4C,0xF0,0x4D,0xF1,0x4E,0xF2,0x4F,0xF3,
    0x50,0xEC,0x51,0xED,0x52,0xEE,0x53,0xEF,0x54,0xE8,0x55,0xE9,0x56,0xEA,0x57,0xEB,
    0x58,0xE4,0x59,0xE5,0x5A,0xE6,0x5B,0xE7,0x5C,0xE0,0x5D,0xE1,0x5E,0xE2,0x5F,0xE3,
    0x60,0xDC,0x61,0xDD,0x62,0xDE,0x63,0xDF,0x64,0xD8,0x65,0xD9,0x66,0xDA,0x67,0xDB,
    0x68,0xD4,0x69,0xD5,0x6A,0xD6,0x6B,0xD7,0x6C,0xD0,0x6D,0xD1,0x6E,0xD2,0x6F,0xD3,
    0x70,0xCC,0x71,0xCD,0x72,0xCE,0x73,0xCF,0x74,0xC8,0x75,0xC9,0x76,0xCA,0x77,0xCB,
    0x78,0xC4,0x79,0xC5,0x7A,0xC6,0x7B,0xC7,0x7C,0xC0,0x7D,0xC1,0x7E,0xC2,0x7F,0xC3 ]
    class Signature:
        StartQuery  = 0x40
        StartAnswer = 0x23
    class AddressList:
        Broadcast   = 0x00
        MUP         = 0x62      # МУП
        NNKT        = 0x71      # ННКТ
        GGLP_SADC   = 0x88      # два µC на одном адресе, но на разных шиных
        GGLP_SADC_S = 0x89      # ближний зонд (не используется)
        GGLP_SADC_L = 0x8A      # дальний зонд (не используется)
        Inclin      = 0x99
        MPI         = 0xEA
        # Модемные
        BKS         = 0x24      # БКС
        VIKPB       = 0x55      # ВИКПБ
        VIKPB_GK    = 0x56      # ВИКПБ ГК
        NNKTe       = 0x73      # ННКТ(б)
        GGP         = 0x82      # ГГкП
        ACP_Aux     = 0xB1      # АКП(б), вспомогательный канал
        ACP         = 0xB2      # АКП(б)
        # РУС
        RUS_Tele    = 0x1A      # РУС (Теле)
        RUS_Pump    = 0x1B      # РУС контроллер электропривода (широковещательный)
        RUS_Pump0   = 0x1C      # РУС контроллер электропривода 0°
        RUS_Pump1   = 0x1D      # РУС контроллер электропривода 120°
        RUS_Pump2   = 0x1E      # РУС контроллер электропривода 240°
        RUS_GK      = VIKPB_GK
        RUS_Inclin  = Inclin

    def __init__( self, Timeout = 0.5, Name = 'SKLP' ):
        ''' Установка таймаута во время передачи пакета через модуль 'serial'
        иногда приводит к нарушению передаваемого фрейма UART, во всяком случае на плате 498_01 с чипом FTDI.
        Из-за этого установку таймаута порта необходимо производить до начала передачи запроса.
        Оказалось удобнее исключить аргумент 'Timeout' из функции __RecFrame(), и передавать таймаут через поле класса.
        '''
        self.Timeout = Timeout  
        self.Name = Name

    def CalcCRC8( Packet ) -> int:
        CRC8 = 0
        PacketLen = len( Packet )
        for i in range( 0, PacketLen ):
            CRC8 = SKLP.__aCRC8_Table[ CRC8 ^ Packet[i] ]
        return CRC8
        
    def Calc_CRC16( Packet: bytearray, Size, CRC16 ):
        h_12 = 255
        h_1 = 255
        hh_5 = 255
        hl_5 = 255
        temp = 65535
        i = 0
        while(Size > 0):
            h_12 = CRC16 >> 8
            h_1 = h_12
            h_12 = (h_12 >> 4)|(h_12 << 4)
            hh_5 = h_12
            h_12 ^= h_1
            h_12 &= 0xF0
            hh_5 &= 0x0F
            h_1 ^= hh_5
            hl_5 = h_12
            temp = ( (hh_5 << 8) | hl_5 )
            temp <<= 1
            hh_5 = temp >> 8
            hl_5 = temp
            h_1 ^= hl_5
            h_12 ^= hh_5
            hl_5 = CRC16
            h_1 ^= Packet
            h_12 ^= hl_5
            CRC16 = ( (h_12 << 8) | h_1 )
            Size = Size - 1
            i += 1
        return CRC16
        
#   Транспортные функции должны быть определены в наследующих классах
#    def __SendFrame( self, Frame :bytearray ):
#    def __RecFrame( self, Timeout = None ) -> ( bytearray, int ):

    def __SendPacket( self, Signature :Signature, Packet :bytearray ) -> None:
        PacketSize = len( Packet )
        if PacketSize > 0xFF:
            PacketSize = 0xFF
        Frame = bytearray( [ Signature, PacketSize ] ) + Packet
        CRC = SKLP.CalcCRC8( Frame )
        Frame = Frame + bytearray( [ CRC ] )
        self.__SendFrame( Frame )

    def __SendQuery( self, Address, Function, Data = bytearray() ) -> None:
        self.__SendPacket( SKLP.Signature.StartQuery, bytearray( [ Address, Function ] ) + Data )
           
    def Answer( self, Data ) -> None:
        self.__SendPacket( SKLP.Signature.StartAnswer, Data )

    def Query( self, Address, Function, Data = bytearray(), RecAnswer = True, Timeout = None ) -> bytearray:
        if Timeout != None:
            self.Timeout = Timeout
        self.__SendQuery( Address, Function, Data )
        if RecAnswer:
            Frame, Signature = self.__RecFrame( Timeout = None )
            if Signature != SKLP.Signature.StartAnswer:
                Frame = bytearray()
            return Frame

    def RecQuery( self, Timeout = None ) -> ( int, int, bytearray ):   # Address, Command, Data
        Frame, Signature = self.__RecFrame( Timeout )
        if ( Frame == None ) or ( Signature != SKLP.Signature.StartQuery ):
            return None
        FrameSize = len ( Frame )
        if FrameSize < 2:
            return None
        return ( Frame[0], Frame[1], Frame[2:FrameSize] )

class SKLP_Serial( SKLP ):
    ''' Интерфейс SKLP, транспорт через COM-порт '''
    def PrintListCOM():
        print( 'COM ports list:' )
        ports = sorted( serial.tools.list_ports.comports() )
        for ( port, desc, hwid ) in ports:
            print( "{}:\t{:30}\t{}".format( port, desc, hwid ) )

    def __init__( self, Port = None, Baud = 115200, Timeout = 0.5, PortAutoDesc = None, PortAutoHwid = None, Name = 'SKLP_Serial' ):
        SKLP.__init__( self, Timeout, Name )
        if( ( Port == None ) and ( ( PortAutoDesc != None ) or ( PortAutoHwid != None ) ) ):
            ports = serial.tools.list_ports.comports()
            for ( port, desc, hwid ) in sorted( ports ):
                if( ( ( PortAutoDesc != None ) and ( desc.find( PortAutoDesc ) >= 0 ) ) or \
                    ( ( PortAutoHwid != None ) and ( hwid.find( PortAutoHwid ) >= 0 ) ) ):
                    Port = port
                    break
        self.SPort = None
        if( Port == None ):
            Signature = PortAutoDesc if ( PortAutoDesc != None ) else PortAutoHwid
            raise ValueError( Name + ' can\'t found COM \"{}\"!'.format( Signature ) )
        self.SPort = serial.Serial(
            port        = Port,
            baudrate    = Baud,
            parity      = serial.PARITY_NONE,
            stopbits    = serial.STOPBITS_ONE,
            bytesize    = serial.EIGHTBITS,
            timeout     = self.Timeout )
        self.Status = self.Name + ' connected to ' + self.SPort.portstr
        self.SPort.reset_input_buffer()
        self.LastRecFrame = None
        self.bLogEnable = False
        self.WriteMtx = threading.Lock()
        
    def __del__( self ):
        if self.SPort != None:
            self.SPort.close()

    def _SKLP__SendFrame( self, Frame :bytearray ):
        self.WriteMtx.acquire()
        try:
            self.SPort.reset_input_buffer()
            self.SPort.reset_output_buffer()
            self.SPort.timeout = self.Timeout  # !! Установка таймаута во время передачи пакета приводит к нарушению фрейма UART, во всяком случае на плате 498_01 с чипом FTDI, поэтому таймаут проходится устанавливать до начала передачи запроса!
            self.SPort.write( Frame )
            if self.bLogEnable:
                print( '>> {}:\t[{}]\t{}'.format( self.SPort.portstr, len( Frame ), Frame.hex() ) )
        except serial.SerialException:
            return None
        finally:
            self.WriteMtx.release()

    def _SKLP__RecFrame( self, Timeout = None ) -> ( bytearray, int ):
        ''' Реализация приема только коротких пакетов, с полем размера от 1 до 254! '''
        try:
            if Timeout != None:                     # Установку таймаута производить только при ожидании приема асинхронного пакета. В режиме запрос-ответ таймаут устанавливать перед началом передачи запроса!
                self.Timeout = Timeout
                self.SPort.timeout = self.Timeout
            Frame = self.SPort.read( 2 )
            if ( Frame != None ) and ( len( Frame ) == 2 ):
                PacketSize = Frame[1];
                Frame = Frame + self.SPort.read( PacketSize + 1 )
                if ( 0 < PacketSize < 255 ) and ( SKLP.CalcCRC8( Frame ) == 0 ) :
                    self.SPort.reset_input_buffer()
                    self.LastReqFrame = Frame
                    if self.bLogEnable:
                        print( '<< {}:\t[{}]\t{}'.format( self.SPort.portstr, len( Frame ), Frame.hex() ) )
                    return Frame[ 2 : PacketSize+2 ], Frame[0]
            self.SPort.reset_input_buffer()
            raise serial.SerialException
        except serial.SerialException:
            return None, None
        
class SKLP_Emul( SKLP ):
    ''' Интерфейс SKLP, виртуальный транспорт через очереди '''
    def __init__( self, QueTx = None, QueRx = None, Name = 'SKLP_Emul' ):
        SKLP.__init__( self, Timeout = None, Name = Name )
        if ( QueRx == None ) or ( QueTx == None ):
            raise ValueError( Name + ' can\'t connected to Queues!' )
        self.Name = Name
        self.QueTx = QueTx
        self.QueRx = QueRx
        self.Status = '{}.Rx connected to Queue at 0x{:0X}, {}.Tx connected to Queue at 0x{:0X}'.format( self.Name, id( self.QueRx ), self.Name, id( self.QueTx ) )
        self.LastRecFrame = None

    def CreatePair( Name = 'SKLP_Emul' ) -> ( 'SKLP_Emul_Master', 'SKLP_Emul_Salve' ):
        ''' Создать пару интерфейсов, соединенных через очереди '''
        QueMasterTx = queue.Queue( maxsize = 4 )
        QueMasterRx = queue.Queue( maxsize = 4 )
        SKLP_EmulMaster = SKLP_Emul( QueTx = QueMasterTx, QueRx = QueMasterRx, Name = Name + '_Master' )
        SKLP_EmulSlave = SKLP_Emul( QueTx = QueMasterRx, QueRx = QueMasterTx, Name = Name + '_Slave' )
        return ( SKLP_EmulMaster, SKLP_EmulSlave )

    def __QueClear( Que ):
        while not Que.empty():
            Que.get()

    def _SKLP__SendFrame( self, Frame :bytearray ):
        Que = self.QueTx
        if ( Que != None ) and ( Frame != None ):
            SKLP_Emul.__QueClear( Que )
            Que.put_nowait( Frame )
            self.Status = '{}.SendFrame, {} bytes'.format( self.Name, len( Frame ) )

    def _SKLP__RecFrame( self, Timeout ) -> ( bytearray, int ):
        if Timeout == None:
            Timeout = self.Timeout
        Que = self.QueRx
        while True:
            if Que == None:
                break
            self.Status = '{}.RecFrame, wait {} s for Frame...'.format( self.Name, Timeout )
            try:
                Frame = Que.get( timeout = Timeout )
            except queue.Empty:
                self.Status += ' Timeout!'
                break
            FrameSize = len( Frame )
            if( FrameSize < 3 ):
                self.Status += ' Broken frame!'
                break
            if SKLP.CalcCRC8( Frame ) == 0:
                self.Status += ' Success!'
                self.LastRecFrame = Frame
                return Frame[ 2 : FrameSize-1 ], Frame[0]
            self.Status += ' Bad CRC!'
            break
        return None, None
 
# ********************************************
# ********************************************

class SKLP_Module:
    class Commands:
        GetID = 0x01
    class ID:
        def __init__( self, Address, Serial, DevType, Version ):
            self.Address    = Address
            self.Serial     = Serial
            self.DevType    = DevType
            self.Version    = Version
            
    def __init__( self, Address, Serial = 0, DevType = 0, Version = 0, Name = 'SKLP_Module', Interface :SKLP = None ):
        self.InterfaceDefault = Interface
        self.ID     = SKLP_Module.ID( Address, Serial, DevType, Version )
        self.Flag   = 0
        self.LogMsg = None
        self.Name   = Name
        
    def __CheckInterface( self, Interface :SKLP ) -> SKLP:
        if Interface != None:
            return Interface
        if self.InterfaceDefault != None:
            return self.InterfaceDefault
        raise ValueError( 'No Interface!' )
        
    def QueryCB( self, Address, Command, Data, Interface = None ):
        if Address != self.ID.Address:
            return
        Interface = self.__CheckInterface( Interface )
        if ( Command == self.Commands.GetID ) and ( len( Data ) == 0 ):
            Interface.Answer( pack( '=BLHL', self.Flag, self.ID.Serial, self.ID.DevType, self.ID.Version ) )
            
    def Query( self, Command, Data = bytearray( ), Interface = None, Timeout = None, RecAnswer = True ):
        Interface = self.__CheckInterface( Interface )
        return Interface.Query( self.ID.Address, Command, Data, Timeout = Timeout, RecAnswer = RecAnswer )
        
    def Query_GetID( self, Interface = None, Timeout = None ):
        Interface = self.__CheckInterface( Interface )
        AnswerPacket = self.Query( self.Commands.GetID, Interface = Interface, Timeout = Timeout )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == 1+4+2+4 ):
            ( self.Flag, self.ID.Serial, self.ID.DevType, self.ID.Version ) = unpack( '=BLHL', AnswerPacket[0:1+4+2+4] )
            self.LogMsg = '{}.GetID Success at 0x{:X}: Serial=0x{:X},\tDevType=0x{:X},\tVersion=0x{:X}'\
                            .format( Interface.Name, self.ID.Address, self.ID.Serial, self.ID.DevType, self.ID.Version )
            return True
        else:
            self.LogMsg = '{}.GetID Fail at 0x{:X}'.format( Interface.Name, self.ID.Address )
            return False
            
class SKLP_ModuleInclin( SKLP_Module ):
    class Commands:
        GetID           = 0x01
        GetDataMain     = 0x13
        GetDataCalibr   = 0x14
    class Axes:
        def __init__( self, Axes :tuple = ( 0, 0, 0, 0, 0, 0 ) ):
            ( self.BX, self.BY, self.BZ, self.GX, self.GY, self.GZ ) = Axes
            
        def ToTuple( self ) -> tuple:
            return ( self.BX, self.BY, self.BZ, self.GX, self.GY, self.GZ )
            
        def __mul__( self, Value ):
            return SKLP_ModuleInclin.Axes( ( x * Value ) for x in SKLP_ModuleInclin.Axes.ToTuple( self ) )
        
    class Angles:
        def __init__( self, Angles :tuple = ( 0, 0, 0, 0, 0 ) ):
            ( self.TFG, self.TFM, self.ZENI, self.ZENIGZ, self.AZIM ) = Angles
            
        def ToTuple( self ) -> tuple:
            return ( self.TFG, self.TFM, self.ZENI, self.ZENIGZ, self.AZIM )
            
        def __mul__( self, Value ):
            return SKLP_ModuleInclin.Angles( ( x * Value ) for x in SKLP_ModuleInclin.Angles.ToTuple( self ) )
        
    def __init__( self, Interface, Serial = 0, Version = 0 ):
        SKLP_Module.__init__( self, SKLP.AddressList.Inclin, Serial, 0x1710, Version, Name = 'Inclin', Interface = Interface )
        self.DIP = 0
        self.TOTB, self.TOTG, self.TMCU = ( 0, 0, 0 )   # [x/16+56->°C]
        self.Angles = self.Angles() # [x0.01->°]
        self.IAxes = self.Axes()    # [?,?]                     после АЦП
        self.TAxes = self.Axes()    # [?,?]                     после температурной калибровки
        self.GAxes = self.Axes()    # [x0.01?->uT, x0.0001->G]  после геометрической калибровки
        
    def QueryCB( self, Address, Command, Data ):
        if Address != self.ID.Address:
            return
        Interface = self.InterfaceDefault
        if ( Command == self.Commands.GetDataMain ) and ( len( Data ) == 0 ):
            Interface.Answer( pack( '5H', *self.Angles.ToTuple() ) + pack( '3H', self.DIP, self.TOTB, self.TOTG ) )
        elif ( Command == self.Commands.GetDataCalibr ) and ( len( Data ) == 0 ):
            Interface.Answer(
                    pack( '6h', *self.IAxes.ToTuple() ) +
                    pack( '6h', *self.TAxes.ToTuple() ) +
                    pack( '6h', *self.GAxes.ToTuple() ) +
                    pack( '3H', 0, 0, self.TMCU ) )
        else:
            SKLP_Module.QueryCB( self, Address, Command, Data )

    def Query_GetDataMain( self, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.GetDataMain, Timeout = Timeout )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == 8*2 ):
            self.Angles = SKLP_ModuleInclin.Angles( unpack( '5H', AnswerPacket[0:5*2] ) )
            ( self.DIP, self.TOTB, self.TOTG ) = unpack( '3H', AnswerPacket[5*2:8*2] )
            return True
        return False

    def Query_GetDataCalibr( self, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.GetDataCalibr, Timeout = Timeout )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == 3*6*2+3*2 ):
            Axes = SKLP_ModuleInclin.Axes
            self.IAxes = Axes( unpack( '6h', AnswerPacket[0*6*2:1*6*2] ) )
            self.TAxes = Axes( unpack( '6h', AnswerPacket[1*6*2:2*6*2] ) )
            self.GAxes = Axes( unpack( '6h', AnswerPacket[2*6*2:3*6*2] ) )
            ( self.TA, self.TM, self.TMCU ) = unpack( '3H', AnswerPacket[3*6*2:3*6*2+3*2] )
            return True
        return False

class SKLP_ModuleGK( SKLP_Module ):
    class Commands:
        GetID           = 0x01
        GetBlockMain    = 0x13
        GetBlockTech    = 0x14
        GetBlockAux     = 0x15
    ParseGetBlockMain = '=B3B2fHHlh'
        
    def __init__( self, Interface, Serial = 0, Version = 0 ):
        SKLP_Module.__init__( self, SKLP.AddressList.VIKPB_GK, Serial, 0xF000, Version, Name = 'Gamma', Interface = Interface )
        self.GRC = 0        # [pps]     счет приведенный ко времени накоплениЯ
        self.GR = 0         # [µR/h]    мгновенное значение естественной радиоактивности
        self.dt = 0         # [ms]      интервал времени измерениЯ
        self.UHV = 0        # [V]       напряжение высоковольтника
        self.Press_kPa = 0  # [kPa]     давление затрубное
        self.Temp = 0       # [°C]      температура
        
    def QueryCB( self, Address, Command, Data ):
        if Address != self.ID.Address:
            return
        Interface = self.InterfaceDefault
        if ( Command == self.Commands.GetBlockMain ) and ( len( Data ) == 0 ):
            Interface.Answer( pack( self.ParseGetBlockMain, 0, 0, 0, 0, self.GRC, self.GR, int( self.dt ), int( self.UHV ), int( self.Press_kPa ), int( self.Temp ) ) )
        else:
            SKLP_Module.QueryCB( self, Address, Command, Data )

    def Query_GetBlockMain( self, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.GetBlockMain, Timeout = Timeout )
        PacketSize = calcsize( self.ParseGetBlockMain )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            ( f0, id, f1, f2, self.GRC, self.GR, self.dt, self.UHV, self.Press_kPa, self.Temp ) = unpack( self.ParseGetBlockMain, AnswerPacket[0:PacketSize] )
            return True
        return False

class SKLP_Module_RUSPump( SKLP_Module ):
    class Commands:
        GetID       = 0x01
        ModeSet     = 0x11
        DataGet     = 0x13
    class Parse:
        AnDataGet       = '=BBffffffh'      # Flags, iMode, VBus, Power, MotorSpeedSet, MotorSpeed, PressSet, Press, DriveTemp
        QuModeSet       = '=B'              # iMode
        QuModeSetArg    = '=Bf'             # iMode, Argument (Speed, Press)
    dPumpModes = { 'Off': 0, 'Idle': 1, 'HoldSpeed': 2, 'HoldPress': 3 }
    class Constraints:
        PressMax                    = 1.5   # [МПа]
        MotorSpeedMin               = 1500  # [Об/мин]
        MotorSpeedMax               = 9000  # [Об/мин]
    class Data:                             # Data Packet from Command.DataGet
        def __init__( self ):
            self.Flags              = 0     # reserved
            self.iMode              = 0     # dPumpModes.values()
            self.VBus               = 0.0   # [В]
            self.MotorPower         = 0.0   # [Вт]
            self.MotorSpeedSetup    = 0.0   # [Об/мин]
            self.MotorSpeedCurrent  = 0.0   # [Об/мин]
            self.PressSetup         = 0.0   # [МПа]
            self.PressCurrent       = 0.0   # [МПа]
            self.TempDrive          = 0.0   # [°C]
        def PackForDataGet( self ):
            return pack( SKLP_Module_RUSPump.Parse.AnDataGet, self.Flags, self.iMode, self.VBus, self.MotorPower, self.MotorSpeedSetup, self.MotorSpeedCurrent, self.PressSetup, self.PressCurrent, self.TempDrive )
        def UnPackFromDataGet( self, Packet ):
            ( self.Flags, self.iMode, self.VBus, self.MotorPower, self.MotorSpeedSetup, self.MotorSpeedCurrent, self.PressSetup, self.PressCurrent, self.TempDrive ) = unpack( SKLP_Module_RUSPump.Parse.AnDataGet, Packet )

    def __init__( self, Interface, Order, Serial = 0, Version = 0 ):
        if Order == 0:      # TF = 0°
            Address = SKLP.AddressList.RUS_Pump0
            Name    = 'Pump   0°'
        elif Order == 1:    # TF = 120°
            Address = SKLP.AddressList.RUS_Pump1
            Name    = 'Pump 120°'
        elif Order == 2:    # TF = 240°
            Address = SKLP.AddressList.RUS_Pump2
            Name    = 'Pump 240°'
        elif Order == -1:   # Single Pump on Bus
            Address = SKLP.AddressList.RUS_Pump
            Name    = 'Pump Single'
        else:
            raise ValueError( 'Set order to -1, 0, 1 or 2!' )
        self.Order = Order
        self.Data = SKLP_Module_RUSPump.Data()
        SKLP_Module.__init__( self, Address, Serial, 0xF000, Version, Name = Name, Interface = Interface )

    def QueryCB( self, Address, Command, QueryPacket ):
        if Address != self.ID.Address:
            return
        Interface = self.InterfaceDefault
        if ( Command == self.Commands.DataGet ) and ( len( QueryPacket ) == 0 ):
            Interface.Answer( self.Data.PackForDataGet() )
        elif Command == self.Commands.ModeSet:
            if len( QueryPacket ) == calcsize( self.Parse.QuModeSet ):
                self.Data.iMode, = unpack( self.Parse.QuModeSet, QueryPacket[0:calcsize( self.Parse.QuModeSet )] )
                if self.Data.iMode == self.dPumpModes['Off']:
                    self.Data.PressSetup = 0
                    self.Data.MotorSpeedSetup = 0
            elif len( QueryPacket ) == calcsize( self.Parse.QuModeSetArg ):
                iMode, Arg = unpack( self.Parse.QuModeSetArg, QueryPacket[0:calcsize( self.Parse.QuModeSetArg )] )
                if iMode == self.dPumpModes['HoldSpeed']:
                    self.Data.MotorSpeedSetup = Arg
                    self.Data.iMode = iMode
                elif iMode == self.dPumpModes['HoldPress']:
                    self.Data.PressSetup = Arg
                    self.Data.iMode = iMode
        else:
            SKLP_Module.QueryCB( self, Address, Command, QueryPacket )

    def Query_DataGet( self, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.DataGet, Timeout = Timeout )
        PacketSize = calcsize( self.Parse.AnDataGet )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            self.Data.UnPackFromDataGet( AnswerPacket[0:PacketSize] )
            return True
        return False

    def Query_ModeSet( self, Mode, Arg = None, Timeout = None ):
        LogStr = f'{self.Name} set to Mode \"{Mode}\"'
        if Mode in ( 'Off', 'Idle' ):
            QueryPacket = pack( self.Parse.QuModeSet, self.dPumpModes[Mode] )
        elif Mode in ( 'HoldSpeed', 'HoldPress' ):
            QueryPacket = pack( self.Parse.QuModeSetArg, self.dPumpModes[Mode], Arg )
            LogStr += f', Arg = {Arg}'
        else:
            raise ValueError( f'Set Mode to { list( self.dPumpModes.keys() ) }!' )
        print( LogStr )
        self.Query( self.Commands.ModeSet, Data = QueryPacket, Timeout = Timeout, RecAnswer = False )
        return True

class SKLP_Module_RUSTele( SKLP_Module ):
    class Commands:
        GetID           = 0x01
        GetData         = 0x13      # считать основные данные
        SetHydro        = 0x60      # установить параметры через гидроканал
    ParseGetData = '=2B4Hh2B3BH'    # ответ на [0x13]
    ParseSetHydro = '=BB2H2BH'      # запрос [0x60], ответ как на [0x13]

    def __init__( self, Serial = 0, Version = 0, GateEnable = False ):
        SKLP_Module.__init__( self, SKLP.AddressList.RUS_Tele, Serial, 0x3500, Version, Name = 'RUS_Tele' )
        self.Flags1 = 0
        self.Flags2 = 0
        self.InclZENI = 0       # [x0.01->°]    зенитный угол от инклина
        self.InclTF = 0         # [x0.01->°]    угол отклонителя от инклина
        self.TarZENI = 0        # [x0.01->°]    целевой зенитный угол
        self.TarTF = 0          # [x0.01->°]    целевой угол отклонителя
        self.GenRate = 0        # [об/мин]      обороты генератора
        self.TarForceStatic = 0 # [0..1->0..FF] целевая сила распирания
        self.TarForceCurve = 0  # [0..1->0..FF] целевая сила кривления
        self.Force = ( 0, 0, 0 )# [0..1->0..FF] расчетные усилия на лапах
        self.GR = 0             # [pps]         счет модуля гаммы
        self.GuidanceMode = 0   # []            режим наведения
        self.PipeCurrent = 0    # []            номер текущей свечки
        if( GateEnable ):
            self.GateQueM2S = queue.Queue( maxsize = 1 )
            self.GateQueS2M = queue.Queue( maxsize = 1 )
            self.GateBuzy = False

    def QueryCB( self, Address, Command, Data, Interface :SKLP ):
        if Address != self.ID.Address:
            return
        if ( Command == self.Commands.GetData ) and ( len( Data ) == 0 ):
            Forces = ( ( int( f * 256 ) if f < 0.9999 else 255 ) for f in ( self.TarForceStatic, self.TarForceCurve, *self.Force ) )
            DataToPack = ( self.Flags1, self.Flags2, self.InclZENI, self.InclTF, self.TarZENI, self.TarTF, self.GenRate, *Forces, self.GR )
            try:
                Interface.Answer( pack( SKLP_Module_RUSTele.ParseGetData, *DataToPack ) )
            except struct.error as ex:
                print( 'Exception \"{}\" while packing {}'.format( ex, DataToPack ) )
        else:
            SKLP_Module.QueryCB( self, Address, Command, Data, Interface = Interface )

    def Query_GetData( self, Interface :SKLP, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.GetData, Interface = Interface, Timeout = Timeout )
        PacketSize = calcsize( self.ParseGetData )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            Forces = (0,)*5
            ( self.Flags1, self.Flags2, self.InclZENI, self.InclTF, self.TarZENI, self.TarTF, self.GenRate, *Forces, self.GR ) = unpack( self.ParseGetData, AnswerPacket[0:PacketSize] )
            self.TarForceStatic, self.TarForceCurve, *self.Force = ( f / 256 for f in Forces )
            return True
        return False

    def Query_SetHydro( self, Interface :SKLP, Timeout = None ):
        try:
            Forces = tuple( ( int( f * 256 ) if f < 0.9999 else 255 ) for f in ( self.TarForceStatic, self.TarForceCurve, *self.Force ) )
            DataToPack = ( 0xFF, self.GuidanceMode, self.TarZENI, self.TarTF, Forces[0], Forces[1], self.PipeCurrent )
            QueryPacket = pack( self.ParseSetHydro, *DataToPack )
        except struct.error as ex:
            print( 'Exception \"{}\" while packing {}'.format( ex, DataToPack ) )
        AnswerPacket = self.Query( self.Commands.SetHydro, Data = QueryPacket, Interface = Interface, Timeout = Timeout )
        PacketSize = calcsize( self.ParseGetData )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            Forces = (0,)*5
            ( self.Flags1, self.Flags2, self.InclZENI, self.InclTF, self.TarZENI, self.TarTF, self.GenRate, *Forces, self.GR ) = unpack( self.ParseGetData, AnswerPacket[0:PacketSize] )
            self.TarForceStatic, self.TarForceCurve, *self.Force = ( f / 256 for f in Forces )
            return True
        return False

import time
import numpy as np
class SKLP_Module_GGP_SADC( SKLP_Module ):
    ''' Протокол 0x6381 одноканального спектрометрического АЦП для ГГЛП
        Обмен по внутренней шине (без драйверов) с платой телеметрии на 460800,
        две платы подключены к разным интерфейсам и не разнесены по адресам.
        !! Добавлен тестовый протокол 0x6370 двухканальной платы без АЦП, только интегральные счета.
        !! Протокол вероятно будет пересмотрен.
        !! Реализация класса именно под 0x6370!
    '''
    class Commands:
        GetID           = 0x01
        SetMode         = 0x11
        GetDataMain     = 0x13
        GetDataTech     = 0x14
        GetSpectrum     = 0x15
        SetMode6370     = 0x21      # !! Test for 0x6370 protocol
        GetDataMain6370 = 0x23      # !! Test for 0x6370 protocol
    ParseGetDataMain6370    = '=BHHHBHH'
    ParseSetMode6370_Query  = '=BHH'
    ParseSetMode6370_Answer = '=B'
    Baud = 460800
        
    def __init__( self, Interface, Serial = 0, Version = 0 ):
        # !! Test for 0x6370 protocol
        SKLP_Module.__init__( self, SKLP.AddressList.GGLP_SADC, Serial, 0xF000, Version, Name = 'GGLP_SADC', Interface = Interface )
        self.Flag = 0           #       флаги
        self.dCntS = 0          # [p]   мгновенный счет по ближнему зонду за единицу времени
        self.dCntL = 0          # [p]   мгновенный счет по дальнему зонду за единицу времени
        self.dt = 0             # [ms]  дельта времени, за которую были накоплены мгновенные счета
        self.Temp = 0           # [°C]  температура
        self.RSD = 0            # [pps] счет по ближнему зонду за 1 с
        self.RLD = 0            # [pps] счет по дальнему зонду за 1 с
        self.Mode = 1           #       режим работы, 1 - разрешение
        self.VCompS = 0xFFFF    # [mV]  напряжение порога по ближнему зонду, 0xFFFF - Auto
        self.VCompL = 0xFFFF    # [mV]  напряжение порога по дальнему зонду, 0xFFFF - Auto
        self.GetDataMainTS = 0  # [s]   отметка времени последнего опроса
        self.Dispersion = 50    # [??]  дисперсия при эмуляции счета
        
    # Интерфес опроса слейва
    def Query_GetDataMain6370( self, Timeout = None ):
        AnswerPacket = self.Query( self.Commands.GetDataMain6370, Timeout = Timeout )
        PacketSize = calcsize( self.ParseGetDataMain6370 )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            Temp = 0
            ( self.Flag, self.dt, self.dCntS, self.dCntL, Temp, self.VCompS, self.VCompL ) = unpack( self.ParseGetDataMain6370, AnswerPacket[0:PacketSize] )
            self.Temp = Temp - 55.0
            return True
        return False

    def Query_SetMode6370( self, Timeout = None ):
        QueryPacket = pack( ParseSetMode6370_Query, self.Mode, self.VCompS, self.VCompSL )
        AnswerPacket = self.Query( self.Commands.SetMode6370, Data = QueryPacket, Timeout = Timeout )
        PacketSize = calcsize( self.ParseSetMode6370_Answer )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            ( self.Flag, ) = unpack( self.ParseSetMode6370_Answer, AnswerPacket[0:PacketSize] )
            return True
        return False

    # Интерфес ответов виртуального слейва
    def QueryCB( self, Address, Command, Data ):
        if Address != self.ID.Address:
            return
        if ( Command == self.Commands.GetDataMain6370 ) and ( len( Data ) == 0 ):
            Timestamp = time.time()
            dt = ( Timestamp - self.GetDataMainTS ) if ( self.GetDataMainTS != 0 ) else 0
            self.GetDataMainTS = Timestamp
            self.dt     = int( dt * 1000 )
            self.dCntS  = int( np.random.normal( self.RSD, self.Dispersion ) * dt )
            self.dCntL  = int( np.random.normal( self.RLD, self.Dispersion ) * dt )
            if self.dCntS < 0:
                self.dCntS = 0
            if self.dCntL < 0:
                self.dCntL = 0
            Temp = self.Temp + 55
            DataToPack = ( self.Flag, self.dt, self.dCntS, self.dCntL, Temp, self.VCompS, self.VCompL )
            Interface = self.InterfaceDefault
            try:
                Interface.Answer( pack( self.ParseGetDataMain6370, *DataToPack ) )
            except struct.error as ex:
                print( 'Exception \"{}\" while packing {}'.format( ex, DataToPack ) )
        else:
            SKLP_Module.QueryCB( self, Address, Command, Data, Interface = Interface )

import datetime
class SKLP_Module_MUP( SKLP_Module ):
    SectorsCount = 16
    HydroSectorsCount = 4
    BlockAzimSizeSD = 512           # размер азимутального блока на SD-карте
    class Commands:
        Reset           = 0x00
        GetID           = 0x01
        DataGet         = 0x13      # запросить данные
        DataMainSend    = 0x18      # отправить блок основных данных
        DataAzimSend    = 0x19      # отправить блок азимутальных данных
    class Parse:
        # Answers
        DataGet         = '=BLHBBhHBBBB'
        # DataMainSend    = DataGet
        DataAzimSend    = '=B'
        DataAzimSendTest    = '=B4B'            # Flag, RHOB[4]
        DataAzimSendTest2   = '=B4f4B'          # Flag, RHO[4], RHOB[4]
        # Queries
        DataMainSend    = '=B7B16BBHBB191B'     # ID, Time, ModulesFlags, FlagsAux, FlagsInfo, Mode, Protocol, Data
        DataAzimSign    = '=5B'                 # 'START'
        DataAzimHeader  = '=B7BLHB'             # ID, Time, ModulesCurrent, FlagsInfo, Mode
        DataAzimModHeader   = '=BH'             # Address, Size
        DataAzimACP     = '=BB64B14B'           # Flag1, Flag2, DataIntg, DataInst
        DataAzimGGP     = '=B136BHB'            # Flag, GammaDataTable, TF, Temp
        DataAzimGGPSect = '=HHBBH'              # RSD, RLD, RHOB, R, dT
        DataAzimBKS     = '=BBBB96B8B8B6BHlh'   # Flag, ID, Flag1, Flag2, ResistivityTbl, ElectroData, ButtonData, GenCoil, TF, Press, Temp
        DataAzimBKSSect = '=fH'                 # Rho, dT
    class AzimGGP:
        def __init__( self ):
            self.TF         = 0
            self.Temp       = 0
            self.aRLD       = [0]*SKLP_Module_MUP.SectorsCount
            self.aRSD       = [0]*SKLP_Module_MUP.SectorsCount
            self.aRHOB      = [0]*SKLP_Module_MUP.SectorsCount
            self.adT        = [0]*SKLP_Module_MUP.SectorsCount
            self.aRHOB_Hydro= [0]*SKLP_Module_MUP.HydroSectorsCount
            AzimModHeaderSize = calcsize( SKLP_Module_MUP.Parse.DataAzimModHeader )
            self.Packet     = bytearray( calcsize( SKLP_Module_MUP.Parse.DataAzimGGP ) + AzimModHeaderSize )
            self.Packet[0:AzimModHeaderSize] = pack( SKLP_Module_MUP.Parse.DataAzimModHeader, SKLP.AddressList.GGP, calcsize( SKLP_Module_MUP.Parse.DataAzimGGP ) )
    class AzimBKS:
        def __init__( self ):
            self.aRho       = [0]*SKLP_Module_MUP.SectorsCount
            self.adT        = [0]*SKLP_Module_MUP.SectorsCount
            self.aRho_Hydro = [0]*SKLP_Module_MUP.HydroSectorsCount
            AzimModHeaderSize = calcsize( SKLP_Module_MUP.Parse.DataAzimModHeader )
            self.Packet     = bytearray( calcsize( SKLP_Module_MUP.Parse.DataAzimBKS ) + AzimModHeaderSize )
            self.Packet[0:AzimModHeaderSize] = pack( SKLP_Module_MUP.Parse.DataAzimModHeader, SKLP.AddressList.BKS, calcsize( SKLP_Module_MUP.Parse.DataAzimBKS ) )

    def __init__( self, Interface, Serial = 0, Version = 0 ):
        SKLP_Module.__init__( self, SKLP.AddressList.MUP, Serial, 0xF000, Version, Name = 'MUP', Interface = Interface )
        AzimModHeaderSize = calcsize( self.Parse.DataAzimModHeader )
        # Default data
        self.Time       = (0,0,0,0,0,0,0)
        self.ModFlags   = (0xFF,0xFF,0xFF,0xFF)*4
        self.FlagsInfo  = 0x0301     # Reserved0, Flow1, Flow2???
        # Azim blocks
        self.AzimGGP = SKLP_Module_MUP.AzimGGP()
        self.AzimBKS = SKLP_Module_MUP.AzimBKS()

    def QueryCB( self, Address, Command, Data ):
        if Address != self.ID.Address:
            return
        SKLP_Module.QueryCB( self, Address, Command, Data )

    def Query_Reset( self, Interface :SKLP, Timeout = None ):                       # Выполнить команду сброса модуля
        print( f'{self.Name} Reset!' )
        self.Query( self.Commands.Reset, Interface = Interface, Timeout = Timeout )

    def Query_DataGet( self, Interface :SKLP, Timeout = None ):                     # Принять блок данных от МУП
        AnswerPacket = self.Query( self.Commands.DataGet, Interface = Interface, Timeout = Timeout )
        PacketSize = calcsize( self.Parse.DataGet )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            ( self.Flags, self.Modules, self.FInfo, self.Mode, self.Protocol, self.IPRS, self.FGen, self.VGen, self.VibroAP, self.VibroAS, self.Temp ) = unpack( self.Parse.DataGet, AnswerPacket[0:PacketSize] )
            return True
        return False

    def Query_DataMainSend( self, Interface :SKLP, Timeout = None ):                # Отправить основной блок в МУП (тест, устанавливаются только флаги модулей и режим работы)
        PacketSize = calcsize( self.Parse.DataMainSend )
        DataToSend = bytearray( PacketSize )
        DataToSend = pack( self.Parse.DataMainSend, 0, *self.Time, *self.ModFlags, 0, self.FlagsInfo, 6, 0, *((0,)*191) )
        
        AnswerPacket = self.Query( self.Commands.DataMainSend, Data = DataToSend, Interface = Interface, Timeout = Timeout )
        PacketSize = calcsize( self.Parse.DataGet )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            ( self.Flags, self.Modules, self.FInfo, self.Mode, self.Protocol, self.IPRS, self.FGen, self.VGen, self.VibroAP, self.VibroAS, self.Temp ) = unpack( self.Parse.DataGet, AnswerPacket[0:PacketSize] )
            return True
        return False

    def Query_DataAzimSendImage( self, Image, Interface :SKLP, Timeout = None ):    # Отправить азимутальный блок в МУП
        AnswerPacket = self.Query( self.Commands.DataAzimSend, Data = Image, Interface = Interface, Timeout = Timeout )
        PacketSize = calcsize( self.Parse.DataAzimSendTest2 )
        if ( AnswerPacket != None ) and ( len( AnswerPacket ) == PacketSize ):
            ( self.Flags, self.AzimBKS.aRho_Hydro[0], self.AzimBKS.aRho_Hydro[1], self.AzimBKS.aRho_Hydro[2], self.AzimBKS.aRho_Hydro[3], self.AzimGGP.aRHOB_Hydro[0], self.AzimGGP.aRHOB_Hydro[1], self.AzimGGP.aRHOB_Hydro[2], self.AzimGGP.aRHOB_Hydro[3] ) = unpack( self.Parse.DataAzimSendTest2, AnswerPacket[0:PacketSize] )
            return True
        return False

    def Query_DataAzimSend( self, Interface :SKLP, Timeout = None ):                # заполнить азимутальный блок по данным из self.AzimGGP, self.AzimBKS и т.п., отправить в МУП
        DataToSend = bytearray(512-5)
        AzimModHeaderSize = calcsize( self.Parse.DataAzimModHeader )
        # Fill AzimGGP
        Pos = calcsize( self.Parse.DataAzimHeader )
        Pos += AzimModHeaderSize + calcsize( self.Parse.DataAzimACP )
        SectorSize = calcsize( self.Parse.DataAzimGGPSect )
        for i in range( SKLP_Module_MUP.SectorsCount ):
            self.AzimGGP.Packet[ AzimModHeaderSize+1+i*SectorSize :] = pack( self.Parse.DataAzimGGPSect, int( self.AzimGGP.aRLD[i] ), int( self.AzimGGP.aRSD[i] ), int( self.AzimGGP.aRHOB[i] ), i*17, self.AzimGGP.adT[i] )   # replace 0x1111 -> RSD, 0x2222 -> RLD, ii -> R
        DataToSend[Pos:Pos+len(self.AzimGGP.Packet)] = self.AzimGGP.Packet
        # Fill AzimBKS
        Pos = calcsize( self.Parse.DataAzimHeader )
        Pos += AzimModHeaderSize + calcsize( self.Parse.DataAzimACP )
        Pos += AzimModHeaderSize + calcsize( self.Parse.DataAzimGGP )
        SectorSize = calcsize( self.Parse.DataAzimBKSSect )
        for i in range( SKLP_Module_MUP.SectorsCount ):
            self.AzimBKS.Packet[ AzimModHeaderSize+4+i*SectorSize :] = pack( self.Parse.DataAzimBKSSect, self.AzimBKS.aRho[i], self.AzimBKS.adT[i] )
        DataToSend[Pos:Pos+len(self.AzimBKS.Packet)] = self.AzimBKS.Packet

        return self.Query_DataAzimSendImage( Image = DataToSend, Interface = Interface, Timeout = Timeout )

    def ParseAzimBlock( self, BlockSD ):    # Распарсить азимутальный блок в данные ГГП, БКС и т.п.
        BlockLenReq = SKLP_Module_MUP.BlockAzimSizeSD
        BlockLenInput = len( BlockSD )
        if( BlockLenInput != BlockLenReq ):
            raise ValueError( f'Block size must be {BlockLenReq} B, not {BlockLenInput} B!' )
        SignatureReq = b'START'
        SignatureInput = BlockSD[0:5]
        if SignatureInput != SignatureReq:
            raise ValueError( f'Block signature must be \"{SignatureReq}\", not {SignatureInput}!' )
            

        Parse = SKLP_Module_MUP.Parse
        AzimModHeaderSize = calcsize( Parse.DataAzimModHeader )
        AzimHeaderSize  = calcsize( Parse.DataAzimHeader )
        AzimOffsetHeader= calcsize( Parse.DataAzimSign )
        AzimOffsetAKP   = AzimOffsetHeader + AzimHeaderSize
        AzimOffsetGGP   = AzimOffsetAKP + AzimModHeaderSize + calcsize( Parse.DataAzimACP )
        AzimOffsetBKS   = AzimOffsetGGP + AzimModHeaderSize + calcsize( Parse.DataAzimGGP )
        AzimOffsetGK    = AzimOffsetGGP + AzimModHeaderSize + calcsize( Parse.DataAzimBKS )

        aDT = [0]*7     # YY,MM,DD,hh,mm,ss,zz
        ( ID, *aDT, ModulesCurrent, FlagsInfo, Mode ) = unpack( Parse.DataAzimHeader, BlockSD[AzimOffsetHeader:AzimOffsetHeader+AzimHeaderSize] )
        self.AzimDateTime = datetime.datetime( aDT[0]+2000, aDT[1], aDT[2], aDT[3], aDT[4], aDT[5], aDT[6]*10000 )

        for ( Address, Size, Offset ) in [
                ( SKLP.AddressList.GGP, calcsize( Parse.DataAzimGGP ), AzimOffsetGGP ),
                ( SKLP.AddressList.BKS, calcsize( Parse.DataAzimBKS ), AzimOffsetBKS ) ]:
            ( PackAddress, PackSize ) = unpack( Parse.DataAzimModHeader, BlockSD[Offset:Offset+AzimModHeaderSize] )
            if PackAddress == 0xFF and PackSize == 0xFFFF:
                raise ValueError( f'Address = 0x{PackAddress:02X}, Size = 0x{PackSize:04X}' )
            if PackAddress == Address and PackSize == Size:
                if Address == SKLP.AddressList.GGP:
                    Dest = self.AzimGGP
                    Packet = BlockSD[Offset+AzimModHeaderSize:Offset+AzimModHeaderSize+Size]
                    DataTable = []
                    ( Flag, *DataTable, Dest.TF, Dest.Temp ) = unpack( Parse.DataAzimGGP, Packet )
                    SectorSize = calcsize( Parse.DataAzimGGPSect )
                    for i in range( SKLP_Module_MUP.SectorsCount ):
                        ( Dest.aRSD[i], Dest.aRLD[i], Dest.aRHOB[i], R, Dest.adT[i] ) = unpack( Parse.DataAzimGGPSect, Packet[1+i*SectorSize:1+(i+1)*SectorSize] )
                    print( f'TF = {Dest.TF}°\tRHOB = {Dest.aRHOB}\tdT = {Dest.adT}' )
                elif Address == SKLP.AddressList.BKS:
                    Dest = self.AzimBKS
                    Packet = BlockSD[Offset+AzimModHeaderSize:Offset+AzimModHeaderSize+Size]
                    SectorSize = calcsize( Parse.DataAzimBKSSect )
                    for i in range( SKLP_Module_MUP.SectorsCount ):
                        ( Dest.aRho[i], Dest.adT[i] ) = unpack( Parse.DataAzimBKSSect, Packet[4+i*SectorSize:4+(i+1)*SectorSize] )
                    print( f'TF = {0}°\tRho = {Dest.aRho}\tdT = {Dest.adT}' )
            else:
                if Address == SKLP.AddressList.GGP:
                    self.AzimGGP = SKLP_Module_MUP.AzimGGP()
                elif Address == SKLP.AddressList.BKS:
                    self.AzimBKS = SKLP_Module_MUP.AzimBKS()
            
