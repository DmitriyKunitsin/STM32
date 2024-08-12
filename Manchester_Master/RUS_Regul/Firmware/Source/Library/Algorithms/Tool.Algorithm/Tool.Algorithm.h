#ifndef __TOOL_ALGORITHM_H__
#define __TOOL_ALGORITHM_H__

//#include <cmath>
#include <stdint.h>

#ifdef _WIN32  
#ifdef __TOOL_ALGORITHM_EXPORT__
#define EXPORT_EMF_CORE __declspec(dllexport)
#else
#define EXPORT_EMF_CORE __declspec(dllimport)
#endif
#else
#define EXPORT_EMF_CORE
#endif


#endif
