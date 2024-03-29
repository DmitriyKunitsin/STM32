################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/SSD1306/fonts.c \
../Drivers/SSD1306/lcd.c \
../Drivers/SSD1306/ssd1306.c 

OBJS += \
./Drivers/SSD1306/fonts.o \
./Drivers/SSD1306/lcd.o \
./Drivers/SSD1306/ssd1306.o 

C_DEPS += \
./Drivers/SSD1306/fonts.d \
./Drivers/SSD1306/lcd.d \
./Drivers/SSD1306/ssd1306.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SSD1306/%.o Drivers/SSD1306/%.su Drivers/SSD1306/%.cyclo: ../Drivers/SSD1306/%.c Drivers/SSD1306/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-SSD1306

clean-Drivers-2f-SSD1306:
	-$(RM) ./Drivers/SSD1306/fonts.cyclo ./Drivers/SSD1306/fonts.d ./Drivers/SSD1306/fonts.o ./Drivers/SSD1306/fonts.su ./Drivers/SSD1306/lcd.cyclo ./Drivers/SSD1306/lcd.d ./Drivers/SSD1306/lcd.o ./Drivers/SSD1306/lcd.su ./Drivers/SSD1306/ssd1306.cyclo ./Drivers/SSD1306/ssd1306.d ./Drivers/SSD1306/ssd1306.o ./Drivers/SSD1306/ssd1306.su

.PHONY: clean-Drivers-2f-SSD1306

