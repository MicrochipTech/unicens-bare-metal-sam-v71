#ifndef _GMAC_INIT_H_
#define _GMAC_INIT_H_

#include "gmac.h"
#include "gmac_init.h"
#include "gmacb_phy.h"
#include "gmacd.h"
#include "gmii.h"

#ifdef __cplusplus
extern "C" {
#endif
    
void init_gmac(sGmacd  *pGmacd);

#ifdef __cplusplus
}
#endif

#endif
