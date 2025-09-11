################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/hw/hw_modbus.c \
../Core/Src/hw/hw_oled.c \
../Core/Src/hw/hw_sd.c 

OBJS += \
./Core/Src/hw/hw_modbus.o \
./Core/Src/hw/hw_oled.o \
./Core/Src/hw/hw_sd.o 

C_DEPS += \
./Core/Src/hw/hw_modbus.d \
./Core/Src/hw/hw_oled.d \
./Core/Src/hw/hw_sd.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/hw/%.o Core/Src/hw/%.su Core/Src/hw/%.cyclo: ../Core/Src/hw/%.c Core/Src/hw/subdir.mk
	arm-none-eabi-gcc -fcommon "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/hw" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/ap" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/common" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps/ANIMATION" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps/ANIMATION/csrc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-hw

clean-Core-2f-Src-2f-hw:
	-$(RM) ./Core/Src/hw/hw_modbus.cyclo ./Core/Src/hw/hw_modbus.d ./Core/Src/hw/hw_modbus.o ./Core/Src/hw/hw_modbus.su ./Core/Src/hw/hw_oled.cyclo ./Core/Src/hw/hw_oled.d ./Core/Src/hw/hw_oled.o ./Core/Src/hw/hw_oled.su ./Core/Src/hw/hw_sd.cyclo ./Core/Src/hw/hw_sd.d ./Core/Src/hw/hw_sd.o ./Core/Src/hw/hw_sd.su

.PHONY: clean-Core-2f-Src-2f-hw

