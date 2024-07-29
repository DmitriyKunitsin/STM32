import tkinter as tk
from tkinter import *
import tkinter.ttk as ttk
from tkinter.messagebox import *\


# Создание главного окна

window = Tk()
window.geometry('1100x800')
window.title("Отладка регулятора РУС")

tab_control = ttk.Notebook(window)
tab_regul = tk.Frame(tab_control, bg = 'white')
tab_regul_debug = tk.Frame(tab_control, bg = 'white')
tab_control.add(tab_regul, text = 'Регулятор')
tab_control.add(tab_regul_debug, text = 'Отладка по УАРТ')
tab_control.pack(expand = 1, fill = 'both')
 
style = ttk.Style()
style.configure('All.TButton', font = ("Times New Roman Bold", 16), width = 25)

StartDebugWait = ttk.Button(
    tab_regul_debug,
    text = "Старт",
    style = 'All.TButton'
)
StartDebugWait.grid(column = 0, row = 0, columnspan = 1, rowspan = 1, padx = 10, pady = 10, sticky = W)

StartMeasureButton = ttk.Button(
    tab_regul,
    text = "Начать измерения",
    style = 'All.TButton',
)
StartMeasureButton.grid(column = 0, row = 0, columnspan = 2, rowspan = 1, padx = 10, pady = 5, sticky = W)

GetID = ttk.Button(
    tab_regul,
    text = "Get ID",
    style = 'All.TButton'
)
GetID.grid(column = 0, row = 1, columnspan = 2, rowspan = 1, padx = 10, pady = 5, sticky = W)

PowerMotorOn = ttk.Button(
    tab_regul,
    text = "Подать питание на мотор",
    style = 'All.TButton',
)
PowerMotorOn.grid(column = 0, row = 2, columnspan = 2, rowspan = 1, padx = 10, pady = 5, sticky = W)

PowerOffMotor = ttk.Button(
    tab_regul,
    text = "Снять питание с мотора",
    style = 'All.TButton'
)
PowerOffMotor.grid(column = 0, row = 3, columnspan = 2, rowspan = 1, padx = 10, pady = 5, sticky = W)

SetSpeed = ttk.Button(
    tab_regul, 
    text = "Установить скорость",
    style = 'All.TButton'
)
SetSpeed.grid(column = 0, row = 4, columnspan = 2, rowspan = 1, padx = 10, pady = 5, sticky = W)

setspeed_input = ttk.Entry(
    tab_regul,
    font = ("Arial Bold", 16),
    width = 5,
)
setspeed_input.grid(column = 1, row = 4, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

setspeed_text = ttk.Label(
    tab_regul,
    text = "Скорость [-100:100]",
    background = 'white',
    font = ("Arial Bold", 16)
)
setspeed_text.grid(column = 2, row = 4, columnspan = 1, rowspan = 1, padx = 0, pady = 5, sticky = W)

SetSpeedAndAngle = ttk.Button(
    tab_regul,
    text = "Установка скорости и угла",
    style = 'All.TButton'
)
SetSpeedAndAngle.grid(column = 0, row = 5, columnspan = 1, rowspan = 2, padx = 10, pady = 5, ipady = 20, sticky = W)

SetSpeedAndAngle_input_speed = ttk.Entry(
    tab_regul,
    font = ("Arial Bold", 16),
    width = 5
)
SetSpeedAndAngle_input_speed.grid(column = 1, row = 5, columnspan = 1, rowspan = 1, padx = 10, pady = 0, sticky = W)

SetSpeedAndAngle_input_Angle = ttk.Entry(
    tab_regul,
    font = ("Arial Bold", 16),
    width = 5
)
SetSpeedAndAngle_input_Angle.grid(column = 1, row = 6, columnspan = 1, rowspan = 1, padx = 10, pady = 0, sticky = W)

SetSpeedAndAngle_input_speed_label = ttk.Label(
    tab_regul,
    text = 'Скорость [0:100]',
    font = ("Arial Bold", 16),
    background = 'white'
)
SetSpeedAndAngle_input_speed_label.grid(column = 2, row = 5, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

SetSpeedAndAngle_input_Angle_label = ttk.Label(
    tab_regul,
    text = 'Угол',
    font = ("Arial Bold", 16),
    background = 'white',
)
SetSpeedAndAngle_input_Angle_label.grid(column = 2, row = 6, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

StartCalibrationConsumption = ttk.Button(
    tab_regul,
    text = "Калибровка по потреблению",
    style = 'All.TButton'
)
StartCalibrationConsumption.grid(column = 0, row = 7, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

StartCalibrationPressure = ttk.Button(
    tab_regul,
    text = "Калибровка по давлению",
    style = 'All.TButton'
)
StartCalibrationPressure.grid(column = 0, row = 8, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

Overlap_Set_Button = ttk.Button(
    tab_regul,
    text = "Процент перекрытия",
    style = 'All.TButton'
)
Overlap_Set_Button.grid(column = 0, row = 9, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

Overlap_Set_Entry = ttk.Entry(
    tab_regul,
    font = ("Arial Bold", 16),
    width = 5
)
Overlap_Set_Entry.grid(column = 1, row = 9, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

Overlap_Set_Label = ttk.Label(
    tab_regul,
    text = '[0 : 100%]',
    font = ("Arial Bold", 16),
    background = 'white',
)
Overlap_Set_Label.grid(column = 2, row = 9, columnspan = 1, rowspan = 1, padx = 0, pady = 5, sticky = W)

HoldPressure_Button = ttk.Button(
    tab_regul,
    text = "Стабилизация давления",
    style = 'All.TButton'
)
HoldPressure_Button.grid(column = 0, row = 10, columnspan = 1, rowspan = 1, padx = 10, pady = 0, sticky = W)

HoldPressure_Entry = ttk.Entry(
    tab_regul,
    font = ("Arial Bold", 16),
    width = 5
)
HoldPressure_Entry.grid(column = 1, row = 10, columnspan = 1, rowspan = 1, padx = 10, pady = 5, sticky = W)

# Вывод результатов измрениий

All_width = 7
Value_sticky = N

speedSetText = ttk.Label(
    tab_regul,
    text = "Выстваленная скорость",
    background = 'white',
    font = ("Arial Bold", 16)
)
speedSetText.grid(column = 4, row = 0, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

speedSetValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)   
speedSetValue.grid(column = 5, row = 0, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

speedMeasText = ttk.Label(
    tab_regul,
    text = "Измеренная скорость",
    background = 'white',
    font = ("Arial Bold", 16)
)
speedMeasText.grid(column = 4, row = 1, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

speedMeasValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)   
speedMeasValue.grid(column = 5, row = 1, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

angleSetText = ttk.Label(
    tab_regul,
    text = "Выставленный угол",
    background = 'white',
    font = ("Arial Bold", 16)
)
angleSetText.grid(column = 4, row = 2, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

angleSetValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
angleSetValue.grid(column = 5, row = 2, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

CurPosText = ttk.Label(
    tab_regul,
    text = "Текущее положение",
    background = 'white',
    font = ("Arial Bold", 16)
)
CurPosText.grid(column = 4, row = 3, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

CurPosValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
CurPosValue.grid(column = 5, row = 3, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

CNTText = ttk.Label(
    tab_regul,
    text = "Счета таймера",
    background = 'white',
    font = ("Arial Bold", 16)
)
CNTText.grid(column = 4, row = 4, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

CNTValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
CNTValue.grid(column = 5, row = 4, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

PressText = ttk.Label(
    tab_regul,
    text = "Давление",
    background = 'white',
    font = ("Arial Bold", 16)
)
PressText.grid(column = 4, row = 5, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

PressValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
PressValue.grid(column = 5, row = 5, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

PressCodeText = ttk.Label(
    tab_regul,
    text = "Давление-код АЦП",
    background = 'white',
    font = ("Arial Bold", 16)
)
PressCodeText.grid(column = 4, row = 6, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

PressCodeValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
PressCodeValue.grid(column = 5, row = 6, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

PressCurText = ttk.Label(
    tab_regul,
    text = "Мгновенное давление",
    background = 'white',
    font = ("Arial Bold", 16)
)
PressCurText.grid(column = 4, row = 7, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

PressCurValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
PressCurValue.grid(column = 5, row = 7, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

PowerBoardText = ttk.Label(
    tab_regul,
    text = "Питание на плате",
    background = 'white',
    font = ("Arial Bold", 16)
)
PowerBoardText.grid(column = 4, row = 8, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

PowerBoardValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
PowerBoardValue.grid(column = 5, row = 8, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

CurrentText = ttk.Label(
    tab_regul,
    text = "Ток потребления",
    background = 'white',
    font = ("Arial Bold", 16)
)
CurrentText.grid(column = 4, row = 9, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

CurrentValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
CurrentValue.grid(column = 5, row = 9, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

WeightSensorText = ttk.Label(
    tab_regul,
    text = 'Усилие поршня',
    background = 'white',
    font = ("Arial Bold", 16)
)
WeightSensorText.grid(column = 4, row = 10, columnspan = 1, rowspan = 1, padx = 40, pady = 0, sticky = W)

WeightSensorValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16),
)
WeightSensorValue.grid(column = 5, row = 10, columnspan = 1, rowspan = 1, padx = 0, pady = 0, sticky = W)

# Позициоинрованние
CalibrDataText = ttk.Label(
    tab_regul,
    text = "РЕЗУЛЬТАТЫ ПОЗИЦИОНИРОВАННИЯ",
    background = 'white',
    font = ("Arial Bold", 16)
)
CalibrDataText.grid(column = 4, row = 11, columnspan = 2, rowspan = 1, padx = 40 , pady = 20, sticky = W)

MinAngleText = ttk.Label(
    tab_regul,
    text = "Минимальный угол",
    background = 'white',
    font = ("Arial Bold", 16)
)
MinAngleText.grid(column = 4, row = 12, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

MinAngleValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
MinAngleValue.grid(column = 5, row = 12, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

MaxAngleText = ttk.Label(
    tab_regul,
    text = "Максимальный угол",
    background = 'white',
    font = ("Arial Bold", 16)
)
MaxAngleText.grid(column = 4, row = 13, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

MaxAngleValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
MaxAngleValue.grid(column = 5, row = 13, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

MinPressText = ttk.Label(
    tab_regul,
    text = "Минимальное давление",
    background = 'white',
    font = ("Arial Bold", 16)
)
MinPressText.grid(column = 4, row = 14, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

MinPressValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
MinPressValue.grid(column = 5, row = 14, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

MaxPressText = ttk.Label(
    tab_regul,
    text = "Максимальное давление",
    background = 'white',
    font = ("Arial Bold", 16)
)
MaxPressText.grid(column = 4, row = 15, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

MaxPressValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
MaxPressValue.grid(column = 5, row = 15, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

WorkZoneText = ttk.Label(
    tab_regul,
    text = "Рабочая область",
    background = 'white',
    font = ("Arial Bold", 16)
)
WorkZoneText.grid(column = 4, row = 16, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

WorkzoneValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
WorkzoneValue.grid(column = 5, row = 16, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

FlagShowLabel= ttk.Label(
    tab_regul,
    text = 'СОСТОЯНИЕ ФЛАГОВ',
    background = 'white',
    font = ("Arial Bold", 16)
)
FlagShowLabel.grid(column = 0, row = 11, columnspan = 1, rowspan = 1, padx = 40 , pady = 5, sticky = W)

WorkStateLabel = ttk.Label(
    tab_regul,
    text = 'Стою/Вращаюсь',
    background = 'white',
    font = ("Arial Bold", 16)
)
WorkStateLabel.grid(column = 0, row = 12, columnspan = 1, rowspan = 1, padx = 10 , pady = 5, sticky = W)

WorkstateValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
WorkstateValue.grid(column = 1, row = 12, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

WorkModeLabel = ttk.Label(
    tab_regul,
    text = 'Режим работы',
    background = 'white',
    font = ("Arial Bold", 16)
)
WorkModeLabel.grid(column = 0, row = 13, columnspan = 1, rowspan = 1, padx = 10 , pady = 5, sticky = W)

WorkmodeValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
WorkmodeValue.grid(column = 1, row = 13, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

ErrorFlagLabel = ttk.Label(
    tab_regul,
    text = 'Флаг ошибки',
    background = 'white',
    font = ("Arial Bold", 16)
)
ErrorFlagLabel.grid(column = 0, row = 14, columnspan = 1, rowspan = 1, padx = 10 , pady = 5, sticky = W)

ErrorFlagValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
ErrorFlagValue.grid(column = 1, row = 14, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)

LogicFlagLabel = ttk.Label(
    tab_regul,
    text = 'Флаг логики',
    background = 'white',
    font = ("Arial Bold", 16)
)
LogicFlagLabel.grid(column = 0, row = 15, columnspan = 1, rowspan = 1, padx = 10 , pady = 5, sticky = W)

LogicFlagValue = ttk.Label(
    tab_regul,
    background = '#C0C0C0',
    width = All_width,
    anchor = CENTER,
    font = ("Arial Bold", 16)
)
LogicFlagValue.grid(column = 1, row = 15, columnspan = 1, rowspan = 1, padx = 0 , pady = 5, sticky = W)





