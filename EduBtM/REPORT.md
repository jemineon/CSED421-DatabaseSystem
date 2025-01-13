# EduBtM Report

Name: 전제민

Student id: 20180154

# Problem Analysis

이 프로젝트는 EduCOSMOS의 B+ 트리 인덱스 매니저에 대한 연산들을 구현하는 것을 목표로 한다. 구체적으로는 B+ 트리 인덱스 및 인덱스 페이지 관련 구조에 대한 연산들을 구현한다.

# Design For Problem Solving

## High Level
아래는 B+ 트리 색인 관리 및 연산을 위한 API 함수들이다.

- **EduBtM_CreateIndex**: 새로운 B+ 트리 인덱스를 생성하는 함수이다.
- **EduBtM_DestroyIndex**: 기존의 B+ 트리 인덱스를 삭제하는 함수이다.
- **EduBtM_InsertObject**: B+ 트리 인덱스에 새로운 객체를 삽입하는 함수이다.
- **EduBtM_DeleteObject**: B+ 트리 인덱스에서 객체를 삭제하는 함수이다.
- **EduBtM_Fetch**: B+ 트리 인덱스에서 특정 키 값을 가진 객체를 검색하는 함수이다.
- **EduBtM_FetchNext**: B+ 트리 인덱스에서 현재 커서 위치의 다음 객체를 검색하는 함수이다.
- **EduBtM_FetchPrev**: B+ 트리 인덱스에서 현재 커서 위치의 이전 객체를 검색하는 함수이다.

## Low Level
아래는 B+ 트리 색인 관리 및 연산을 위한 내부 함수들이다.

- **edubtm_CreateIndex**: 새로운 B+ 트리 인덱스를 생성하는 내부 함수이다.
- **edubtm_DestroyIndex**: 기존의 B+ 트리 인덱스를 삭제하는 내부 함수이다.
- **edubtm_InsertObject**: B+ 트리 인덱스에 새로운 객체를 삽입하는 내부 함수이다.
- **edubtm_DeleteObject**: B+ 트리 인덱스에서 객체를 삭제하는 내부 함수이다.
- **edubtm_Fetch**: B+ 트리 인덱스에서 특정 키 값을 가진 객체를 검색하는 내부 함수이다.
- **edubtm_FetchNext**: B+ 트리 인덱스에서 현재 커서 위치의 다음 객체를 검색하는 내부 함수이다.
- **edubtm_FetchPrev**: B+ 트리 인덱스에서 현재 커서 위치의 이전 객체를 검색하는 내부 함수이다.
- **edubtm_SplitLeaf**: B+ 트리의 리프 노드를 분할하는 내부 함수이다.
- **edubtm_SplitInternal**: B+ 트리의 내부 노드를 분할하는 내부 함수이다.
- **edubtm_CompactLeafPage**: B+ 트리의 리프 페이지를 압축하는 내부 함수이다.
- **edubtm_CompactInternalPage**: B+ 트리의 내부 페이지를 압축하는 내부 함수이다.

# Mapping Between Implementation And the Design

```
Four EduBtM_CreateIndex(
    ObjectID *catObjForFile,    /* IN catalog object of B+ tree file */
    PageID *rootPid)        /* OUT root page of the newly created B+tree */
{
    Four e;            /* error number */
    Boolean isTmp;
    SlottedPage *catPage;    /* buffer page containing the catalog object */
    sm_CatOverlayForBtree *catEntry; /* pointer to Btree file catalog information */
    PhysicalFileID pFid;    /* physical file ID */

    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if (e < 0) ERR(e);

    GET_PTR_TO_CATENTRY_FOR_BTREE(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);

    /* Allocate a new page to be used as a B+ tree index page */
    //rootPid 에 반환
    e = btm_AllocPage(catObjForFile, (PageID *)&pFid, rootPid);
    if (e<0) ERR(e);

    //test
    // init leaf
    e = edubtm_InitLeaf(rootPid, TRUE, FALSE);
    if (e<0) ERR(e);

    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
    if (e<0) ERR(e);

    return(eNOERROR);
    
} /* EduBtM_CreateIndex() */


```


```

Four EduBtM_DeleteObject(
    ObjectID *catObjForFile,    /* IN catalog object of B+-tree file */
    PageID   *root,        /* IN root Page IDentifier */
    KeyDesc  *kdesc,        /* IN a key descriptor */
    KeyValue *kval,        /* IN key value */
    ObjectID *oid,        /* IN Object IDentifier */
    Pool     *dlPool,        /* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead) /* INOUT head of the dealloc list */
{
    int        i;
    Four    e;            /* error number */
    Boolean lf;            /* flag for merging */
    Boolean lh;            /* flag for splitting */
    InternalItem item;        /* Internal item */
    SlottedPage *catPage;    /* buffer page containing the catalog object */
    sm_CatOverlayForBtree *catEntry; /* pointer to Btree file catalog information */
    PhysicalFileID pFid;        /* B+-tree file's FileID */


    /*@ check parameters */
    if (catObjForFile == NULL) ERR(eBADPARAMETER_BTM);

    if (root == NULL) ERR(eBADPARAMETER_BTM);

    if (kdesc == NULL) ERR(eBADPARAMETER_BTM);

    if (kval == NULL) ERR(eBADPARAMETER_BTM);

    if (oid == NULL) ERR(eBADPARAMETER_BTM);
    
    if (dlPool == NULL || dlHead == NULL) ERR(eBADPARAMETER_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }


    e = BfM_GetTrain((TrainID*)catObjForFile, (char**)&catPage, PAGE_BUF);
    if(e<0) ERR(e);

    MAKE_PHYSICALFILEID(pFid, catObjForFile->volNo, catObjForFile->pageNo);
    
    //1)일단 삭제 
    //test
    e = edubtm_Delete(catObjForFile, root, kdesc, kval, oid, &lf, &lh, &item, dlPool, dlHead);
    if(e) ERR(e);
    //2)underflow
    if (lf) {   
        e = btm_root_delete(&pFid, root, dlPool, dlHead);
        if(e<0) ERR(e);
        e = BfM_SetDirty(root, PAGE_BUF);
        if(e<0) ERR(e);
    } 
    //3)split
    if (lh) {
        //test
        e = edubtm_root_insert(catObjForFile, root, &item);
        if(e<0) ERR(e);
        e = BfM_SetDirty(root, PAGE_BUF);
        if(e<0) ERR(e);
    }

    e = BfM_FreeTrain((TrainID*)catObjForFile, PAGE_BUF);
    if(e<0) ERR(e);

    return(eNOERROR);
    
}   /* EduBtM_DeleteObject() */

```

```
Four EduBtM_DropIndex(
    PhysicalFileID *pFid,    /* IN FileID of the Btree file */
    PageID *rootPid,        /* IN root PageID to be dropped */
    Pool   *dlPool,        /* INOUT pool of the dealloc list elements */
    DeallocListElem *dlHead) /* INOUT head of the dealloc list */
{
    Four e;            /* for the error number */


    /*@ Free all pages concerned with the root. */

    // Delete a B+ tree index from an index file. 
    // Deallocate a root page and every child page of the B+ tree index.
    e = edubtm_FreePages(pFid, rootPid, dlPool, dlHead);
    if (e<0) ERR(e);

    return(eNOERROR);
    
} /* EduBtM_DropIndex() */


```


```
Four EduBtM_Fetch(
    PageID   *root,        /* IN The current root of the subtree */
    KeyDesc  *kdesc,        /* IN Btree key descriptor */
    KeyValue *startKval,    /* IN key value of start condition */
    Four     startCompOp,    /* IN comparison operator of start condition */
    KeyValue *stopKval,      /* IN key value of stop condition */
    Four     stopCompOp,    /* IN comparison operator of stop condition */
    BtreeCursor *cursor)    /* OUT Btree Cursor */
{
    int i;
    Four e;           /* error number */

    
    if (root == NULL) ERR(eBADPARAMETER_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    if (startCompOp == SM_BOF){
        e = edubtm_FirstObject(root, kdesc, stopKval, stopCompOp, cursor);
        if (e<0) ERR(e);
    }
    else if(startCompOp == SM_EOF){
        e = edubtm_LastObject(root, kdesc, stopKval, stopCompOp, cursor);
        if (e<0) ERR(e);
    }
    else{
        e = edubtm_Fetch(root, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e<0) ERR(e);
    }

    return(eNOERROR);

} /* EduBtM_Fetch() */


```


```

Four edubtm_Fetch(
    PageID              *root,          /* IN The current root of the subtree */
    KeyDesc             *kdesc,         /* IN Btree key descriptor */
    KeyValue            *startKval,     /* IN key value of start condition */
    Four                startCompOp,    /* IN comparison operator of start condition */
    KeyValue            *stopKval,      /* IN key value of stop condition */
    Four                stopCompOp,     /* IN comparison operator of stop condition */
    BtreeCursor         *cursor)        /* OUT Btree Cursor */
{
    Four                e;              /* error number */
    Four                cmp;            /* result of comparison */
    Two                 idx;            /* index */
    PageID              child;          /* child page when the root is an internla page */
    Two                 alignedKlen;    /* aligned size of the key length */
    BtreePage           *apage;         /* a Page Pointer to the given root */
    BtreeOverflow       *opage;         /* a page pointer if it necessary to access an overflow page */
    Boolean             found;          /* search result */
    PageID              *leafPid;       /* leaf page pointed by the cursor */
    Two                 slotNo;         /* slot pointed by the slot */
    PageID              ovPid;          /* PageID of the overflow page */
    PageNo              ovPageNo;       /* PageNo of the overflow page */
    PageID              prevPid;        /* PageID of the previous page */
    PageID              nextPid;        /* PageID of the next page */
    ObjectID            *oidArray;      /* array of the ObjectIDs */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */

    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }


    e = BfM_GetTrain(root, &apage, PAGE_BUF);
	if (e < 0) ERR(e);

	// 1. root page - internal
    if (apage->any.hdr.type & INTERNAL) 
	{
		found = edubtm_BinarySearchInternal(&(apage->bi), kdesc, startKval, &idx);

		if (idx != -1) {
            iEntry = &(apage->bi.data[apage->bi.slot[-idx]]);
			MAKE_PAGEID(child, root->volNo, iEntry->spid);
		}
		else{
            MAKE_PAGEID(child, root->volNo, apage->bi.hdr.p0);
        }
				
        e = edubtm_Fetch(&child, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e<0) ERR(e);

        e = BfM_FreeTrain(root, PAGE_BUF);
        if (e<0) ERR(e);
	}
	
    // 2. root page - leaf
    else if (apage->any.hdr.type & LEAF)
	{
        found = edubtm_BinarySearchLeaf(&apage->bl, kdesc, startKval, &idx);
        Boolean bflag = FALSE;
        leafPid = root;
		cursor->flag = CURSOR_ON;

        if (startCompOp == SM_EQ) {
            if (!found) {
                cursor->flag = CURSOR_EOS;
            }
        } 
        else if (startCompOp == SM_LT) {
            if (found) {
                idx -=1;
            } 
        } 
        else if (startCompOp == SM_LE) {
        } 
        else if (startCompOp == SM_GT) {
            idx+=1;
        } 
        else if (startCompOp == SM_GE) {
            if (!found) {
                idx +=1;
            }
        }

        // cursor  == CURSOR_EOS 
		if (cursor->flag == CURSOR_EOS){
            BfM_FreeTrain(root, PAGE_BUF);
            return eNOERROR;
        }
        
        // CURSOR_ON
        else if(cursor->flag == CURSOR_ON){
            if (idx == -1){   
                bflag = TRUE;
                if(apage->bl.hdr.prevPage == NIL){
                    cursor->flag = CURSOR_EOS;
                    
                }
                else
                {
                    MAKE_PAGEID(prevPid, root->volNo, apage->bl.hdr.prevPage);
                    leafPid = &prevPid;
                }
		    }
            else if (idx >= apage->bl.hdr.nSlots)
            {   
                if(apage->bl.hdr.nextPage == NIL){
                    cursor->flag = CURSOR_EOS;
                }
                else
                {
                    MAKE_PAGEID(nextPid, root->volNo, apage->bl.hdr.nextPage);
                    leafPid = &nextPid;
                    idx = 0;
                }
            }
            e = BfM_FreeTrain(root, PAGE_BUF);
            if(e<0) ERR(e);
            e = BfM_GetTrain(leafPid, &apage, PAGE_BUF);
            if(e<0) ERR(e);
            cursor->leaf = *leafPid;
            if (cursor->flag == CURSOR_ON)
            {   
                if(bflag){
                    idx = apage->bl.hdr.nSlots-1;
                }    
                cursor->slotNo = idx;
                lEntryOffset = apage->bl.slot[-idx];
                lEntry = &(apage->bl.data[lEntryOffset]);
                alignedKlen = ALIGNED_LENGTH(lEntry->klen);

                memcpy(&cursor->oid, &lEntry->kval[alignedKlen], sizeof(ObjectID));
                memcpy(&cursor->key, &lEntry->klen, sizeof(Two) + alignedKlen);

                cmp = edubtm_KeyCompare(kdesc, &cursor->key, stopKval);
                if (stopCompOp == SM_LT) {
                    if (cmp != LESS) {
                        cursor->flag = CURSOR_EOS;
                    }
                } 
                else if (stopCompOp == SM_LE) {
                    if(!(cmp == LESS || cmp == EQUAL)){
                        cursor->flag = CURSOR_EOS;
                    }
                } 
                else if (stopCompOp == SM_GT) {
                    if (cmp != GREATER) {
                        cursor->flag = CURSOR_EOS;
                    }
                } 
                else if (stopCompOp == SM_GE) {
                    if(!(cmp == GREATER || cmp == EQUAL)){
                        cursor->flag = CURSOR_EOS;
                    }
                }

            }	

		    e = BfM_FreeTrain(leafPid, PAGE_BUF);
            if(e<0) ERR(e);
        }
        
	}

    return(eNOERROR);
    
} /* edubtm_Fetch() */

          

```

