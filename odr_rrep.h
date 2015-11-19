/**
* @file         :  odr_rrep.h
* @author       :  Jiewen Zheng
* @date         :  2015-11-15
* @brief        :  RREP implementation
* @changelog    :
**/

#ifndef __ODR_RREP_H_
#define __ODR_RREP_H_

#include "np.h"

/**
* @brief  Handle the RREP message
* @param[in] obj     : odr_object object
* @param[in] frame   : odr_frame object 
* @param[in] from    : struct sockaddr_ll
* @return  0 if OK, -1 on error
**/
int HandleRREP(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from); 

#endif // __ODR_RREP_H_
