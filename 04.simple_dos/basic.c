#include <stdint.h>
#include <stddef.h>
#include "pico/stdlib.h"
#include "commands.h"

int32_t basic_cmd_led(int32_t argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }

    if(strcmp(argv[1], "on") == 0) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        printf("Turning LED on.\n");
    } else if(strcmp(argv[1], "off") == 0) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        printf("Turning LED off.\n");
    } else {
        return -1;
    }

    return 0;
}

int32_t basic_cmd_help(int32_t argc, char **argv)
{
    CommandList* cmd = commands;
    for(; cmd->command_func != NULL; cmd++) {
        printf("%s\n", cmd->usage);
    }
    return 0;
}

