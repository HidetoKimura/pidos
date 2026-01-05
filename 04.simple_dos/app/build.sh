!#/bin/sh
arm-none-eabi-gcc -I../ -mcpu=cortex-m0plus -mthumb \
  -ffreestanding -nostdlib -O0 -g -c hello.c -o hello.o 
arm-none-eabi-ld -T user_app.ld -Map hello.map -o hello.elf hello.o
arm-none-eabi-objcopy -O binary hello.elf hello.bin
arm-none-eabi-objcopy -O ihex hello.elf hello.hex
arm-none-eabi-objdump -d hello.elf > hello.dis