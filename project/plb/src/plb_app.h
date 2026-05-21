#ifndef PLB_APP_H
#define PLB_APP_H

#include <stdbool.h>

void plb_app_init(void);
void plb_app_request_sos(void);
bool plb_app_step(void);
int plb_app_dump_evtlog(void);

#endif /* PLB_APP_H */
