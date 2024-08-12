# Run from Notepad++:
# cmd /K cd "$(CURRENT_DIRECTORY)" && "%LOOCH_PYTHON_DIR%"\python "$(FULL_CURRENT_PATH)"

from math import sqrt, pi

g = 9.81    # [m/s^2]

# Условная вязкость промывочной жидкости
vu_min = 30     # [s]
vu_max = 150    # [s]

# Вязкость кинематическая промывочной жидкости
''' Расчет кинематической вязкости
- vu [s]    условная вязкость
'''
def v_vu( vu ):     # [m^2/s],[Pa*s]
    v = 7.4e-6 * vu
    print( f'v_vu({vu}) = {v} [m^2/s]' )
    return v

# v_min = 30  # [s]
# v_max = 130 # [s]

# Разность давлений IPRS-OPRS в управляемом диапазоне
dP_min = 1  # [MPa]
dP_max = 5  # [MPa]

''' Расчет числа Рейонльдса
- V [m/s]   скорость потока жидкости
- D [m]     диаметр трубы
- v         кинематическая вязкость
'''
def Re_VDv( V, D, v ):
    Re = V * D / v
    print( f'Re( {V}, {D}, {v}) = {Re}')
    return Re

''' Расчет скорости потока жидкости
- L [m]     длина трубы
- D [m]     диаметр трубы
- dP [m]    падение давления
- k         коэффициент гидравлического трения
'''
def V_LDPk( L, D, dP, k ):      # [m/s]
    V = sqrt( dP * D * 2 * g / k / L )
    print( f'V( {L}, {D}, {dP}, {k}) = {V} [m/s]')
    return V
    
''' Расчет перепада давления
- L [m]     длина трубы
- D [m]     диаметр трубы
- V [m/s]   скорости потока жидкости
- k         коэффициент гидравлического трения
'''
def dP_LDVk( L, D, V, k ):      # [m]
    dP = k * L * V * V / D / 2 / g
    print( f'dP( {L}, {D}, {V}, {k}) = {dP} [m]')
    return dP
    
''' Расчет потока через сечение
- V [m/s]   скорость потока жидкости
- D [m]     диаметр трубы
'''
def W_VD( V, D ):       # [m^3/s]
    r = D/2
    W = V * pi * r * r
    print( f'W( {V}, {D}) = {W} [m^3/s]')
    return W

''' Расчет скорость потока жидкости
- V [m/s]   скорость потока жидкости
- D [m]     диаметр трубы
'''
def V_WD( W, D ):       # [m/s]
    r = D/2
    V = W / ( pi * r * r )
    print( f'V( {W}, {D}) = {V} [m/s]')
    return V

# vu = vu_min
# Re = 1400

# vu = vu_max
# Re = 50

vu = (vu_max+vu_min)/2
Re = 155

v = v_vu(vu)

k = 70 / Re

dP = 10 * 10    # 10 bar, 100 m
L = 0.05
D = 0.005
V = V_LDPk( L, D, dP, k )
Re_VDv( V, D, v )

# Участок L = 12 mm, D = 6 mm
# Участок L = 15 mm, D = 4 mm
# Участок L = 27 mm, D = 5 mm
# IPRS-OPRS = 10 bar
# Какой поток? Какие падения давления?

# Рачет по участку L = 50 mm, D = 5 mm дает V = 20 m/s - около 0.4 л/с
W = W_VD( V, D )
V = V_WD( W, D )
dP = dP_LDVk( L, D, V, k )

L1, D1 = 0.012, 0.006
L2, D2 = 0.015, 0.004
L3, D3 = 0.027, 0.005
L4, D4 = 0.005, 0.002 * sqrt( 2 )
L5, D5 = 0.008, 0.004 * sqrt( 2 )

dP_Summ = 0
for ( Ln, Dn ) in [ ( L1, D1 ), ( L2, D2 ), ( L3, D3 ), ( L4, D4 ), ( L5, D5 ), ]:
    print()
    Vn = V_WD( W, Dn )
    dPn = dP_LDVk( Ln, Dn, Vn, k )
    Re_VDv( Vn, Dn, v )
    dP_Summ += dPn

print( f'dP_Summ = {dP_Summ} [m]')
