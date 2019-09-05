#include "libdino_internal.h"
#include "system.h"

void dino_data_free(Dino_Data dd) {
    dd.off=0;
    dd.size=0;
    free(dd.data);
}

void section_data_free(Dino_Sec *sec) {
    dino_data_free(sec->data.d);
    Dino_Data_List *cur=sec->data.next, *next;
    while (cur) {
        dino_data_free(cur->d);
        next = cur->next;
        free(cur);
        cur = next;
    }
}
