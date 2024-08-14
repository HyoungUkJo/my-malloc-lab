/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/// 함수선언
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 16     // doubleword size 선언

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * mm_init - initialize the malloc package.
 */
/* 
mm_init 함수는 초기화 함수이다. 
초기화 하면서 만들어놔야할걸 생각
1. 프롤로그 헤더,푸타
2. 초기 패딩값
3. 에필로그헤더
 */

//////기본 상수 및 매크로 선언//////

#define WSIZE       4       //header footer size 를 word(4)로 초기화
#define DSIZE       8       // dubleword Size
#define CHUNKSIZE   (1<<12) // heap을 늘릴때 얼만큼 늘릴거냐?? 4kb 분량.
// 실제로 할당 할 메모리 사이즈
#define ALLOCSIZE(size)    ALIGN(size)+DSIZE

#define MAX(x,y) ((x)>(y)? (x) : (y)) //x,y중 더 큰값

/* 블록의 크기와 할당 상태를 하나의 워드로 압축해서 저장하기 위함 */
#define PACK(size,alloc)    ((size)|(alloc))    // size와 alloc을 or연산해서 하위 3비트에 alloc상태를 결합할 수있음.

/* read 와 write를 할 get, put 메크로 설정 */
#define GET(p)          (*(unsigned int *)(p))          // 포인터를 써서 주소의 값을 얻는다. 다른주소의 값을 가리키거나 이동할때 사용
                                                        // 블록의 헤더나 푸터에서 정보를 읽을 때 사용. 블록의 크기나 메타데이터에 접근 가능
#define PUT(p, val)     (*(unsigned int *)(p) = (val))  //블록의 주소를 담는다. 위치를 담아야지 나중에 헤더나 푸터를 읽어서 연결할 수 있다.
                                                        // 새로운 메모리 블럭을 할당하거나 기존 블록의 상태를 변경할때 사용
/* p의 위치로부터 size를 읽고 field를 할당 */
#define GET_SIZE(p)     (GET(p) & ~0x7)                 //메모리의 사이즈를 얻기 위한 메크로 0x07은 ~ 연산자로 상위비트만 가지고 있는 값이 되고 이를 and연산해서 하위 비트를 날린다
#define GET_ALLOC(p)    (GET(p) & 0x01)                 // 반대로 최하위 비트만 알기 위한 연산

#define HDRP(bp)        ((char *) (bp) - WSIZE)                         // 헤더 포인터 보통 bp는 페이로드를 가리키고 있음으로 4바이트 만큼 뒤로 가게한다.
#define FTRP(bp)        ((char *) (bp) + GET_SIZE(HDRP(bp)) - DSIZE)    // 푸터 포인터 bp에서 전체 크기만큼 이동하고 DSIZE만큼(8바이트)만크 뒤로 이동해서 푸터의 시작 주소를 가리킨다.

#define NEXT_BLKP(bp)   ((char *) (bp) + GET_SIZE(((char *)(bp) - WSIZE)))      // 현재 블록포인터를 기준으로 다음 블록의 시작주소 계산 다음 주소에서 헤더의 크기만큼 빼준다.
#define PREV_BLKP(bp)   ((char *) (bp) - GET_SIZE(((char *)(bp) - WSIZE)))      // 현재 블록포인터를 기준으로 이전 블록포인터의 시작주소 계산  이전 주소에서 푸터의 크기만큼 빼준다

static char *heap_listp;    //  처음에 쓸 가용 블록 힙을 만들어준다. -> 시작점 제공

////////////////////////////////////

int mm_init(void){
    /* 초기 힙구조 설정 */
    if ((heap_listp = mem_sbrk(4*WSIZE))== (void*)-1)
        return -1;
    PUT(heap_listp,0);                              // 힙의 첫 워드를 0으로 초기화
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1));       // 프롤로그 헤더 설정
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1));       // 프롤로그 푸터 설정
    PUT(heap_listp + (3*WSIZE), PACK(0,1));         // 에필로그 헤더 설정
    heap_listp += (2*WSIZE);                        // 힙의 시작점을 프롤로그 포인터 다음으로 설정
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)          // 초기에 할당한 메모리를 다 설정했음으로 확장 이를위해 확장할 수 있는 함수도 만들어야함.
        return -1;

    return 0;   
}
/* 힙의 크기를 늘려야 할때 */
static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words &1) ? (words+1) * WSIZE : words * WSIZE;  // 조건 연산자 8byte 정렬을 유지하기 위해 크기를 조정한다.
    if((long)(bp=mem_sbrk(size))== -1){                     // 힙의 크기를 증가
        return NULL;
    }
    PUT(HDRP(bp), PACK(size,0));                            // 새로 확장된 힙의 위치에 할당되지 않은 블록 헤더 설정
    PUT(FTRP(bp), PACK(size,0));                             // 푸터 설정
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));                     // 새블록 다음에 에필로그 헤더를 배치

    return coalesce(bp);                                    // 가용 블록과 새 블록을 병합

}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));     // 이전 블럭 가용상태 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));     // 다음 블럭 가용상태 확인
    size_t size = GET_SIZE(HDRP(bp));                       // 지금 블록 사이즈

    if (prev_alloc && next_alloc){                          // 지금 블럭과 다음 블럭이 다 사용중인 경우
        return bp;                                          //
    }
    else if (prev_alloc && !next_alloc){                    // 이전 블럭은 할당 다음 블럭은 미할당
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));              // 다음 블록의 헤더를 보고 그 블록의 크기만큼 블록의 사이즈에 추가
        PUT(HDRP(bp),PACK(size,0));                         // 지금 블럭의 헤더 갱신
        PUT(FTRP(bp), PACK(size, 0));                       // 지금 블럭의 푸터 갱신
    }
    else if (!prev_alloc && next_alloc){                    // 이전블록은 미할당 다음 블록은 할당 
        size= GET_SIZE(HDRP(PREV_BLKP(bp)));               // 이전 블럭의 헤더를보고 그 크기만큼 블록의 사이즈에 할당
        PUT(FTRP(bp), PACK(size,0));                        // 지금블럭의 푸터에 갱신
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));             // 현재 헤더에서 앞블록으로 이동해서 조정한 size 저장
        bp = PREV_BLKP(bp);                                 // bp를 그 앞블록의 헤더로 이동
    }
    else {//두 블록 다 가용상태(미할당 상태)
        size+= GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전블록 헤더, 다음블록 푸터까지 통합할 사이즈
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));                               // 앞 헤더 사이즈 최신화
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));                               // 뒤 푸터 사이즈 최신화
        bp = PREV_BLKP(bp);                                                   // bp는 전블럭으로 이동
    }
    return bp;
}

static void *find_fit(size_t size){
    // heap_listp를 이용해서 탐색 시작? or bp를 이용해서 탐색시작
    // 힙을 탐색할 포인터
    char * findp = heap_listp;

    while(GET_SIZE(HDRP(findp))!=0){        //에필로그 헤더를 가리키기 전까지 계속
        if (!GET_ALLOC(HDRP(findp))){       // 만약 할당되지 않은 상태라면
            if(GET_SIZE(HDRP(findp))>=size){ // 사이즈 확인해서 더 크면 할당
                return findp;
            }
            else
                findp=NEXT_BLKP(findp);     // 아니면 다음 블록으로 이동
        }
        else
            findp=NEXT_BLKP(findp);
    }
    return NULL;
}

static void place(void *bp, size_t size){ // 들어갈 위치를 포인터로 받는다.(find fit에서 찾는 bp) 크기는 asize로 받음.
    // 요청한 블록을 가용 블록의 시작 부분에 배치, 나머지 부분의 크기가 최소 블록크기와 같거나 큰 경우에만 분할하는 함수.
    size_t csize = GET_SIZE(HDRP(bp)); // 현재 있는 블록의 사이즈.
    if ( (csize-size) >= (2*DSIZE)){ // 현재 블록 사이즈안에서 asize를 넣어도 2*DSIZE(헤더와 푸터를 감안한 최소 사이즈)만큼 남냐? 남으면 다른 data를 넣을 수 있으니까.
        PUT(HDRP(bp), PACK(size,1)); 
        PUT(FTRP(bp), PACK(size,1)); 
        bp = NEXT_BLKP(bp); // regular block만큼 하나 이동해서 bp 위치 갱신.
        PUT(HDRP(bp), PACK(csize-size,0)); 
        PUT(FTRP(bp), PACK(csize-size,0)); 
    }
    else{
        PUT(HDRP(bp), PACK(csize,1)); // 위의 조건이 아니면 asize만 csize에 들어갈 수 있으니까 내가 다 먹는다.
        PUT(FTRP(bp), PACK(csize,1));
    }
}


/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
/*     int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    } */

   // 1. 비어있는 블럭을 탐색한다.
   size_t r_size;       // 실제로 할당해야하는 사이즈
   size_t ex_size;   // 추가로 할당받은 사이즈
   char * bp;
    // 오류제어
   if (size==0) return NULL;

    // 실제로 할당해야하는 사이즈 
    r_size = ALLOCSIZE(size);

    // 메모리에 맞는 위치 탐색
    if((bp=find_fit(r_size)) != NULL ){
        place(bp, r_size);
        return bp;
    }
    //만약 맞는 메모리 크기가 없을 경우
    // 최소 단위는 16byte
    ex_size = MAX(r_size, CHUNKSIZE);
    if((bp=extend_heap(ex_size/WSIZE))==NULL){  //wsize로 나누는 이유는 우리는 워드 블록 단위로 힙을 확장할 것임으로... extend 함수 안에서 알아서 실제 사이즈로 확장함.
        return NULL;
    }
    place(bp, r_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr){
    // 미할당으로 변경하기 위해 사이즈 저장
    size_t size = GET_SIZE(HDRP(ptr));

    // 해당 블럭의 헤더 푸터 재설정
    PUT(HDRP(ptr), PACK(size,0));
    PUT(FTRP(ptr), PACK(size,0));

    // 메모리 병합을 위한 함수 선언
    coalesce(ptr);


}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

