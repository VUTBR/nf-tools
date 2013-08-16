#include <stdio.h>
#include <stdlib.h>
#include <math.h>


int lpm_init(void);
int lpm_add(int handle, char *prefix, SV *value);
SV * lpm_lookup(int handle, char *addr);
SV *lpm_lookup_raw(int handle, SV *svaddr);
void lpm_destroy(int handle);


