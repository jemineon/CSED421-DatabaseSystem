# EduOM Report

Name: 전제민

Student id: 20180154

# Problem Analysis

이 프로젝트는 EduCOSMOS DBMS 내의 오브젝트 매니저와 페이지 구조에 관련된 연산을 구현하는 것을 목표로 한다. 구체적으로, 객체 저장을 위한 슬롯 페이지 구조와 관련된 연산을 구현한다.

# Design For Problem Solving

## High Level
아래는 Slotted Page manage를 위한 API function들이다.

- **EduOM_CreateObject**: 적합한 슬롯 페이지에 새 객체를 삽입하고, 객체 헤더를 초기화한 후 그 ID를 반환하는 함수이다. 
- **EduOM_CompactPage**: 페이지 내 객체 오프셋을 조정하여 모든 여유 공간이 연속되게 하여, 공간 활용을 개선하는 함수이다. 
- **EduOM_DestroyObject**: 페이지에서 객체를 제거하고, 페이지가 비었거나 재구성이 필요한 경우 필요한 정리 작업을 수행한다.
- **EduOM_ReadObject**: 객체 데이터의 전체 또는 일부를 검색하기 위한 함수이다. 
- **EduOM_NextObject 및 EduOM_PrevObject**: 파일 내 객체를 순차적으로 접근할 수 있게 하기 위한 함수이다.  

## Low Level
아래는 Slotted Page manage를 위한 internal function이다. 

- **eduom_CreateObject**: 
File을 구성하는 page들 중 파라미터로 지정한 object와 같거나 인접한 page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환하는 함수이다. 



# Mapping Between Implementation And the Design


## EduOM_CreateObject
```
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
    // printf("SEX");
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
```

## eduom_CreateObject
```
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
```

## EduOM_DestroyObejct
```
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
```

## EduOM_CompactPage
```
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
    //시발 길이 머고
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
            // 여기 데이터 얼라인 해야되나? 그럴듯.
            len = ALIGNED_LENGTH(obj->header.length) + sizeof(obj->header);
            apage->slot[-i].offset = apageDataOffset;
            // memcpy(apage+apageDataOffset, (&tpage) + (&tpage)->slot[-i].offset, len);
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
```

## EduOM_NextObject
```
Four EduOM_NextObject(
    ObjectID  *catObjForFile,	/* IN informations about a data file */
    ObjectID  *curOID,		/* IN a ObjectID of the current Object */
    ObjectID  *nextOID,		/* OUT the next Object of a current Object */
    ObjectHdr *objHdr)		/* OUT the object header of next object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for next page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    PhysicalFileID pFid;	/* file in which the objects are located */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* data structure for catalog object access */



    /*@
     * parameter checking
     */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (nextOID == NULL) ERR(eBADOBJECTID_OM);

    //카탈로그 가져옴
    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if( e < 0 ) ERR( e );
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    Boolean findflag = 0;

    // If curOID given as a parameter is NULL, 
    if(curOID ==NULL){
        pageNo = catEntry->firstPage;
        while(pageNo> 0){
            MAKE_PAGEID(pid, pFid.volNo, pageNo);
            e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
            if(e<0) ERR(e);
            for (i=0; apage->header.nSlots; i++){
                offset = apage->slot[-i].offset;

                if (offset!=EMPTYSLOT){
                    findflag = 1;
                    obj = (Object*)&(apage->data[offset]);
                    MAKE_OBJECTID(*nextOID, pFid.volNo, pageNo, i, apage->slot[-i].unique);
                    objHdr = &obj->header;
                    break;
                }
            }
            // 찾음
            if(findflag){
                BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                break;
            }

            // 못찾음
            if(!findflag){
                // 방금 페이지가 마지막 페이지였다면 break
                if(pageNo == catEntry->lastPage){
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                    break;
                }
                else{
                    pageNo = apage->header.nextPage;
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                }
            }
        }
    }

    else{
        MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
        e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
        if (e<0) ERR(e);
        //일단 현재 curobj 있는 페이지에 다음 오브젝트 있는지 찾는다.
        if (curOID->slotNo +1 <apage->header.nSlots){
            for(i = curOID->slotNo+1; i < apage->header.nSlots; i++)
            {
                offset = apage->slot[-i].offset;
                if(offset!=EMPTYSLOT)
                {
                    findflag = 1;
                    obj = (Object*)&(apage->data[offset]);
                    MAKE_OBJECTID(*nextOID, curOID->volNo, curOID->pageNo, i, apage->slot[-i].unique);
                    objHdr = &obj->header;
                    break;
                        
                }
            }
        }
        
        pageNo = apage->header.nextPage;
        e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
        if (e<0) ERR(e);

        // 현재페이지에 없다. 그럼 찾을때까지 nextpage
        if (!findflag){
            
            while(pageNo>0)
            {
                // pageNo에 해당하는 페이지 찾고
                MAKE_PAGEID(pid, pFid.volNo, pageNo);
                e = BfM_GetTrain((TrainID *)&pid, (char**)&apage, PAGE_BUF);
                if (e<0) ERR(e);

                // 해당 페이지 slot 다 검사
                for(i = 0; i<apage->header.nSlots; i++){
                    offset = apage->slot[-i].offset;

                    if(offset!=EMPTYSLOT)
                    {
                        findflag = 1;
                        obj = (Object*)&(apage->data[offset]);
                        MAKE_OBJECTID(*nextOID, pFid.volNo, pageNo, i, apage->slot[-i].unique);
                        objHdr = &obj->header;
                        break;
                
                    }
                }

                // next object 찾았다.
                if(findflag){
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                    break;
                }
                // next obj 못찾았다. 
                if(!findflag)
                {   
                    // 방금 페이지가 파일의 마지막 페이지였다면
                    if(pageNo == catEntry->lastPage)
                    {
                        BfM_FreeTrain((TrainID*)&pid, PAGE_BUF); 
                        break;
                    }
                    else
                    {
                        BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                        pageNo = apage->header.nextPage;
                    } 
                } 

            }
        }



    }
    
    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);

    return(EOS);		/* end of scan */
    
} /* EduOM_NextObject() */
```


## EduOM_PrevObject
```
Four EduOM_PrevObject(
    ObjectID *catObjForFile,	/* IN informations about a data file */
    ObjectID *curOID,		/* IN a ObjectID of the current object */
    ObjectID *prevOID,		/* OUT the previous object of a current object */
    ObjectHdr*objHdr)		/* OUT the object header of previous object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    PageNo pageNo;		/* a temporary var for previous page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */



    /*@ parameter checking */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (prevOID == NULL) ERR(eBADOBJECTID_OM);

    PhysicalFileID pFid;	/* file in which the objects are located */
    //카탈로그 가져옴
    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if( e < 0 ) ERR( e );
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    Boolean findflag = 0;

    // If curOID given as a parameter is NULL, 
    if(curOID ==NULL){
        pageNo = catEntry->lastPage;
        while(pageNo> 0){
            MAKE_PAGEID(pid, pFid.volNo, pageNo);
            e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
            if(e<0) ERR(e);
            for (i=apage->header.nSlots-1; i>=0; i--){
                offset = apage->slot[-i].offset;

                if (offset!=EMPTYSLOT){
                    findflag = 1;
                    obj = (Object*)&(apage->data[offset]);
                    MAKE_OBJECTID(*prevOID, pFid.volNo, pageNo, i, apage->slot[-i].unique);
                    objHdr = &obj->header;
                    break;
                }
            }
            // 찾음
            if(findflag){
                BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                break;
            }

            // 못찾음
            if(!findflag){
                // 방금 페이지가 마지막 페이지였다면 break
                if(pageNo == catEntry->firstPage){
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                    break;
                }
                else{
                    pageNo = apage->header.prevPage;
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                }
            }
        }
    }

    else{
        MAKE_PAGEID(pid, curOID->volNo, curOID->pageNo);
        e = BfM_GetTrain((TrainID*)&pid, (char**)&apage, PAGE_BUF);
        if (e<0) ERR(e);
        //일단 현재 curobj 있는 페이지에 다음 오브젝트 있는지 찾는다.
        if (curOID->slotNo -1>=0){
            for(i = curOID->slotNo-1; i >=0; i--)
            {
                offset = apage->slot[-i].offset;
                if(offset!=EMPTYSLOT)
                {
                    findflag = 1;
                    obj = (Object*)&(apage->data[offset]);
                    MAKE_OBJECTID(*prevOID, curOID->volNo, curOID->pageNo, i, apage->slot[-i].unique);
                    objHdr = &obj->header;
                    break;
                        
                }
            }
        }
        
        pageNo = apage->header.prevPage;
        e = BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
        if (e<0) ERR(e);

        // 현재페이지에 없다. 그럼 찾을때까지 prevpage
        if (!findflag){
            
            while(pageNo>0)
            {
                // pageNo에 해당하는 페이지 찾고
                MAKE_PAGEID(pid, pFid.volNo, pageNo);
                e = BfM_GetTrain((TrainID *)&pid, (char**)&apage, PAGE_BUF);
                if (e<0) ERR(e);

                // 해당 페이지 slot 다 검사
                for(i = apage->header.nSlots -1; i>=0; i--){
                    offset = apage->slot[-i].offset;

                    if(offset!=EMPTYSLOT)
                    {
                        findflag = 1;
                        obj = (Object*)&(apage->data[offset]);
                        MAKE_OBJECTID(*prevOID, pFid.volNo, pageNo, i, apage->slot[-i].unique);
                        objHdr = &obj->header;
                        break;
                
                    }
                }

                // prev object 찾았다.
                if(findflag){
                    BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                    break;
                }
                // prev obj 못찾았다. 
                if(!findflag)
                {   
                    // 방금 페이지가 파일의 마지막 페이지였다면
                    if(pageNo == catEntry->firstPage)
                    {
                        BfM_FreeTrain((TrainID*)&pid, PAGE_BUF); 
                        break;
                    }
                    else
                    {
                        BfM_FreeTrain((TrainID*)&pid, PAGE_BUF);
                        pageNo = apage->header.prevPage;
                    } 
                } 

            }
        }



    }
    



    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);

    return(EOS);
    
} /* EduOM_PrevObject() */
```

## EduOM_ReadObject
```
Four EduOM_ReadObject(
    ObjectID 	*oid,		/* IN object to read */
    Four     	start,		/* IN starting offset of read */
    Four     	length,		/* IN amount of data to read */
    char     	*buf)		/* OUT user buffer to return the read data */
{
    Four     	e;              /* error code */
    PageID 	pid;		/* page containing object specified by 'oid' */
    SlottedPage	*apage;		/* pointer to the buffer of the page  */
    Object	*obj;		/* pointer to the object in the slotted page */
    Four	offset;		/* offset of the object in the page */

    
    
    /*@ check parameters */

    if (oid == NULL) ERR(eBADOBJECTID_OM);

    if (length < 0 && length != REMAINDER) ERR(eBADLENGTH_OM);
    
    if (buf == NULL) ERR(eBADUSERBUF_OM);

    
    // The 'length' bytes from 'start' are copied from the disk to the user buffer

    // SlotNo objSlotNo = NIL;
    //pid계산, bfm gettrain
    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain((TrainID *)&pid, (char**)&apage, PAGE_BUF);
    if (e<0) ERR(e);

    offset = apage->slot[oid->slotNo].offset;
    obj = (Object*)&(apage->data[offset]);

    // Read as much data as length from start in the data area of the object.
    // If length == REMAINDER, read data to the end.

    if(length == REMAINDER){
        // if obj->
        
        length = obj->header.length;
        memcpy(buf, &obj->data, length);
        // memcpy(buf, (char*)(obj->data + start), 1);
        // memcpy(buf, (char*)(obj->data + start), obj->header.length);
    }
    else{
        memcpy(buf, (char*)(obj->data + start), length);
    }

    e = BfM_FreeTrain((TrainID *)&pid, PAGE_BUF);

    

    return(length);
    
} /* EduOM_ReadObject() */
```





