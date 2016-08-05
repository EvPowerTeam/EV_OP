

#ifndef __SYSTEM_CALL__H
#define __SYSTEM_CALL__H

#include "API.h"

int Socket(int , int, int);
int Bind(int , const struct sockaddr *, socklen_t);
int Listen(int, int);
int Accept(int, struct sockaddr *, socklen_t *);
int Access(const char *, int);
int Setsockopt(int, int, int, const void *, socklen_t);

#endif //  system_call.h

