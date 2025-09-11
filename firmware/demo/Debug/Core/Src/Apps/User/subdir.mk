################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Apps/User/user_func.c 

OBJS += \
./Core/Src/Apps/User/user_func.o 

C_DEPS += \
./Core/Src/Apps/User/user_func.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/Apps/User/%.o Core/Src/Apps/User/%.su Core/Src/Apps/User/%.cyclo: ../Core/Src/Apps/User/%.c Core/Src/Apps/User/subdir.mk
	arm-none-eabi-gcc -fcommon "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G431xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -I../FATFS/Target -I../FATFS/App -I../Middlewares/Third_Party/FatFs/src -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/hw" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/ap" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/common" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps/ANIMATION" -I"C:/Users/user/Desktop/projects/workspace/demo/Core/Src/Apps/ANIMATION/csrc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-Apps-2f-User

clean-Core-2f-Src-2f-Apps-2f-User:
	-$(RM) ./Core/Src/Apps/User/user_func.cyclo ./Core/Src/Apps/User/user_func.d ./Core/Src/Apps/User/user_func.o ./Core/Src/Apps/User/user_func.su

.PHONY: clean-Core-2f-Src-2f-Apps-2f-User

