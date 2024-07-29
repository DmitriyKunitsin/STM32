# Run from Notepad++ by F5:
# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

''' Моделирование регулятора '''

import matplotlib.pyplot as plt
from math import sin, pi
import numpy as np

''' Функции, связывающие перепад давления на участке трубы, поток жидкости и сопротивление трубы
!!! Закон должен быть квадратичным при ламинарном потоке, не говоря уже о турбулентном!
!!! Ламинарность/турбулентность определяется по соотношению скорости потока и диаметра трубок?
!!! Должна быть существенная зависимость от вязкости жидкости, которая в т.ч. зависит от температуры!
'''
def Flow( Press, R ):       # функция расчета потока через участок трубы в зависимости от перепада давления и сопротивления
    return Press / R
def Press( Flow, R ):       # функция расчета перепада давления на участке трубы по потоку и сопротивлению
    return Flow * R

''' Модель гидросистемы:
R1  - сопротивление между IPRS и вентилем (константа)
R   - сопротивление вентиля, изменияется при вращении вала регулятора от Min до Max, закон зависит от конструкции заслонки
R2  - сопротивление между вентилем и подцилиндровым объемом (константа)
R3  - сопротивление между подцилиндровым объемом и OPRS (константа)
h   - ход поршня (он же ход лапы), изменяется при выдвижении поршня от Min до Max
S   - площадь поршня
V   - объем цилиндра, изменяется при выдвижении поршня от Min до Max
'''
Rmin = 1                    # сопротивление вентиля минимальное
Rmax = 10                   # сопротивление вентиля максимальное
R1 = 0                      # сопротивление перед вентилем
R2 = 0                      # сопротивление между вентилем и цилиндром
R3 = Rmax/2                 # сопротивление байпаса цилиндра
S = 1                       # площадь поршня
H = 10                      # ход поршня
k = 1                       # пропорциональность между разницей давлений на поршне и скоростью перемещения поршня

def R( Shaft ):             # расчет сопротивления на вентиле в зависимости от положения вала
    # Константа
    # return Rmin
    # return ( Rmin + Rmax ) / 2
    # Односинусоидальный профиль перекрытия
    return ( Rmax - Rmin ) / 2 * sin( Shaft ) + ( Rmax + Rmin ) / 2
def R12( R ):               # расчет сопротивления между IPRS и цилиндром 
    return R1 + R + R2
def R123( R ):              # расчет сопротивления между IPRS и OPRS
    return R12( R ) + R3
def vPiston( P1, P2, h ):   # скорость перемещения поршня: прямопропорциональна разнице давлений, обнуляется на границах цилиндра
    v = ( P1 - P2 ) * k
    if ( ( h < 0 ) and ( v < 0 ) ) or ( ( h > H ) and ( v > 0 ) ):
        return 0
    else:
        return v

# Константы моделирования
ShaftSpeedRPM_Max = 0.5         # [RPM] максимальная скорость вращения вала вентиля
# CntPeriod = 100                 # количество циклов расчета на период управляющего воздействия
# Periods = 5                     # количество периодов на всю симуляцию
# Counts = CntPeriod*Periods      # количество циклов расчета на всю симуляцию
Periods = 4                     # количество периодов вращения вала на всю симуляцию
Counts = int( ( Periods * 60 ) // ShaftSpeedRPM_Max )                   # количество циклов расчета на всю симуляцию
dt      = 1.0                   # время на один такт вычислений
Fext0   = 0.5                   # сила противодействия цилиндру (пружина)
at      = [None] * Counts       # время
aIPRS   = [None] * Counts       # давление внутритрубное
aOPRS   = [None] * Counts       # давление затрубное
aR      = [None] * Counts       # сопротивление вентиля
aFlowIn = [None] * Counts       # поток из внутритрубъя
aFlowOut= [None] * Counts       # поток в затрубье
ah      = [None] * Counts       # положение поршня/лапы
aPV     = [None] * Counts       # давление в цилиндре
av      = [None] * Counts       # скорость поршня
aShaft  = [None] * Counts       # положение вала вентиля
ah[0] = H / 2                   # начальное положение поршня
av[0] = 0
Phi0 = pi/2                     # [rad] начальное положение вала заслонки
Phi0 = pi*1.0                     # [rad] начальное положение вала заслонки

# Законы моделирования
def OPRS( i ):              # закон изменения затрубного давления (принято за ноль)
    return 0

def IPRS( i ):              # закон изменения внутритрубного давления (зависит от потока, и режима долота - над забоем, на забое и с какой разгрузкой)
    # Константа
    # return 0.85
    IPRS_Start = 0.5
    IPRS_Finish = 2.0
    # Плавное наростание Start->Finish за цикл симуляции
    # return IPRS_Start + i * ( IPRS_Finish - IPRS_Start ) / Counts
    # Плавное наростание Start->Finish за цикл симуляции, с подмешиванием синусоидального шума
    return IPRS_Start + i * ( IPRS_Finish - IPRS_Start ) / Counts + ( IPRS_Finish - IPRS_Start ) / 5 * sin( i )

def ShaftSpeedRPM( i ):     # закон изменения скорости вала
    # Константа
    return ShaftSpeedRPM_Max
    # if i > 100:
        # return ShaftSpeedRPM_Max
    # if i > 50:
        # return ShaftSpeedRPM_Max/2
    # else:
        # return 0
    
def Shaft( i ):             # закон изменения положения вала
    # Константа
    # return Rmin
    # return ( Rmin + Rmax ) / 2
    # return i * 2 * pi / CntPeriod + Phi0
    if i > 0:
        return aShaft[i-1] + ShaftSpeedRPM( i ) * 2 * pi * dt / 60
    else:
        return Phi0

def Fext( i ):              # закон изменения внешнего усилия на поршне
    # Константа, только встроенная пружина
    return Fext0

# Моделирование
for i in range( Counts ):
    at[i] = i * dt
    aIPRS[i] = IPRS( i )
    aOPRS[i] = OPRS( i )
    aShaft[i] = Shaft( i )
    aR[i] = R( aShaft[i] )
    F = Fext( i )

    if i > 0:
        v = av[i-1]
        # if aV[i-1] > ( Vmin + ( Vmax - Vmin ) * 0.2 ):
            # F = Fext0 + 0.5 * Fext0
    else:
        v = av[0]
    # Расчет давления в цилиндре
    if v != 0:
        # Поршень двигается — входящий поток разделен на исходящий поток и поток в цилиндр
        aPV[i] = ( aIPRS[i] / R12( aR[i] ) + aOPRS[i] / R3 + ( aOPRS[i] + F / S ) * k * S ) / ( 1 / R12( aR[i] ) + 1 / R3 + k * S )
    else:
        # Поршень не двигается — входящий поток равен исходящему потоку
        aPV[i] = ( aIPRS[i] * R3 - aOPRS[i] * R12( aR[i] ) ) / R123( aR[i] )
    # Расчет потоков по давлениям
    aFlowIn[i] = Flow( aIPRS[i] - aPV[i], R12( aR[i] ) )
    aFlowOut[i] = Flow( aPV[i] - aOPRS[i], R3 )

    if i > 0:
        ah[i] = ah[i-1] + av[i-1] * dt
        av[i] = vPiston( aPV[i], aOPRS[i] + F / S, ah[i] )
    
Fig, (ash, ar,ax,ay,az) = plt.subplots( nrows = 5, ncols = 1, figsize=( 6, 8 ), num = f'Valve Modelling {Phi0}°', sharex=True )
ash.plot( at, aShaft,   label = 'Angle',        linestyle='solid', marker=".", markersize=2 )
ar.plot( at, aR,        label = 'Resistance',   linestyle='solid', marker=".", markersize=2 )
ax.plot( at, aFlowIn,   label = 'In',           linestyle='solid', marker=".", markersize=2 )
ax.plot( at, aFlowOut,  label = 'Out',          linestyle='solid', marker=".", markersize=2 )
ax.plot( at, av*S,      label = 'Cylinder',     linestyle='solid', marker=".", markersize=2 )
ay.plot( at, ah,        label = 'Piston Pos',   linestyle='solid', marker=".", markersize=2 )
az.plot( at, aIPRS,     label = 'IPRS',         linestyle='solid', marker=".", markersize=2 )
az.plot( at, aOPRS,     label = 'OPRS',         linestyle='solid', marker=".", markersize=2 )
az.plot( at, aPV,       label = 'Cylinder',     linestyle='solid', marker=".", markersize=2 )

# lags = az.xcorr( aR, aPV, normed=True, maxlags=None )
# npR = np.array( aR )
# npR -= np.average( npR )
# npR /= 10
# npPV = np.array( aPV )
# npPV -= np.average( npPV )
# npPV *= 2
# az.plot( at, npR, linestyle='solid', marker=".", markersize=2 )
# az.plot( at, npPV, linestyle='solid', marker=".", markersize=2 )
# c = np.correlate( npR, npPV, mode = 'same' )
# az.plot( at, c[0:len(at)]/20, linestyle='none', marker=".", markersize=2 )
# print( c )

npPV = np.array( aPV )
npPV -= np.average( npPV )
# npPV *= 2
# npR = np.array( aR )
# npR -= np.average( npR )
# npR /= 10
ShaftPeriodCnt = int( 60 / ShaftSpeedRPM_Max )
npSine = np.array( [ sin( 2 * pi * i / ShaftPeriodCnt ) for i in range( Counts ) ] )
ar.plot( at, npSine,        label = 'Sine',   linestyle='solid', marker=".", markersize=2 )
# c = np.correlate( npR, npPV, mode = 'valid' )
# c = np.correlate( npSine, npPV, mode = 'valid' )
# print( c )
aCorr = []
for i in range( ShaftPeriodCnt ):
    # c = np.correlate( np.roll( npR, i ), npPV, mode = 'valid' )
    c = np.correlate( np.roll( npSine, i ), npPV, mode = 'valid' )
    # print( c )
    aCorr.append( c )
    
# for (i,phi) in [(0,0), (30,90), (60, 180), (90,240) ]:
    # cr =  np.correlate( np.roll( npSine, i ), npPV, mode = 'same' )
    # az.plot( at, cr, label = f'Corr {phi}', linestyle='solid', marker=".", markersize=2 )
    # print(  f'Corr {phi}: { sum( cr )}')

# for i in range( ShaftPeriodCnt ):
    # cr =  np.correlate( np.roll( npSine, i ), npPV, mode = 'same' )
    # print(  f'Corr {i/ShaftPeriodCnt*360}°: { sum( cr )}')

fff = np.fft.rfft( npPV )

# az.plot( at, np.correlate( np.roll( npSine, 0 ), npPV, mode = 'same' ),       label = 'Corr0',     linestyle='solid', marker=".", markersize=2 )
# az.plot( at, np.correlate( np.roll( npSine, 120//4 ), npPV, mode = 'same' ),       label = 'Corr90',     linestyle='solid', marker=".", markersize=2 )
# az.plot( at, np.correlate( np.roll( npSine, 120//2 ), npPV, mode = 'same' ),       label = 'Corr180',     linestyle='solid', marker=".", markersize=2 )

Fig2, acc = plt.subplots( nrows = 1, ncols = 1, figsize=( 6, 4 ), num = f'Correlation {Phi0}°' )
# acc.plot( np.array( range(ShaftPeriodCnt) ) / ShaftPeriodCnt * 360, aCorr, linestyle='solid', marker=".", markersize=2 )
# acc.plot( npSine, linestyle='solid', marker=".", markersize=2 )
fff = fff[0:30]
acc.plot( fff.real, linestyle='solid', marker=".", markersize=2 )
acc.plot( fff.imag, linestyle='solid', marker=".", markersize=2 )
acc.plot( np.absolute(fff), linestyle='solid', marker=".", markersize=2 )
acc.plot( np.angle(fff), linestyle='solid', marker=".", markersize=2 )
print( np.angle(fff)[4]*180/pi)

ash.set_ylabel( 'Shaft [rad]' )
ar.set_ylabel( 'Valve' )
ax.set_ylabel( 'Flow' )
ay.set_ylabel( 'Position' )
az.set_ylabel( 'Press' )
ax.legend()
az.legend()
acc.xaxis.grid()
Fig.tight_layout()
plt.draw()
plt.pause( 0.1 )

print( '\n\nPress \'Enter\' to exit... ')
input()
quit()
