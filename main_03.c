#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define PAGE_ENTRY 256 //page table의 entry 2^8
#define PAGE 256// Page size = 2^8
#define TLB_ENTRY 16 //16 TLB 엔트리
#define P_FRAME 128 // Physical frames = 128

typedef union
{
    unsigned short int Add;  //2바이트의 Address를 저장함
    struct  // union을 사용하며 offset과 page를 1바이트씩 가지게 한다.
    {
        unsigned char offset;    // offset = 8bit
        unsigned char page;  // page = 8bit
    };
    
}address;

typedef struct list //stack형식
{
    unsigned char page;  // page = 8bit
    unsigned char frame; // frame = 8bit
    struct list *next; // 다음꺼 가리킨다.
}list;

typedef struct TLB_entry //stack형식
{
    int lastUseTime;    // LRU형식으로 TLB를 update하기 위해
    unsigned char page;  // page = 8bit
    unsigned char frame; // frame = 8bit
    bool valid;
}TLB_entry;

void set_TLB_ENTRY(TLB_entry *tlb_entry, int time, unsigned char page, unsigned char frame)
{
    tlb_entry->lastUseTime = time;  //언제 사용 되었는지 저장
    tlb_entry->page = page; //page번호
    tlb_entry->frame = frame;   //frame 번호
    tlb_entry->valid = true;    //사용 됐다면 valid 하다고 함
}

TLB_entry TLB_TABLE[TLB_ENTRY]; //PageTable의 일부 = TLB에 저장한다.

int find_TLB_TABLE(unsigned char page, int num) //TLB에 해당 page가 존재 하는지 확인하는 함수
{
    int i;
    for(i = 0; i < TLB_ENTRY; i++)
    {
        if(!TLB_TABLE[i].valid) break;
        if(TLB_TABLE[i].page == page)   //해당 page가 존재한다면
        {
            TLB_TABLE[i].lastUseTime = num; //언제 썼는지 update 해줌
            return i;                       //i번째에 해당 page가 존재한다.
        }
    }
    return -1;  //존재하지 않는다면 -1을 return 해줌
}

int find_first_TLB()    //TLB에서 가장 최근에 쓰이지 않는  page를 찾는다.
{
    int i = 0, ret = 0;
    int firstTime = TLB_TABLE[i].lastUseTime;
    for(i = 1; i < TLB_ENTRY; i++)
    {
        if(firstTime > TLB_TABLE[i].lastUseTime)
        {
            firstTime = TLB_TABLE[i].lastUseTime;   //가장 오래전에 쓰였다면 lastUseTime의 값이 작기때문에 그걸로 update 해준다.
            ret = i;    //해당 위치를 저장함
        }
    }
    return ret;
}


int TLB_HIT = 0, PAGE_Fault = 0, STACK_SIZE = 0;    //TLB_HIT,PAGE_Fault,STACK_SIZE
char PAGE_TABLE[PAGE_ENTRY];    //page_table을 만들어줌
list *LRU_FRAME, *LRU_BEGIN, *LRU_END;  //LRU형식으로 가장 최근에 쓰이지 않는 page를 out시킨다.

void push(unsigned char page, unsigned char frame)  //LRU에 새로운 값을 넣는다.
{
    list *NEWEND = (list*)malloc(sizeof(list)); //다시 끝(TOP)이라고 지정할 list를 만든다.
    PAGE_TABLE[page] = frame;   //pagetable의 page번째에 frame값을 저장함
    LRU_END->page = page;   //가장 최근에 들어온 것이기 때문에 stack의 가장 끝(TOP)에 들어간다.
    LRU_END->frame = frame; //frame 값을 저장한다.
    LRU_END->next = NEWEND; //새롭게 만든 빈 list를 END의 next에 단다
    LRU_END = LRU_END->next;    //END의 값을 바꿔준다
}

char pop()//가장 최근에 쓰이지 않은 (stack의 가장 바닥에 있는) list를 삭제한다.
{
    list *temp = LRU_BEGIN->next;   //Begin은 빈 list이다 그 다음에 있는 것 = 가장최근에 쓰이지 않은 것을 삭제 함
    unsigned char cframe = PAGE_TABLE[temp->page];
    PAGE_TABLE[temp->page] = -1;    //pageTable에서도 삭제 함 -1 로 바꿔줌
    
    LRU_BEGIN->next = temp->next;   //가장 최근에 쓰이지 않은 것을 지금 삭제 할 다음 것이라고 지정
    free(temp); //메모리 할당을 해제해줌
    return cframe;  //그리고 frame num 반환
}

char find_and_push(unsigned char page)  //LRU stack list 에 해당 page의 위치를 찾고 frame값을 지정하고 LRU에 넣어줌.
{
    unsigned char cframe;   // 존재를 한다면 frame의 위치를 반환한다.
    list *prev = LRU_BEGIN;     //그 전 위치
    list *temp = LRU_BEGIN->next;   //현재 위치
    while(temp != LRU_END)  //LRU stack list에 존재 하는지 확인함
    {
        if(temp->page == page)  //해당 page를 찾으면
        {
            cframe = temp->frame;   //그 frame값을 저장하고,
            prev->next = temp->next;    //자신의 전 list가 나의 다음 것을 가리키고
            free(temp); //해당 list를 LRU에서 삭제 하고 반복문을 탈출
            break;
        }
        prev = prev->next;
        temp = temp->next;
    }                           //LRU에 존재하지 않는다면 끝에 할당, page를 찾으면 삭제하고 끝에 할당!
    push(page, cframe);
    return cframe;
}

int main(int argc, char *argv[])
{
    freopen("Physical.txt","w+",stdout);
    unsigned int i, frame = 0;
    unsigned int num=0;
    memset(PAGE_TABLE, -1, sizeof(PAGE_TABLE)); //page table을 -1로 초기화함.
    LRU_BEGIN = (list*)malloc(sizeof(list));
    LRU_END = (list*)malloc(sizeof(list));
    LRU_BEGIN->next = LRU_END;
    
    for(i = 0; i < TLB_ENTRY; i++)              // TLB_TABLE을 초기화 시킴
    {
        set_TLB_ENTRY(&TLB_TABLE[i], i - TLB_ENTRY, -1, -1);
        TLB_TABLE[i].valid = false;
    }
    
    FILE *fin;
    fin = fopen(argv[1], "r");  //파일 입출력을 통해 해당 파일에서 값을 불러들임
    
    address vPage, pPage;   //virtual page, physical page
    int TLB_TABLE_IDX;  //TLB의 몇번째에 존재하는지
    
    while(fscanf(fin, "%d", &vPage.Add) != EOF) //virtual address를 읽어 들임
    {
        num++;  //num =virtual address가 몇개 들어왔는지!! num이 작을수록 오래 전에 들어온 주소임
        
        pPage.offset = vPage.offset;    //offset의 값은 같다.
        if((TLB_TABLE_IDX = find_TLB_TABLE(vPage.page, num)) != -1)   //TLB hit 발생 (TLB에 존재하면)
        {
            TLB_HIT++; // hit값 증가
            pPage.page = TLB_TABLE[TLB_TABLE_IDX].frame;    //TLB_TABLE이 가지고 있는 page의 frame값을 지정함
            find_and_push(vPage.page);  //LRU를 update함
        }
        
        else    //TLB에서 찾지 못함
        {
            //TLB table을 갱신 시킨다
            int idx = find_first_TLB();
            
            if(PAGE_TABLE[vPage.page] == -1) //frame이 할당되지 않았을 때, pagefault 발생
            {
                PAGE_Fault++;   //fault값 증가
                if(STACK_SIZE < P_FRAME)    //비어있는 프레임이 있으면
                    STACK_SIZE++;   //stacksize를 증가 시킨다.
                
                else   //비어있는 프레임이 존재 하지 않는다면
                    frame = pop();  //가장 오래전에 사용된 page를 삭제 하고 해당 frame을 할당해준다.
                
                push(vPage.page, frame++);  //그리고 LRU에 새로운 page를 넣어줌
            }
            
            else
                find_and_push(vPage.page);  //frame이 할당은 되있다. PAGE_TABLE에서 값이 존재함 = 그 해당 page를 넣어줌

            pPage.page = PAGE_TABLE[vPage.page];    //해당 virtual pagenum에 해당하는 physical pagenum을 넣음
            set_TLB_ENTRY(&TLB_TABLE[idx], num, vPage.page, pPage.page);    //TLB를 갱신 시킴!
        }
        
        printf("Virtual address : %d Physical address : %d\n", vPage.Add, pPage.Add);   //virtual address, physical address 출력
    }
    
    printf("TLB hit ratio : 총 (%d) 중 (%d) hit 했음\n",num,TLB_HIT);
    printf("LRU hit ratio : 총 (%d) 중 (%d) hit 했음\n",num,PAGE_Fault);
}