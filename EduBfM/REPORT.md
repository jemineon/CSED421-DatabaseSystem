# EduBfM Report

Name: Jeon Jemin

Student id: 20180154

# Problem Analysis

EduBfM 프로젝트는 Odysseus/COSMOS 시스템의 일부를 교육 목적으로 구현하는 EduCOSMOS 프로젝트의 일환이며 버퍼 매니저를 구현하는 것에 중점을 두고 있다.

# Design For Problem Solving

## High Level

아래는 Buffer manager API function들이다.

EduBfM_GetTrain()
- Page/train을 bufferPool에 fix 하고, page/train이 저장된 buffer element에 대한 포인터를 반환하는 함수이다. 

EduBfM_FreeTrain()
- Page/train을 bufferPool에서 unfix 하는 함수이다. 

EduBfM_SetDirty()
- bufferPool에 저장 된 page/train이 modified(dirty bit set) 되었음을 표시하기 위한 함수이다. 

EduBfM_FlushAll()
- bufferPool에 저장된 page/train 들 중 modified 된(dirty bit set) page/train을 disk에 쓰는 함수이다. 

EduBfM_DiscardAll()
- bufferPool에 저장된 page/train 들을 bufferPool에서 삭제하는 함수이다. 수정된 page/train 이 존재하더라도 disk에 쓰지 않는다.  


## Low Level

아래는 Buffer manager internal function들이다. 

edubfm_ReadTrain()
- Page/train을 disk로부터 읽어와서 buffer element에 저장하고, 해당 buffer element에 대한 포인터를 반환하는 함수이다. 

edubfm_AllocTrain()
- bufferPool에서 buffer element 하나를 할당 받고, 해당 buffer element의 array index를 반환하는 함수이다. 할당 받을 buffer element를 선정할 때, Second chance buffer replacement algorithm을 사용한다. 

edubfm_Insert()
- hashTable에 buffer element의 array index를 삽입하기 위한 함수이다. 

edubfm_Delete()
- hashTable에서 buffer element의 array index를 삭제하기 위한 함수이다. 

edubfm_Deleteall()
- 각 hashTable에서 모든 entry를 삭제하기 위한 함수이다. 

edubfm_LookUp()
- 주어진 hash key를 갖는 page/train이 저장된 buffer element의 array index를 hashTable에서 검색하여 반환하는 함수이다. 

edubfm_FlushTrain()
- 수정된 page/train 들을 disk에 기록하는 함수이다. 기록 후 dirty bit을 초기화 해준다. 

# Mapping Between Implementation And the Design

### Api Functions

- EduBfM_GetTrain

```
Four EduBfM_GetTrain(
    TrainID             *trainId,               /* IN train to be used */
    char                **retBuf,               /* OUT pointer to the returned buffer */
    Four                type )                  /* IN buffer type */
{
    Four                e;                      /* for error */
    Four                index;                  /* index of the buffer pool */

    
    /*@ Check the validity of given parameters */
    /* Some restrictions may be added         */
    if(retBuf == NULL) ERR(eBADBUFFER_BFM);

    /* Is the buffer type valid? */
    if(IS_BAD_BUFFERTYPE(type)) ERR(eBADBUFFERTYPE_BFM);	
    
    // 1. hashing 해서 버퍼풀 확인 . B.E의 index 
    index = edubfm_LookUp((BfMHashKey*)trainId, type);
    // 찾음
    if(index != NOTFOUND_IN_HTABLE) {
        BI_BITS(type, index) |= REFER; //refer update
        BI_FIXED(type, index) += 1; // fixed update
    }

    // 못찾음 
    else if (index == NOTFOUND_IN_HTABLE){
        //Alloc
        index = edubfm_AllocTrain(type);
        if (index < 0) {
            ERR(index);
        }
        
        //Insert
        e = edubfm_Insert((BfMHashKey*)trainId, index, type);
        if (e < 0) {
            ERR(e);
        }

        //ReadTrain
        e = edubfm_ReadTrain(trainId, BI_BUFFER(type, index), type);
        if (e < 0) {
            ERR(e);
        }

        //update
        BI_KEY(type, index) = *((BfMHashKey*)trainId); // key update
        BI_BITS(type, index) |= REFER; // refer update
        BI_FIXED(type, index) = 1; // fixed update
    }

    
    *retBuf = BI_BUFFER(type, index);
    // *retBuf = BI_HASHTABLEENTRY(type,index); //return the idx-th element of the buffer table


    // 2.a 있으면 
    // 버퍼 엘레민트 에 대응되는 버퍼테이블 엘레멘트 갱신 
    //저장된 버퍼 엘레민트 포인터 반환
    // BI_NEXTHASHENTRY(type, idx)

     // 2.b 없으면  bufferpool에서 
    //Fix 할 page/train이 bufferPool에 존재하지 않는 경우,
    // – bufferPool에서 page/train을 저장할 buffer element 한 개를 할당 받음 – Page/train을 disk로부터 읽어와서 할당 받은 buffer element에 저장함 – 할당 받은 buffer element에 대응하는 bufTable element를 갱신함
    // – 할당 받은 buffer element의 array index를 hashTable에 삽입함
    // – 할당 받은 buffer element에 대한 포인터를 반환함
    //edubfm_ReadTrain - Page/train을 disk로부터 읽어와서 buffer element에 저장하고, 해당 buffer element에 대한 포인터를 반환함

    return(eNOERROR);   /* No error */

}  /* EduBfM_GetTrain() */
```

- EduBfM_FreeTrain

```
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
```

- EduBfM_SetDirty

```
Four EduBfM_SetDirty(
    TrainID             *trainId,               /* IN which train has been modified in the buffer?  */
    Four                type )                  /* IN buffer type */
{
    Four                index;                  /* an index of the buffer table & pool */


    /*@ Is the paramter valid? */
    if (IS_BAD_BUFFERTYPE(type)) ERR(eBADBUFFERTYPE_BFM);

// 수정된 page/train의 hash key value를 이용하여, 해당 page/train이 저장된 buffer element의 array index를 hashTable에서 검색함
// 해당 buffer element에 대한 DIRTY bit를 1로 set함

    index = edubfm_LookUp((BfMHashKey*)trainId, type);

    if (index ==NOTFOUND_IN_HTABLE){
        ERR(index);
    }

    else if (index !=NOTFOUND_IN_HTABLE){
        BI_BITS(type, index) |= DIRTY; // dirty set
    }

    return( eNOERROR );

}  /* EduBfM_SetDirty */

```

- EduBfM_FlushAll
```
Four EduBfM_FlushAll(void)
{
    Four        e;                      /* error */
    Two         i;                      /* index */
    Four        type;                   /* buffer type */

// 각 bufferPool에 존재하는 page/train들 중 수정된 page/train들을 disk에 기록함
// DIRTY bit가 1로 set 된 buffer element들에 저장된 각 page/train에 대해, edubfm_FlushTrain()을 호출하여 해당 page/train을 disk에 기록함


    for (type = 0; type <2; type++){
        for (i = 0; i< bufInfo[type].nBufs; i++){
            if(BI_BITS(type, i)&DIRTY){
                e = edubfm_FlushTrain(&(BI_KEY(type, i)),type );
                if(e <0){
                    ERR(e);
                }
            }
        }
    }



    return( eNOERROR );
    
}  /* EduBfM_FlushAll() */
```


- EduBfM_DiscardAll

```
Four EduBfM_DiscardAll(void)
{
    Four 	e;			/* error */
    Two 	i;			/* index */
    Four 	type;			/* buffer type */


// key: set pageNo to NIL(-1).
// fixed: you do not need to initialize fixed, which is managed by EduBfM_GetTrain() and EduBfM_FreeTrain().
// bits: reset all bits.
// nextHashEntry: you do not need to initialize nextHashEntry, which is managed by edubfm_Insert() and edubfm_Delete().

    for (type = 0; type <NUM_BUF_TYPES; type++){
        for (i = 0; i< bufInfo[type].nBufs; i++){
        // for (i = 0; i< BI_NBUFS(type); i++){
            //key초기화
            SET_NILBFMHASHKEY(BI_KEY(type, i));
            // if(e <0){
            //     ERR(e);
            // }
            //bit 초기화
            BI_BITS(type, i)= 0;

        }
    }

    e = edubfm_DeleteAll();
    if (e < 0) {
        ERR(e);
    }

    return(eNOERROR);

}  /* EduBfM_DiscardAll() */
```

### internal functions
- edubfm_ReadTrain

```
Four edubfm_ReadTrain(
    TrainID *trainId,		/* IN which train? */
    char    *aTrain,		/* OUT a pointer to buffer */
    Four    type )		/* IN buffer type */
{
    Four e;			/* for error */


	/* Error check whether using not supported functionality by EduBfM */
	if (RM_IS_ROLLBACK_REQUIRED()) ERR(eNOTSUPPORTED_EDUBFM);

    e = RDsM_ReadTrain(trainId, aTrain, BI_BUFSIZE(type));
    if (e<0){
        ERR(e);
    }

    return( eNOERROR );

}  /* edubfm_ReadTrain */
```

- edubfm_AllocTrain

```
Four edubfm_AllocTrain(
    Four 	type)			/* IN type of buffer (PAGE or TRAIN) */
{
    Four 	e;			/* for error */
    Four 	victim;			/* return value */
    Four 	i;
    

	/* Error check whether using not supported functionality by EduBfM */
	if(sm_cfgParams.useBulkFlush) ERR(eNOTSUPPORTED_EDUBFM);

    victim = BI_NEXTVICTIM(type);
    //second chance
    //refer bit로 1이면 0으로 바꾸고 0이면 바로 evict
    //순회는 /(bufInfo.nextVictim +1)%bufInfo.nBufs
    //buffer table을 순회
    i = 0;
    while(1){
        if (!BI_FIXED(type, victim)){
            if(BI_BITS(type, victim) & REFER){
                BI_BITS(type, victim) &= 0x3;
            }
            else{
                break;
            }
        }

        victim = (victim+1)%BI_NBUFS(type);
        i+=1;

        //error : fixed가 전부 0이 아니다.
        if (i >= 2*BI_NBUFS(type)){
            ERR(eNOUNFIXEDBUF_BFM);
            // e = eNOUNFIXEDBUF_BFM;
            // break;
            // return eNOUNFIXEDBUF_BFM;
        }
    }

    //initialize the data structure related to the victim buffer element

    //check dirty bit
    if (BI_BITS(type, victim) & DIRTY == DIRTY ){
        //flush
        e = edubfm_FlushTrain((TrainID*)&BI_KEY(type, victim), type);
        if (e!= 0){
            ERR(e);
        }
    }

    //reset all bits
    BI_BITS(type, victim) =0;

    // next_victim set
    BI_NEXTVICTIM(type) = (victim+1)%BI_NBUFS(type);

    // hash table에서 index 지움
    e = edubfm_Delete(&BI_KEY(type, victim) , type);
    // if (e!= 0){
    //     ERR(e);
    // }
    
    return( victim );

    
}  /* edubfm_AllocTrain */
```

- edubfm_Insert

```
Four edubfm_Insert(
    BfMHashKey 		*key,			/* IN a hash key in Buffer Manager */
    Two 		index,			/* IN an index used in the buffer pool */
    Four 		type)			/* IN buffer type */
{
    Four 		i;			
    Two  		hashValue;

    Two BufTableidx;

    CHECKKEY(key);    /*@ check validity of key */

    if( (index < 0) || (index > BI_NBUFS(type)) )
        ERR( eBADBUFINDEX_BFM );
    
    hashValue = BFM_HASH(key, type);

    // idx : buffer table의 idx, hash_value : hash table의 index
    // hash table의 index에 해당하는 엔트리 값을 idx로 바꿔줘야된다
    // next hash entry = 만약 같은 해시키를 가진애가 있다면 bufTable내의 다음 인덱스
    // BI_NEXTHASHENTRY(type, index) = BI_HASHTABLEENTRY(type, hashValue);

    // 기존 buftable idx
    BufTableidx = BI_HASHTABLEENTRY(type, hashValue);

    // hash table entry update
    BI_HASHTABLEENTRY(type, hashValue) = index;

    BI_NEXTHASHENTRY(type, index) = BufTableidx;

    return( eNOERROR );

}  /* edubfm_Insert */
```

- edubfm_Delete
```
Four edubfm_Delete(
    BfMHashKey          *key,                   /* IN a hash key in buffer manager */
    Four                type )                  /* IN buffer type */
{
    Two                 i, prev;                
    Two                 hashValue;
    Two next;

    CHECKKEY(key);    /*@ check validity of key */
    i = edubfm_LookUp(key, type); // buffer table의 index
    if (i ==NOTFOUND_IN_HTABLE){
        ERR(i);
    }
    
    hashValue = BFM_HASH(key, type);

    prev = BI_HASHTABLEENTRY(type, hashValue);

    //삭제하는게 처음 element인 경우
    if(prev== i){
        BI_HASHTABLEENTRY(type, hashValue)=BI_NEXTHASHENTRY(type, i);
    }
    
    else{
        while (1){
            next = BI_NEXTHASHENTRY(type, prev);
            if(next == i){
                break;
            }
            else if(next ==NIL){
                break;
            }
            prev = next;
        }
        BI_NEXTHASHENTRY(type, prev) = BI_NEXTHASHENTRY(type, i);
    }
    return (eNOERROR);
    // ERR( eNOTFOUND_BFM );

}  /* edubfm_Delete */
```

- edubfm_DeleteAll

```
Four edubfm_DeleteAll(void)
{
    Two 	i;
    Four        tableSize;
    Two type;
    for (type = 0; i< 2; i++){
        tableSize= HASHTABLESIZE(type);
        for (i =0; i<tableSize; i++){
            BI_HASHTABLEENTRY(type, i) = NIL;
        }
    }

    return(eNOERROR);

} /* edubfm_DeleteAll() */ 
```

- edubfm_LookUp

```
Four edubfm_LookUp(
    BfMHashKey          *key,                   /* IN a hash key in Buffer Manager */
    Four                type)                   /* IN buffer type */
{
    Two                 i, j;                   /* indices */
    Two                 hashValue;
    

    CHECKKEY(key);    /*@ check validity of key */

    hashValue = BFM_HASH(key, type);
    i = BI_HASHTABLEENTRY(type, hashValue);
    while(1){
        if(EQUALKEY(&BI_KEY(type, i), key)){
            return i;
        }
        if(i == NIL){
            return(NOTFOUND_IN_HTABLE);
        }
        i = BI_NEXTHASHENTRY(type, i);
    }

    return(NOTFOUND_IN_HTABLE);
    
}  /* edubfm_LookUp */
```

- edubfm_FlushTrain

```
Four edubfm_FlushTrain(
    TrainID 			*trainId,		/* IN train to be flushed */
    Four   			type)			/* IN buffer type */
{
    Four 			e;			/* for errors */
    Four 			index;			/* for an index */


	/* Error check whether using not supported functionality by EduBfM */
	if (RM_IS_ROLLBACK_REQUIRED()) ERR(eNOTSUPPORTED_EDUBFM);

    index = edubfm_LookUp((BfMHashKey*)trainId, type);
    if(index == NOTFOUND_IN_HTABLE){
        ERR(index);
    }

    //dirty
    else if(BI_BITS(type, index)&DIRTY){
        e = RDsM_WriteTrain(BI_BUFFER(type, index), trainId, BI_BUFSIZE(type)); 
        BI_BITS(type, index) &=0x6; // reset dirty bit
        if (e<0){
            ERR(e);
        }
    }
	
    return( eNOERROR );

}  /* edubfm_FlushTrain */
```
