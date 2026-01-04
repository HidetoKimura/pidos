#include "commands.h"

#include "basic.h"
#include "rdisk.h"

CommandList commands[] = {
    {basic_cmd_help,    "help",             "help"                                             },    
    {basic_cmd_led,     "led",              "led <on|off>"                                     }, 
    {rdisk_cmd_format,  "format",           "format"                                           },    
    {rdisk_cmd_dir,     "dir",              "dir"                                              },    
    {rdisk_cmd_save,    "save",             "save <filename> <data>"                           },    
    {rdisk_cmd_delete,  "delete",           "delete <filename>"                                },    
    {rdisk_cmd_type,    "type",             "type <filename>"                                  },    
    {rdisk_cmd_loadhex, "loadhex",          "loadhex <name> <caphex>"                          },    
    {rdisk_cmd_run,     "run",              "run <name> [addrhex]"                             },    
    {NULL,              "",                 ""                                                 }  // End marker                                            
};
