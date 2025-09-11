################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/animation/Anifunc.c 

OBJS += \
./Core/Src/animation/Anifunc.o 

C_DEPS += \
./Core/Src/animation/Anifunc.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/animation/%.o Core/Src/animation/%.su Core/Src/animation/%.cyclo: ../Core/Src/animation/%.c Core/Src/animation/subdir.mk
	arm-none-eabi-gcc -fcommon "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-animation

clean-Core-2f-Src-2f-animation:
	-$(RM) ./Core/Src/animation/Anifunc.cyclo ./Core/Src/animation/Anifunc.d ./Core/Src/animation/Anifunc.o ./Core/Src/animation/Anifunc.su

.PHONY: clean-Core-2f-Src-2f-animation

