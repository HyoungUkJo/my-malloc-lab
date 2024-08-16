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

// #define WSIZE       4       //header footer size 를 word(4)로 초기화
// #define DSIZE       8       // dubleword Size
// #define CHUNKSIZE   (1<<12) // heap을 늘릴때 얼만큼 늘릴거냐?? 4kb 분량.
// // 실제로 할당 할 메모리 사이즈
#define ALLOCSIZE(size)    ALIGN(size)+DSIZE

// #define MAX(x,y) ((x)>(y)? (x) : (y)) //x,y중 더 큰값

// /* 블록의 크기와 할당 상태를 하나의 워드로 압축해서 저장하기 위함 */
// #define PACK(size,alloc)    ((size)|(alloc))    // size와 alloc을 or연산해서 하위 3비트에 alloc상태를 결합할 수있음.

// /* read 와 write를 할 get, put 메크로 설정 */
// #define GET(p)          (*(unsigned int *)(p))          // 포인터를 써서 주소의 값을 얻는다. 다른주소의 값을 가리키거나 이동할때 사용
//                                                         // 블록의 헤더나 푸터에서 정보를 읽을 때 사용. 블록의 크기나 메타데이터에 접근 가능
// #define PUT(p, val)     (*(unsigned int *)(p) = (val))  //블록의 주소를 담는다. 위치를 담아야지 나중에 헤더나 푸터를 읽어서 연결할 수 있다.
//                                                         // 새로운 메모리 블럭을 할당하거나 기존 블록의 상태를 변경할때 사용
// /* p의 위치로부터 size를 읽고 field를 할당 */
// #define GET_SIZE(p)     (GET(p) & ~0x7)                 //메모리의 사이즈를 얻기 위한 메크로 0x07은 ~ 연산자로 상위비트만 가지고 있는 값이 되고 이를 and연산해서 하위 비트를 날린다
// #define GET_ALLOC(p)    (GET(p) & 0x01)                 // 반대로 최하위 비트만 알기 위한 연산

// #define HDRP(bp)        ((char *) (bp) - WSIZE)                         // 헤더 포인터 보통 bp는 페이로드를 가리키고 있음으로 4바이트 만큼 뒤로 가게한다.
// #define FTRP(bp)        ((char *) (bp) + GET_SIZE(HDRP(bp)) - DSIZE)    // 푸터 포인터 bp에서 전체 크기만큼 이동하고 DSIZE만큼(8바이트)만크 뒤로 이동해서 푸터의 시작 주소를 가리킨다.

// #define NEXT_BLKP(bp)   ((char *) (bp) + GET_SIZE(((char *)(bp) - WSIZE)))      // 현재 블록포인터를 기준으로 다음 블록의 시작주소 계산 다음 주소에서 헤더의 크기만큼 빼준다.
// #define PREV_BLKP(bp)   ((char *) (bp) - GET_SIZE(((char *)(bp) - WSIZE)))      // 현재 블록포인터를 기준으로 이전 블록포인터의 시작주소 계산  이전 주소에서 푸터의 크기만큼 빼준다

// static char *heap_listp;    //  처음에 쓸 가용 블록 힙을 만들어준다. -> 시작점 제공
#define WSIZE 4 // word and header footer 사이즈를 byte로. 
#define DSIZE 8 // double word size를 byte로
#define CHUNKSIZE (1<<12) // heap을 늘릴 때 얼만큼 늘릴거냐? 4kb 분량.

#define MAX(x,y) ((x)>(y)? (x) : (y)) // x,y 중 큰 값을 가진다.

// size를 pack하고 개별 word 안의 bit를 할당 (size와 alloc을 비트연산), 헤더에서 써야하기 때문에 만듬.
#define PACK(size,alloc) ((size)| (alloc)) // alloc : 가용여부 (ex. 000) / size : block size를 의미. =>합치면 온전한 주소가 나온다.

/* address p위치에 words를 read와 write를 한다. */
#define GET(p) (*(unsigned int*)(p)) // 포인터를 써서 p를 참조한다. 주소와 값(값에는 다른 블록의 주소를 담는다.)을 알 수 있다. 다른 블록을 가리키거나 이동할 때 쓸 수 있다. 
#define PUT(p,val) (*(unsigned int*)(p)=(int)(val)) // 블록의 주소를 담는다. 위치를 담아야지 나중에 헤더나 푸터를 읽어서 이동 혹은 연결할 수 있다.

// address p위치로부터 size를 읽고 field를 할당
#define GET_SIZE(p) (GET(p) & ~0x7) // '~'은 역수니까 11111000이 됨. 비트 연산하면 000 앞에거만 가져올 수 있음. 즉, 헤더에서 블록 size만 가져오겠다.
#define GET_ALLOC(p) (GET(p) & 0x1) // 00000001이 됨. 헤더에서 가용여부만 가져오겠다.

/* given block ptr bp, header와 footer의 주소를 계산*/
#define HDRP(bp) ((char*)(bp) - WSIZE) // bp가 어디에있던 상관없이 WSIZE 앞에 위치한다.
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // 헤더의 끝 지점부터 GET SIZE(블록의 사이즈)만큼 더한 다음 word를 2번빼는게(그 뒤 블록의 헤드에서 앞으로 2번) footer의 시작 위치가 된다.

/* GIVEN block ptr bp, 이전 블록과 다음 블록의 주소를 계산*/
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp)-WSIZE))) // 그 다음 블록의 bp 위치로 이동한다.(bp에서 해당 블록의 크기만큼 이동 -> 그 다음 블록의 헤더 뒤로 감.)
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE))) // 그 전 블록의 bp위치로 이동.(이전 블록 footer로 이동하면 그 전 블록의 사이즈를 알 수 있으니 그만큼 그 전으로 이동.)
static char *heap_listp;  // 처음에 쓸 큰 가용블록 힙을 만들어줌.
////////////////////////////////////
    /* 초기 힙구조 설정 */

int mm_init(void){
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

/* create 초기 빈 heap*/
/* int mm_init(void){ // 처음에 heap을 시작할 때 0부터 시작. 완전 처음.(start of heap)
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1){ // old brk에서 4만큼 늘려서 mem brk로 늘림.
        return -1;
    }
    PUT(heap_listp,0); // 블록생성시 넣는 padding을 한 워드 크기만큼 생성. heap_listp 위치는 맨 처음.
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // prologue header 생성. pack을 해석하면, 할당을(1) 할건데 8만큼 줄거다(DSIZE). -> 1 WSIZE 늘어난 시점부터 PACK에서 나온 사이즈를 줄거다.)
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); // prologue footer생성.
    PUT(heap_listp + (3*WSIZE), PACK(0,1)); // epilogue block header를 처음에 만든다. 그리고 뒤로 밀리는 형태.
    heap_listp+= (2*WSIZE); // prologue header와 footer 사이로 포인터로 옮긴다. header 뒤 위치. 다른 블록 가거나 그러려고.

    if (extend_heap(CHUNKSIZE/WSIZE)==NULL) // extend heap을 통해 시작할 때 한번 heap을 늘려줌. 늘리는 양은 상관없음.
        return -1;
    return 0;
} */
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

// static void *extend_heap(size_t words){ // 새 가용 블록으로 힙 확장, 예시로는 2의 10승만큼 워드블록을 만들어라.
//     char *bp;
//     size_t size; // size+t = unsigned int, 이 함수에서 넣을 size를 하나 만들어줌.
//     /* alignment 유지를 위해 짝수 개수의 words를 Allocate */
//     size = (words%2) ? (words+1) * WSIZE : words * WSIZE; // 홀수이면(1이나오니까) 앞에꺼, 짝수이면(0이 나오니까) 뒤에꺼. 홀수 일 때 +1 하는건 8의 배수 맞추기 위함인듯.
//     // 홀수가 나오면 사이즈를 한번 더 재정의. 힙에서 늘릴 사이즈를 재정의.
//     if ( (long)(bp = mem_sbrk(size)) == -1){ // sbrk로 size로 늘려서 long 형으로 반환한다.(한번 쫙 미리 늘려놓는 것) mem_sbrk가 반환되면 bp는 현재 만들어진 블록의 끝에 가있음.
//         return NULL;
//     } // 사이즈를 늘릴 때마다 old brk는 과거의 mem_brk위치로 감.

//     /* free block 헤더와 푸터를 init하고 epilogue 헤더를 init*/
//     PUT(HDRP(bp), PACK(size,0)); // free block header 생성. /(prologue block이랑 헷갈리면 안됨.) regular block의 총합의 첫번째 부분. 현재 bp 위치의 한 칸 앞에 헤더를 생성.
//     PUT(FTRP(bp),PACK(size,0)); // free block footer / regular block 총합의 마지막 부분.
//     PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); // 블록을 추가했으니 epilogue header를 새롭게 위치 조정해줌. (HDRP: 1워드 뒤에 있는 것을 지칭. 
//     // 처음 세팅의 의미 = bp를 헤더에서 읽은 사이즈만큼 이동하고, 앞으로 한칸 간다. 그위치가 결국 늘린 블록 끝에서 한칸 간거라 거기가 epilogue header 위치.

//     /* 만약 prev block이 free였다면, coalesce해라.*/
//     return coalesce(bp); // 처음에는 coalesc를 할게 없지만 함수 재사용을 위해 리턴값으로 선언
// }


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
        size+= GET_SIZE(HDRP(PREV_BLKP(bp)));               // 이전 블럭의 헤더를보고 그 크기만큼 블록의 사이즈에 할당
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

/* static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); // 그전 블록으로 가서 그 블록의 가용여부를 확인
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); // 그 뒷 블록의 가용 여부 확인.
    size_t size =  GET_SIZE(HDRP(bp)); // 지금 블록의 사이즈 확인

    if (prev_alloc && next_alloc){ // case 1 - 이전과 다음 블록이 모두 할당 되어있는 경우, 현재 블록의 상태는 할당에서 가용으로 변경
        return bp; // 이미 free에서 가용이 되어있으니 여기선 따로 free 할 필요 없음.
    }
    else if (prev_alloc && !next_alloc){ // case2 - 이전 블록은 할당 상태, 다음 블록은 가용상태. 현재 블록은 다음 블록과 통합 됨.
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))); // 다음 블록의 헤더를 보고 그 블록의 크기만큼 지금 블록의 사이즈에 추가한다.
        PUT(HDRP(bp),PACK(size,0)); // 헤더 갱신(더 큰 크기로 PUT)
        PUT(FTRP(bp), PACK(size,0)); // 푸터 갱신
    }
    else if(!prev_alloc && next_alloc){ // case 3 - 이전 블록은 가용상태, 다음 블록은 할당 상태. 이전 블록은 현재 블록과 통합. 
        size+= GET_SIZE(HDRP(PREV_BLKP(bp))); 
        PUT(FTRP(bp), PACK(size,0));  // 푸터에 먼저 조정하려는 크기로 상태 변경한다.
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0)); // 현재 헤더에서 그 앞블록의 헤더 위치로 이동한 다음에, 조정한 size를 넣는다.
        bp = PREV_BLKP(bp); // bp를 그 앞블록의 헤더로 이동(늘린 블록의 헤더이니까.)
    }
    else { // case 4- 이전 블록과 다음 블록 모두 가용상태. 이전,현재,다음 3개의 블록 모두 하나의 가용 블록으로 통합.
        size+= GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))); // 이전 블록 헤더, 다음 블록 푸터 까지로 사이즈 늘리기
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0)); // 헤더부터 앞으로 가서 사이즈 넣고
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0)); // 푸터를 뒤로 가서 사이즈 넣는다.
        bp = PREV_BLKP(bp); // 헤더는 그 전 블록으로 이동.
    }
    return bp; // 4개 케이스중에 적용된거로 bp 리턴
} */

/* static void *find_fit(size_t size){
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
} */
static void *find_fit(size_t asize){ // first fit 검색을 수행
    void *bp;
    for (bp= heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){ // init에서 쓴 heap_listp를 쓴다. 처음 출발하고 그 다음이 regular block 첫번째 헤더 뒤 위치네.
        // for문이 계속 돌면 epilogue header까기 간다. epilogue header는 0이니까 종료가 된다.
        if (!GET_ALLOC(HDRP(bp)) && (asize<=GET_SIZE(HDRP(bp)))){ // 이 블록이 가용하고(not 할당) 내가 갖고있는 asize를 담을 수 있으면
            return bp; // 내가 넣을 수 있는 블록만 찾는거니까, 그게 나오면 바로 리턴.
        }
    }
    return NULL;  // 종료 되면 null 리턴. no fit 상태.
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
/* void *mm_realloc(void *ptr, size_t size)
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

 */
void *mm_realloc(void *bp, size_t size){
    if(size <= 0){ 
        mm_free(bp);
        return 0;
    }
    if(bp == NULL){
        return mm_malloc(size); 
    }
    void *newp = mm_malloc(size); 
    if(newp == NULL){
        return 0;
    }
    size_t oldsize = GET_SIZE(HDRP(bp));
    if(size < oldsize){
    	oldsize = size; 
	}
    memcpy(newp, bp, oldsize); 
    mm_free(bp);
    return newp;
}