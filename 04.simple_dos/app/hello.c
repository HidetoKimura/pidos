// Comments must be in English.
#include "service_table.h"

__attribute__((section(".text.main")))
void main(void){
    svc_tbl()->puts("[APP]HELLO");
    svc_tbl()->exit(123);  // Return to the OS
    while(1){}
}
