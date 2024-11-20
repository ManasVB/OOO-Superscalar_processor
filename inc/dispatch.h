#pragma once

#include <cstdio>

extern bool Fetch(FILE *fp, unsigned long int);

extern void Decode(void);

extern void Rename(void);

extern void RegRead(void);

extern void Dispatch(void);
