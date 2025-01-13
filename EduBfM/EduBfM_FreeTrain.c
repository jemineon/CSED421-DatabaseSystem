/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module: EduBfM_FreeTrain.c
 *
 * Description :
 *  Free(or unfix) a buffer.
 *
 * Exports:
 *  Four EduBfM_FreeTrain(TrainID *, Four)
 */


#include "EduBfM_common.h"
#include "EduBfM_Internal.h"



/*@================================
 * EduBfM_FreeTrain()
 *================================*/
/*
 * Function: Four EduBfM_FreeTrain(TrainID*, Four)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Free(or unfix) a buffer.
 *  This function simply frees a buffer by decrementing the fix count by 1.
 *
 * Returns :
 *  error code
 *    eBADBUFFERTYPE_BFM - bad buffer type
 *    some errors caused by fuction calls
 */
Four EduBfM_FreeTrain( 
    TrainID             *trainId,       /* IN train to be freed */
    Four                type)           /* IN buffer type */
{
    Four                index;          /* index on buffer holding the train */
    Four 		e;		/* error code */

    /*@ check if the parameter is valid. */
    if (IS_BAD_BUFFERTYPE(type)) ERR(eBADBUFFERTYPE_BFM);	

//     Unfix 할 page/train의 hash key value를 이용하여, 해당 page/train이 저장된 buffer element의 array index를 hashTable에서 검색함
// 해당 buffer element에 대한 fixed 변수 값을 1 감소시킴

    //check
    index = edubfm_LookUp((BfMHashKey*)trainId, type);
    // 못찾음
    if (index ==NOTFOUND_IN_HTABLE){
        ERR(index);
    }

    // 찾음
    else if (index !=NOTFOUND_IN_HTABLE){
        if (BI_FIXED(type, index)>0) {
            BI_FIXED(type, index) -=1;
        }
        else{
            printf("fixed counter is less than 0!!!\n");
            printf("trainId = {%d, %d}\n", trainId->volNo, trainId->pageNo);
        }
    }
    
    
    return( eNOERROR );
    
} /* EduBfM_FreeTrain() */
