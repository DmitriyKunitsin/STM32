# Run from Notepad++ by F5:
# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

''' Моделирование регулятора '''

import matplotlib.pyplot as plt
from math import sin

def IPRS( t ):
    return 1

def OPRS( t ):
    return 0

def Flow( Press, R ):
    return Press / R

def Press( Flow, R ):
    return Flow * R

Rmin = 1                        # сопротивление вентиля минимальное
Rmax = 10                       # сопротивление вентиля максимальное
R1 = 0                          # сопротивление перед вентилем
R2 = 0                          # сопротивление между вентилем и цилиндром
R3 = Rmax/2                     # сопротивление байпаса цилиндра
Vmax = 10                       # полный заполненный объем цилиндра
Vmin = 0                        # опустошенный объем цилиндра
S = 1                           # площадь цилиндра
dt = 1.0                        # время на один такт вычислений
k = 1                           # пропорциональность между давлением и скоростью цилиндра
Fext0 = 0.5                     # сила противодействия цилиндру (пружина)

def R12( R ):
    return R1 + R + R2
def R123( R ):
    return R12( R ) + R3
def vCyl( P1, P2, V ):        # скорость перемещения цилиндра, прямопрорциональна усилию, обнуляется на границах
    v = ( P1 - P2 ) * k
    if ( ( V < Vmin ) and ( v < 0 ) ) or ( ( V > Vmax ) and ( v > 0 ) ):
        return 0
    else:
        return v

Counts = 400
at      = [None] * Counts       # время
aIPRS   = [None] * Counts       # давление внутритрубное
aOPRS   = [None] * Counts       # давление затрубное
aR      = [None] * Counts       # сопротивление вентиля
aFlowIn = [None] * Counts       # поток из внутритрубъя
aFlowOut= [None] * Counts       # поток в затрубье
aV      = [None] * Counts       # объем цилиндра заполненный
aPV     = [None] * Counts       # давление в цилиндре
av      = [None] * Counts       # скорость цилиндра
aV[0] = ( Vmax - Vmin ) / 2
av[0] = 0
for i in range( Counts ):

    at[i] = i * dt
    aIPRS[i] = IPRS( at[i] )
    aOPRS[i] = OPRS( at[i] )
    # aR[i] = Rmin
    # aR[i] = Rmin + ( Rmax-Rmin ) / 1.5
    # aR[i] = Rmin + ( Rmax-Rmin ) * ( i / ( Counts - 1 ) )
    aR[i] = ( Rmax-Rmin ) / 2 * sin( i * 2 * 3.1415 / ( Counts / 5 ) ) + ( Rmax+Rmin ) / 2
    F = Fext0

    if i > 0:
        v = av[i-1]
        # if aV[i-1] > ( Vmin + ( Vmax - Vmin ) * 0.2 ):
            # F = Fext0 + 0.5 * Fext0
    else:
        v = av[0]
    if v != 0:
        aPV[i] = ( aIPRS[i] / R12( aR[i] ) + aOPRS[i] / R3 + ( aOPRS[i] + F / S ) * k * S ) / ( 1 / R12( aR[i] ) + 1 / R3 + k * S )
    else:
        aPV[i] = ( aIPRS[i] * R3 - aOPRS[i] * R12( aR[i] ) ) / R123( aR[i] )
    aFlowIn[i] = Flow( aIPRS[i] - aPV[i], R12( aR[i] ) )
    aFlowOut[i] = Flow( aPV[i] - aOPRS[i], R3 )

    if i > 0:
        aV[i] = aV[i-1] + av[i-1] * S * dt
        av[i] = vCyl( aPV[i], aOPRS[i] + F / S, aV[i] )
    
# print( f'Time:\t{ at }' )
# print( f'IPRS:\t{ aIPRS }' )
# print( f'OPRS:\t{ aOPRS }' )
# print( f'R:\t{ aR }' )
# print( f'FlowIn:\t{ aFlowIn }' )
# print( f'FlowOut:\t{ aFlowOut }' )
# print( f'PressV:\t{ aPV }' )
# print( f'VCyl:\t{ aV }' )
# print( f'vCyl:\t{ av }' )

Fig, (ar,ax,ay,az) = plt.subplots( nrows = 4, ncols = 1, figsize=( 6, 4 ), num = 'Valve Modelling', sharex=True )
ar.plot( at, aR, linestyle='solid', marker=".", markersize=2 )
ax.plot( at, aFlowIn, label = 'In', linestyle='solid', marker=".", markersize=2 )
ax.plot( at, aFlowOut, label = 'Out', linestyle='solid', marker=".", markersize=2 )
ax.plot( at, av*S, label = 'Cylinder', linestyle='solid', marker=".", markersize=2 )
ay.plot( at, aV, linestyle='solid', marker=".", markersize=2 )
az.plot( at, aPV, linestyle='solid', marker=".", markersize=2 )
ar.set_ylabel( 'Valve' )
ax.set_ylabel( 'Flow' )
ay.set_ylabel( 'Position' )
az.set_ylabel( 'Press' )
ax.legend()
plt.draw()
plt.pause( 0.1 )

print( '\n\nPress \'Enter\' to exit... ')
input()
quit()
