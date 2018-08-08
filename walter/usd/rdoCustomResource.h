// Copyright 2017 Rodeo FX. All rights reserved.

#ifndef __RDOCUSTOMRESOURCE_H__
#define __RDOCUSTOMRESOURCE_H__

#ifdef __cplusplus
extern "C" {
#endif

void setResource(const char* filename, const char* contents);
const char* getResource(const char* filename);

#ifdef __cplusplus
}
#endif

#endif
