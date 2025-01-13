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
 * Module : EduOM_DestroyObject.c
 * 
 * Description : 
 *  EduOM_DestroyObject() destroys the specified object.
 *
 * Exports:
 *  Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 */

#include "EduOM_common.h"
#include "Util.h"		/* to get Pool */
#include "RDsM.h"
#include "BfM.h"		/* for the buffer manager call */
#include "LOT.h"		/* for the large object manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_DestroyObject()
 *================================*/
/*
 * Function: Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_DestroyObject() destroys the specified object. The specified object
 *  will be removed from the slotted page. The freed space is not merged
 *  to make the contiguous space; it is done when it is needed.
 *  The page's membership to 'availSpaceList' may be changed.
 *  If the destroyed object is the only object in the page, then deallocate
 *  the page.
 *
 *  (2) How to do?
 *  a. Read in the slotted page
 *  b. Remove this page from the 'availSpaceList'
 *  c. Delete the object from the page
 *  d. Update the control information: 'unused', 'freeStart', 'slot offset'
 *  e. IF no more object in this page THEN
 *	   Remove this page from the filemap List
 *	   Dealloate this page
 *    ELSE
 *	   Put this page into the proper 'availSpaceList'
 *    ENDIF
 * f. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    eBADFILEID_OM
 *    some errors caused by function calls
 */
Four EduOM_DestroyObject(
    ObjectID *catObjForFile,	/* IN file containing the object */
    ObjectID *oid,		/* IN object to destroy */
    Pool     *dlPool,		/* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead)	/* INOUT head of dealloc list */
{
    Four        e;		/* error number */
    Two         i;		/* temporary variable */
    FileID      fid;		/* ID of file where the object was placed */
    PageID	pid;		/* page on which the object resides */
    SlottedPage *apage;		/* pointer to the buffer holding the page */
    Four        offset;		/* start offset of object in data area */
    Object      *obj;		/* points to the object in data area */
    Four        alignedLen;	/* aligned length of object */
    Boolean     last;		/* indicates the object is the last one */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */
    DeallocListElem *dlElem;	/* pointer to element of dealloc list */
    PhysicalFileID pFid;	/* physical ID of file */
    
    

    /*@ Check parameters. */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

    if (oid == NULL) ERR(eBADOBJECTID_OM);
    // if(IS_VALID_OBJECTID(oid, apage)){
    //     return 
    // }

    /* Fix the page that contains the catalog object to the buffer */
    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if( e < 0 ) ERR( e );
    //cat entry get
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    //catObjForFile 이게 object가 들어있는 파일 가리키는 오브젝트를 가리키는 포인터
    //sm_CatOverlayForData strsuct

    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
    if( e < 0 ) ERR( e );

    //1.delete the page from the available space list
    e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    if(e<0) ERRB1(e, &pid, PAGE_BUF);
    
    // 이제 apage가 slotted page pointer. offset 을 초기화 해줌
    offset = apage->slot[-(oid->slotNo)].offset;
    apage->slot[-(oid->slotNo)].offset = EMPTYSLOT;
    obj = apage->data+offset;

    // 2. update page header
    // 2.1 If the slot corresponding to the object deleted is the last slot of the slot array, update the size of the slot array.
    // nSlots 은 slot array 의 크기. SlotNo : arry index
    if(oid->slotNo== apage->header.nSlots-1 ){
        apage->header.nSlots -=1;
    }
    
    // 2.2 Update the variables free or unused depending on the offset in the data area of the object deleted.
    alignedLen = ALIGNED_LENGTH(obj->header.length);
    if (apage->header.free == offset + sizeof(ObjectHdr) + alignedLen){
        apage->header.free = offset;
    }
    else{
        //unused : contiguous가 아닌 free space 크기
        apage->header.unused += (sizeof(ObjectHdr) + alignedLen);

    }
    // apage->header.unused +=1;
    
    // 3. 삭제된 obj가 그 페이지의 유일한 오브젝트 && not first page of file
    // i = 0;
    // last = FALSE;
    Boolean flag = FALSE;
    for (i=0; i< apage->header.nSlots; i++){
        if (apage->slot[-i].offset != EMPTYSLOT) {
            flag = TRUE;
            break;
        }
    }
    //flag 가 TRUE 면 삭제된 obj 말고도 페이지에 다른 obj가 있다. 
    // if (flag)
        // if (i == apage->header.nSlots) last = TRUE; //유일한 object이다. 
    if (!flag&& (catEntry->firstPage != pid.pageNo) ){
        // ppt 63
        // delete the page from the list of pages of the file
        e = om_FileMapDeletePage(catObjForFile, &pid);
        if(e<0) ERRB1(e, &pid, PAGE_BUF);

         /* Insert the deallocated page into the dealloc list */
        e = Util_getElementFromPool(dlPool, &dlElem);
        if( e < 0 ) ERR( e );
        dlElem->type = DL_PAGE;
        dlElem->elem.pid = pid;    /* ID of the deallocated page */
        dlElem->next = dlHead->next;
        dlHead->next = dlElem;
    }
    //4. 삭제된게 페이지의 유일한 obj가 아니거나, 그 페이지가 파일의 첫 페이지가 아니거나 
    else{
        e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
    }

    /* Unfix the page that contains the catalog object from the buffer */
    e = BfM_SetDirty((TrainID*)catObjForFile, PAGE_BUF);
    if( e < 0 ) ERR( e );
    /* Set the DIRTY bit */
    e = BfM_SetDirty((TrainID*)&pid, PAGE_BUF);
    if (e < 0) ERRB1(e, &pid, PAGE_BUF);

    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
    if( e < 0 ) ERR( e );
    e = BfM_FreeTrain(&pid, PAGE_BUF);
    if( e < 0 ) ERR( e );
    
    return(eNOERROR);
    
} /* EduOM_DestroyObject() */
