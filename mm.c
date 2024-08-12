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
#define ALIGNMENT 8     // doubleword size 선언

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

#define WSZIE       4       //header footer size 를 word(4)로 초기화
#define DSZIE       8       // dubleword Size
#define CHUNKSIZE   (1<<12) // heap을 늘릴때 얼만큼 늘릴거냐?? 4kb 분량.

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
#define GET_ALLOC(p)    (GET(p) & 0x01)                // 반대로 최하위 비트만 알기 위한 연산

#define HDRP(bp)        ((char *) (dp) - WSIZE)                         // 헤더 포인터 보통 bp는 페이로드를 가리키고 있음으로 4바이트 만큼 뒤로 가게한다.
#define FTRP(bp)        ((char *) (dp) + GET_SIZE(HDRP(bp)) - DSIZE)    // 푸터 포인터 bp에서 전체 크기만큼 이동하고 DSIZE만큼(8바이트)만크 뒤로 이동해서 푸터의 시작 주소를 가리킨다.

#define NEXT_BLKP(bp)   ((char *) (bp) + GET_SIZE(((char *)(dp) - WSZIE)))      // 현재 블록포인터를 기준으로 다음 블록의 시작주소 계산 다음 주소에서 헤더의 크기만큼 빼준다.
#define PREV_BLKP(bp)   ((char *) (bp) - GET_SIZE(((char *)(dp) - WSZIE)))      // 현재 블록포인터를 기준으로 이전 블록포인터의 시작주소 계산  이전 주소에서 푸터의 크기만큼 빼준다


////////////////////////////////////

int mm_init(void)
{
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
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














