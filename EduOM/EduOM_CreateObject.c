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
 * Module : EduOM_CreateObject.c
 * 
 * Description :
 *  EduOM_CreateObject() creates a new object near the specified object.
 *
 * Exports:
 *  Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 */

#include <string.h>
#include "EduOM_common.h"
#include "RDsM.h"		/* for the raw disk manager call */
#include "BfM.h"		/* for the buffer manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_CreateObject()
 *================================*/
/*
 * Function: Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 * 
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 * (1) What to do?
 * EduOM_CreateObject() creates a new object near the specified object.
 * If there is no room in the page holding the specified object,
 * it trys to insert into the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page.
 *
 * (2) How to do?
 *	a. Read in the near slotted page
 *	b. See the object header
 *	c. IF large object THEN
 *	       call the large object manager's lom_ReadObject()
 *	   ELSE 
 *		   IF moved object THEN 
 *				call this function recursively
 *		   ELSE 
 *				copy the data into the buffer
 *		   ENDIF
 *	   ENDIF
 *	d. Free the buffer page
 *	e. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADLENGTH_OM
 *    eBADUSERBUF_OM
 *    some error codes from the lower level
 *
 * Side Effects :
 *  0) A new object is created.
 *  1) parameter oid
 *     'oid' is set to the ObjectID of the newly created object.
 */
Four EduOM_CreateObject(
    ObjectID  *catObjForFile,	/* IN file in which object is to be placed */
    ObjectID  *nearObj,		/* IN create the new object near this object */
    ObjectHdr *objHdr,		/* IN from which tag is to be set */
    Four      length,		/* IN amount of data */
    char      *data,		/* IN the initial data for the object */
    ObjectID  *oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    ObjectHdr   objectHdr;	/* ObjectHdr with tag set from parameter */


    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (length < 0) ERR(eBADLENGTH_OM);

    if (length > 0 && data == NULL) return(eBADUSERBUF_OM);

	/* Error check whether using not supported functionality by EduOM */
	if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    //initialize header
    objectHdr.properties = 0x0;
    objectHdr.length = 0;
    if(objHdr!=NULL){
        objectHdr.tag = objHdr->tag;
    }
    else{
        objectHdr.tag = 0;
    }
    e = eduom_CreateObject(catObjForFile, nearObj, &objectHdr, length, data, oid);
    if (e < eNOERROR) ERR(e);
    
    return(eNOERROR);
}

/*@================================
 * eduom_CreateObject()
 *================================*/
/*
 * Function: Four eduom_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  eduom_CreateObject() creates a new object near the specified object; the near
 *  page is the page holding the near object.
 *  If there is no room in the near page and the near object 'nearObj' is not
 *  NULL, a new page is allocated for object creation (In this case, the newly
 *  allocated page is inserted after the near page in the list of pages
 *  consiting in the file).
 *  If there is no room in the near page and the near object 'nearObj' is NULL,
 *  it trys to create a new object in the page in the available space list. If
 *  fail, then the new object will be put into the newly allocated page(In this
 *  case, the newly allocated page is appended at the tail of the list of pages
 *  cosisting in the file).
 *
 * Returns:
 *  error Code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by fuction calls
 */
Four eduom_CreateObject(
                        ObjectID	*catObjForFile,	/* IN file in which object is to be placed */
                        ObjectID 	*nearObj,	/* IN create the new object near this object */
                        ObjectHdr	*objHdr,	/* IN from which tag & properties are set */
                        Four	length,		/* IN amount of data */
                        char	*data,		/* IN the initial data for the object */
                        ObjectID	*oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    Four	neededSpace;	/* space needed to put new object [+ header] */
    SlottedPage *apage;		/* pointer to the slotted page buffer */
    Four        alignedLen;	/* aligned length of initial data */
    Boolean     needToAllocPage;/* Is there a need to alloc a new page? */
    PageID      pid;            /* PageID in which new object to be inserted */
    PageID      nearPid;
    Four        firstExt;	/* first Extent No of the file */
    Object      *obj;		/* point to the newly created object */
    Two         i;		/* index variable */
    sm_CatOverlayForData *catEntry; /* pointer to data file catalog information */
    SlottedPage *catPage;	/* pointer to buffer containing the catalog */
    FileID      fid;		/* ID of file where the new object is placed */
    Two         eff;		/* extent fill factor of file */
    Boolean     isTmp;
    PhysicalFileID pFid;
    
    
    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (objHdr == NULL) ERR(eBADOBJECTID_OM);
    
    /* Error check whether using not supported functionality by EduOM */
    if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    // EduOM_CreateObject(&catalogEntry, NULL, NULL, strlen(omTestObjectNo), omTestObjectNo , &oid);
    Boolean lastpage_flag;
    //카탈로그 가져옴
    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if( e < 0 ) ERR( e );
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    fid = catEntry->fid;
    eff = catEntry->eff;
    

    //1. insert object 하는데 필요한 free space size 계산
    neededSpace= ALIGNED_LENGTH(length) + sizeof(ObjectHdr) + sizeof(SlottedPageSlot);

    //2. Selectt a page to insert the object
    if(nearObj != NULL)MAKE_PAGEID(nearPid, nearObj->volNo,nearObj->pageNo);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    /* Get the first extent number of the file */
    e = RDsM_PageIdToExtNo((PageID *)&pFid, &firstExt);
    if( e < 0 ) ERR( e );

    //2.1 nearObj is not NULL
    if (nearObj != NULL){
        MAKE_PAGEID(nearPid, nearObj->volNo,nearObj->pageNo);
        e = BfM_GetTrain((TrainID*)&nearPid, (char**)&apage, PAGE_BUF );
        if (e <0) ERR(e);
        // 2.1.1 Available space exist
        // total Free space
        if(SP_FREE(apage)>= neededSpace){
        // if (SP_CFREE(apage)>= neededSpace){ 
            // 1) select
            // e = BfM_FreeTrain((TrainID*)&neaerPid, PAGE_BUF);
            // if (e<0) ERR(e);
            // 1) 이 페이지로 선정함
            pid = nearPid;
            // e = BfM_GetNewTrain(&pid, (char**)&apage, PAGE_BUF);
            // if (e<0) ERR(e);

            // 2) delete the page from the available space list
            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if(e<0) ERRB1(e, &pid, PAGE_BUF);

            //3) Compact the selected page if necessary
            if (SP_CFREE(apage) < neededSpace){
                e = EduOM_CompactPage(apage, NIL);
                if (e<0) ERR(e);
            }
            
        }
        // No available space
        else{
            e = BfM_FreeTrain((TrainID*)&nearPid, PAGE_BUF);
            if (e<0) ERR(e);

            /* 1) Allocate a new page */
            e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
            if( e < 0 ) ERR( e );
            e = BfM_GetNewTrain((TrainID *)&pid , (char**)&apage, PAGE_BUF);
            if( e < 0 ) ERR( e );

            // 2) 헤더 초기화
            apage->header.free = 0;
            apage->header.unused  = 0;
            apage->header.fid = fid;
            apage->header.flags =SLOTTED_PAGE_TYPE;
            //page header의 unique는 건들면 안됨. 
            // e = om_GetUnique(&pid, &(apage->header.unique));
            if (e < 0) ERRB1(e, &pid, PAGE_BUF);

            // Insert the page into the list of pages of the file */
            e = om_FileMapAddPage(catObjForFile, &nearPid, &pid);
            // e = om_FileMapAddPage(catObjForFile, (PageID *)nearObj, &pid);
            if (e < 0) ERRB1(e, &pid, PAGE_BUF);

        }
    }
    lastpage_flag = 0;
    needToAllocPage = FALSE;

    
    // 2.2 near Obj == NULL
    if(nearObj == NULL){
        needToAllocPage = 0;
        //2.2.1 Select the first page of the corresponding available space list as the page to insert the object into.
        // pid.pageNo = NIL;

        MAKE_PAGEID(pid, catEntry->fid.volNo, NIL);
        if (neededSpace <= SP_10SIZE){
            pid.pageNo = catEntry->availSpaceList10;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->availSpaceList10);
        //     pid = catEntry->availSpaceList10;
        }
        if(pid.pageNo ==NIL && neededSpace <= SP_20SIZE){
            pid.pageNo = catEntry->availSpaceList20;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->availSpaceList20);
        }
        if(pid.pageNo ==NIL && neededSpace <= SP_30SIZE){
            pid.pageNo = catEntry->availSpaceList30;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->availSpaceList30);
        }
        if(pid.pageNo ==NIL && neededSpace <= SP_40SIZE){
            pid.pageNo = catEntry->availSpaceList40;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->availSpaceList40);
        }
        if(pid.pageNo ==NIL && neededSpace <= SP_50SIZE){
            pid.pageNo = catEntry->availSpaceList50;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->availSpaceList50);
        }
        if (pid.pageNo == NIL){
            needToAllocPage = 1;
        }
        if(!needToAllocPage){
            // pid.volNo = pFid.volNo;
            // delete the page from current available space list
            e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
            if (e<0) ERR(e);

            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if(e<0) ERRB1(e, &pid, PAGE_BUF);

            //3) Compact the selected page if necessary
            if (SP_CFREE(apage) < neededSpace){
                e = EduOM_CompactPage(apage, NIL);
                // if (e<0) ERR(e);
            }
        }
        
        
        //2.2.2 위에 가능한 리스트 없는데 마지막 페이지에 공간 있을떄
        else{
            // e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
            // pid.pageNo = catEntry->lastPage;
            // MAKE_PAGEID(pid, catEntry->fid.volNo, catEntry->lastPage);
            MAKE_PAGEID(pid, pFid.volNo, catEntry->lastPage);
            e = BfM_GetTrain((TrainID *)&pid, (char**)&apage, PAGE_BUF);
            // if( e < 0 ) ERR( e );
            // 여유공간 있다. 
            if(SP_FREE(apage)>=neededSpace){
                om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
                // lastpage_flag = 1;
                //3) Compact the selected page if necessary
                if (SP_CFREE(apage) < neededSpace){
                    e = OM_CompactPage(apage, NIL);
                    if (e<0) ERR(e);
                }
            }
            else{
                e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                if (e<0) ERR(e);

                MAKE_PAGEID(nearPid, pFid.volNo, catEntry->lastPage);

                e = RDsM_PageIdToExtNo(&nearPid, &firstExt);
                if (e<0) ERR(e);

                e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
                if( e < 0 ) ERR( e );
                e = BfM_GetTrain((TrainID *)&pid, (char**)&apage,PAGE_BUF);
                if( e < 0 ) ERR( e );

                // Initialize the page’s header.
                apage->header.fid = catEntry ->fid;
                // RESET_PAGE_TYPE(apage);
                apage->header.flags = SLOTTED_PAGE_TYPE;
                apage->header.free = 0;
                apage->header.unused  = 0;
                apage->header.pid = pid;
                apage->header.prevPage = NIL;
                apage->header.reserved = 0;

                apage->header.nSlots =1;
                // Insert the page as the last page into the list of pages of the file.
                
                e = om_FileMapAddPage(catObjForFile, &catEntry->lastPage, &pid);
                if (e < 0) ERRB1(e, &pid, PAGE_BUF);

                
            }

            
        

        }

    }

    /////////////////////3. Insert an object into the selected page/////////////////////////////
    // 3. 선정된 page에 object를 삽입함


    // 3.1 update object's header
    // obj = (Object *)apage->data + apage->header.free;
    obj = (Object *) &(apage->data[apage->header.free]);
    obj->header.length = length;
    obj->header.properties = objHdr->properties;
    obj->header.tag = objHdr->tag;

    // 3.2 Copy the object into the selected page’s contiguous free area.
    memcpy(obj->data, data, length);

    //3.3 Allocate an empty or new slot of the slot array to store the information for identifying the object copied.
    SlotNo allocslotNo = NIL;
    for (i = 0; i< apage->header.nSlots; i++){
        if(apage->slot[-i].offset == EMPTYSLOT){
            allocslotNo = i;
            break;
        }
    }

    if(allocslotNo ==NIL){
        allocslotNo = apage->header.nSlots;
        apage->header.nSlots +=1;
    }

    apage->slot[-allocslotNo].offset = apage->header.free;
    om_GetUnique(&pid, &(apage->slot[-allocslotNo].unique));
    // apage->slot[-(apage->header.nSlots)].tag 

    // apage->header.nSlots +=1;

    // 3.4 Update the page’s header. 
    //////////////////////////////////////////////////////////////////????///////////////////////////////////////////////////
    apage->header.free += (ALIGNED_LENGTH(length) + sizeof(ObjectHdr));

    // 3.5 Insert the page into an appropriate available space list
    e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);

    MAKE_OBJECTID(*oid, pid.volNo, pid.pageNo, allocslotNo, apage->slot[-allocslotNo].unique);

    BfM_SetDirty((TrainID*)catObjForFile , PAGE_BUF);
    BfM_SetDirty((TrainID*)&pid , PAGE_BUF);
    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
    if (e<0) ERR(e);
    e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
    if (e<0) ERR(e);
    
    return(eNOERROR);
    
} /* eduom_CreateObject() */


