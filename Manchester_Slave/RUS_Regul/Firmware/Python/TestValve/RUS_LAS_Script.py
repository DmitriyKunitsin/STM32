import numpy as np 
import matplotlib.pyplot as plt

lasOut = {
	'Speed Meas': ('new', lasIn['Speed Meas']/100),
	'Angle Set': ('new', lasIn['Angle Set']/100),
	'Cur Position': ('new', lasIn['Cur Position']/100),
	'Press': ('new', lasIn['Press']/100),
	'Current Press': ('new', lasIn['Current Press']/100),
	'Power Supply': ('new', lasIn['Power Supply']/100),
	'Current': ('new', lasIn['Current']/100),
	'Calibr Null': ('new', lasIn['Calibr Null']/100),
	'Min Angle': ('new', lasIn['Min Angle']/100),
	'Max Angle': ('new', lasIn['Max Angle']/100),
	'Min Press': ('new', lasIn['Min Press']/100),
	'Max Press': ('new', lasIn['Max Press']/100),
}