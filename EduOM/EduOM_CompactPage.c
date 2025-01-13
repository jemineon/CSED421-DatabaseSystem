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
 * Module : EduOM_CompactPage.c
 * 
 * Description : 
 *  EduOM_CompactPage() reorganizes the page to make sure the unused bytes
 *  in the page are located contiguously "in the middle", between the tuples
 *  and the slot array. 
 *
 * Exports:
 *  Four EduOM_CompactPage(SlottedPage*, Two)
 */


#include <string.h>
#include "EduOM_common.h"
#include "LOT.h"
#include "EduOM_Internal.h"



/*@================================
 * EduOM_CompactPage()
 *================================*/
/*
 * Function: Four EduOM_CompactPage(SlottedPage*, Two)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_CompactPage() reorganizes the page to make sure the unused bytes
 *  in the page are located contiguously "in the middle", between the tuples
 *  and the slot array. To compress out holes, objects must be moved toward
 *  the beginning of the page.
 *
 *  (2) How to do?
 *  a. Save the given page into the temporary page
 *  b. FOR each nonempty slot DO
 *	Fill the original page by copying the object from the saved page
 *          to the data area of original page pointed by 'apageDataOffset'
 *	Update the slot offset
 *	Get 'apageDataOffet' to point the next moved position
 *     ENDFOR
 *   c. Update the 'freeStart' and 'unused' field of the page
 *   d. Return
 *	
 * Returns:
 *  error code
 *    eNOERROR
 *
 * Side Effects :
 *  The slotted page is reorganized to comact the space.
 */
Four EduOM_CompactPage(
    SlottedPage	*apage,		/* IN slotted page to compact */
    Two         slotNo)		/* IN slotNo to go to the end */
{
    SlottedPage	tpage;		/* temporay page used to save the given page */
    Object *obj;		/* pointer to the object in the data area */
    Two    apageDataOffset;	/* where the next object is to be moved */
    Four   len;			/* length of object + length of ObjectHdr */
    Two    lastSlot;		/* last non empty slot */
    Two    i;			/* index variable */


    //temporary page에 정보 옮겨넣고
    //emptyslot 아닌 애들만 다시 원래 페이지로 옮김. 

    // tpage로 옮기고, last nonempty Slot 찾기
    memcpy(&tpage, apage, PAGESIZE);
    for (i =0; i<apage->header.nSlots; i++){
        if (apage->slot[-i].offset!=EMPTYSLOT && i != slotNo){
            lastSlot = i;
            
        }
    }
    
    apageDataOffset =0;

    for (i =0; i<=lastSlot; i++){
        
        if ((&tpage)->slot[-i].offset == EMPTYSLOT){
            continue;
        }
        if (slotNo == i){
            continue;
        }
        else{
            obj = &(tpage.data[tpage.slot[-i].offset]);

            //len = 오브젝트 길이 + 오브젝트 헤더 길이
            // 데이터 얼라인 
            len = ALIGNED_LENGTH(obj->header.length) + sizeof(obj->header);
            apage->slot[-i].offset = apageDataOffset;
            memcpy(&apage->data[apageDataOffset], obj, len);
            apageDataOffset +=len;
        }
        
    }

    // memcpy(apage->data[apageDataOffset], &tpage.data[tpage.slot[-i].offset], len);

    //3.Update Page header
    apage->header.free = apageDataOffset;
    apage->header.nSlots = lastSlot+1;
    apage->header.unused = 0;

    return(eNOERROR);
    
} /* EduOM_CompactPage */
