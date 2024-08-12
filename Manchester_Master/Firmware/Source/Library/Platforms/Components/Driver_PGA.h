// Driver_PGA.h
// ����� ��������� �������� ���� ����������� ����������
#ifndef	DRIVER_PGA_H
#define	DRIVER_PGA_H

// ������� ���������� PGA
typedef enum PGA_Command_enum
{
	PGA_Command_Set,			// ���������� ������� ��������
	PGA_Command_SetMin,			// ���������� ����������� ��������
	PGA_Command_SetMax,			// ���������� ������������ ��������
	PGA_Command_Inc,			// ��������� �������� �� ���� �����
	PGA_Command_Dec,			// ��������� �������� �� ���� �����
	PGA_Command_GetCurrent, 	// ������� ������� ��������
	PGA_Command_GetMin, 		// ������� ����������� ��������
	PGA_Command_GetMax, 		// ������� ������������ ��������
} PGA_Command_t;

// ��� ������ ��� ����������� ������������ �������� PGA
typedef float PGA_Gain_t;

// ��� ������ ��� �������� PGA
//typedef void * xPGA_Hdl_t;

#endif	// DRIVER_PGA_H
