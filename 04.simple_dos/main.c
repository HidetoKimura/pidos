#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "commands.h"

#define INPUT_BUFFER_SIZE 256
#define STREAM_MAX 256
#define ARG_MAX    32

#define DELIM " \t\n\r"

static void parse_input_stream(char* s, CommandList* cmd)
{
    int32_t     argc;
    uint32_t    i;
    int32_t     ret;
    char*       argv[ARG_MAX];
    char*       tok;

    tok = strtok(s, DELIM);
    if(tok == NULL) {
        return;
    }
        
    argv[0] = tok;
    for(i = 1; i < (sizeof(argv)/sizeof(argv[0])); i++) {
        if((tok = strtok(NULL, DELIM)) == NULL) break;
            argv[i] = tok;
    }
    argc = i;

    ret = -1;
    for(; cmd->command_func != NULL; cmd++) {
        if(strcmp(argv[0], cmd->command_name) == 0) {
            ret = cmd->command_func(argc, argv);
            if (ret){
                printf("error command:%s\nusage:%s \n", s, cmd->usage);
            }
            break;
        }
    }
    if (cmd->command_func == NULL) {
        printf("Unknown command: %s.\nType 'help' for options.\n", argv[0]);
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

    printf("Pico Console Ready. Type something and press Enter:\n");
    printf("> ");

    while (true) {
        int ch = getchar_timeout_us(0);  // Non-blocking read
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\r' || ch == '\n') {
                input[index] = '\0';  // Null-terminate the string
                printf("\n");
                parse_input_stream(input, commands);
                printf("> ");
                index = 0;  // Reset input buffer
            } else if (ch == 0x08 || ch == 0x7F) { // Backspace or DEL
                if (index > 0) {
                    index--;
                    // Echo deletion visually
                    putchar('\b');
                    putchar(' ');
                    putchar('\b');
                }
            } else if (ch >= 32 && ch <= 126) { // printable ASCII
                if (index < INPUT_BUFFER_SIZE - 1) {
                    input[index++] = (char)ch;
                    putchar(ch);  // Echo back
                } else {
                    // buffer full, optional bell
                    putchar('\a');
                }
            }
        }
        sleep_ms(10);  // Reduce CPU load
    }
    return 0;
}
