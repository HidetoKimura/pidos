#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "freertos_basic.h"

#define INPUT_BUFFER_SIZE 256
#define STREAM_MAX 256
#define ARG_MAX    32

#define DELIM " \t\n\r"

typedef int32_t (*command_t)(int32_t argc, char *argv[]);

typedef struct {
    command_t   command_func;
    const char* command_name;
    const char* usage;
} CommandList;

static int32_t hello(int32_t argc, char **argv)
{
    printf("Hi there! \r\n");
    return 0;
}

static int32_t led(int32_t argc, char **argv)
{
    if (argc < 2) {
        return -1;
    }

    if(strcmp(argv[1], "on") == 0) {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        printf("Turning LED on.\r\n");
    } else if(strcmp(argv[1], "off") == 0) {
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        printf("Turning LED off.\r\n");
    } else {
        return -1;
    }

    return 0;
}

static int32_t help(int32_t argc, char **argv)
{
    printf("Available commands:hello, led, help, memory_study\r\n");
    return 0;
}

static CommandList commands[] = {
    {hello,             "hello",            "Usage: hello\r\n"                                                     }, 
    {led,               "led",              "Usage: led <on|off>\r\n"                                              }, 
    {help,              "help",             "Usage: help\r\n"                                                      },    
    {freertos_basic,    "freertos_basic",   "Usage: freertos_basic\r\n"                                            },    
    {NULL,              "",                 "Unknown command. Type 'help' for options.\r\n"                        }    
};

static void parse_input_stream(char* s, CommandList* commands)
{
    int32_t     argc;
    uint32_t    i;
    int32_t     ret;
    char*       argv[ARG_MAX];
    char*       tok;

    tok = strtok(s, DELIM);
    if(tok == NULL) {
        printf("invalid input\r\n");
        return;
    }
        
    argv[0] = tok;
    for(i = 1; i < (sizeof(argv)/sizeof(argv[0])); i++) {
        if((tok = strtok(NULL, DELIM)) == NULL) break;
            argv[i] = tok;
    }
    argc = i;

    ret = -1;
    for(; commands->command_func != NULL; commands++) {
        if(strcmp(argv[0], commands->command_name) == 0) {
            ret = commands->command_func(argc, argv);
        }
    }
    if (ret){
        printf("%s", commands->usage);
    }

    return;
}

int main() 
{
    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN); 
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    char input[INPUT_BUFFER_SIZE];
    int index = 0;

    printf("Pico Console Ready. Type something and press Enter:\r\n");

    while (true) {
        int ch = getchar_timeout_us(0);  // Non-blocking read
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\r' || ch == '\n') {
                input[index] = '\0';  // Null-terminate the string
                printf("\r\n");
                parse_input_stream(input, commands);
                printf("> ");
                index = 0;  // Reset input buffer

            } else if (index < INPUT_BUFFER_SIZE - 1) {
                input[index++] = (char)ch;
                putchar(ch);  // Echo back
            }
        }
        sleep_ms(10);  // Reduce CPU load
    }
    return 0;
}
