#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <malloc.h>

#define UPPER_LIMIT  92
#define Mod 10000

typedef struct {
    long arr[100][100];
} Matrix;

typedef struct {
    double arr[1000];
} double_array;

/**
 * @brief Returns the fibonacci number given by index, reset will
 *        reset n-1 and n-2
 */
static uint64_t calculateFibonacci(uint8_t index, bool reset)
{
    /** initialise variables */
    static uint64_t fibNumber = 0;
    static uint64_t first     = 0;
    static uint64_t second    = 1;

    /** reset the variables since they are static */
    if (reset)
    {
        uint64_t fibNumber = 0;
        uint64_t first     = 0;
        uint64_t second    = 1;
    }

    /** verify input is valid */
    if (index > UPPER_LIMIT)
    {
        fibNumber = 0;
        return fibNumber;
    }

    /** 0 or 1 return themselves */
    if (index <= 1)
    {
        fibNumber = index;
    }

    /** calculate the number (n-1 + n-2) */
    else
    {
        fibNumber = first + second;
        first  = second;
        second = fibNumber;
    }

    return fibNumber;
}

/**
 * @brief Returns true if the given number is a prime number
 *        greater than 1
 */
static bool isPrime(uint64_t toCheck)
{
    /** 0 or 1 does not count */
    if (toCheck <= 1)
    {
        return false;
    }

    /** any event number cannot be prime */
    if (toCheck % 2 == 0)
    {
        return false;
    }

    /** only need to check half the points */
    uint64_t upperLimit = toCheck / 2;

    for (uint64_t i = 3; i < upperLimit; i += 2)
    {
        if (toCheck % i == 0)
        {
            return false;
        }
    }

    return true;
}

double_array fpeak(double_array a, double_array b)
{
    int arr_len = sizeof(a.arr)/sizeof(a.arr[0]);
    double_array sum;
    memset(sum.arr, 0, sizeof(sum.arr));
    for(int i = 0; i < arr_len; i++) {
        sum.arr[i] = a.arr[i] + b.arr[i];
    }
    return sum;
}

/*multiply the two matrices*/
static Matrix mMulti(Matrix a, Matrix b)
{
    int i, j, k;
    Matrix d;
    memset(d.arr,0,sizeof(d.arr));
    for(i=0; i<100; i++)
        for(j=0; j<100; j++)
        {
            for(k=0; k<100; k++)
                d.arr[i][j]+=a.arr[i][k]*b.arr[k][j];
            d.arr[i][j]%=Mod;
        }
    return d;
}

/*the k-th power of matrix a*/
Matrix mPow(Matrix a, int k)
{
    Matrix modd,meven;

    meven=a;                             //matrix meven is initialized

    if(k==0)
        return modd;
    else
    {
        while(k!=1)
        {
            if(k&1)
            {
                k--;
                modd=mMulti(modd,meven);
            }
            else
            {
                k/=2;
                meven=mMulti(meven,meven);
            }
        }
        return mMulti(modd,meven);
    }
}

double pi()
{
    double pi=2;
    int  i;

    for(i=10000; i>=1; i--)
        pi=pi*(double)i/(2*i+1)+2;
    return pi;
}

void stringsort()
{
    char str[500];
    int i,j,len;
    printf("请输入字符串:");
    scanf("%s",str);

    len=strlen(str);//计算你输入的字符串的长度

    if(len>500)
    {
        puts("输入的字符大于500！重新输入：");
        scanf("%s",str);
    }
    printf("\n");
    printf("你输入的字符串是: %s\n",str);

    //冒泡排序（从小到大）
    for( i=0; i<len-1; i++)
        for( j=0; j<len-i-1; j++)
            if(*(str+j)>*(str+j+1))
            {
                char t=*(str+j);
                *(str+j)=*(str+j+1);
                *(str+j+1)=t;
            }

    printf("\n");
    printf("排序后的结果: %s\n",str);

}

typedef struct _Range {
    int start, end;
} Range;

Range new_Range(int s, int e) {
    Range r;
    r.start = s;
    r.end = e;
    return r;
}
static void swap(int *x, int *y) {
    int t = *x;
    *x = *y;
    *y = t;
}

void quick_sort(int arr[], const int len) {
    if (len <= 0)
        return; //避免len等於負值時宣告堆疊陣列當機
    //r[]模擬堆疊,p為數量,r[p++]為push,r[--p]為pop且取得元素
    Range r[len];
    int p = 0;
    r[p++] = new_Range(0, len - 1);
    while (p) {
        Range range = r[--p];
        if (range.start >= range.end)
            continue;
        int mid = arr[range.end];
        int left = range.start, right = range.end - 1;
        while (left < right) {
            while (arr[left] < mid && left < right)
                left++;
            while (arr[right] >= mid && left < right)
                right--;
            swap(&arr[left], &arr[right]);
        }
        if (arr[left] >= arr[range.end])
            swap(&arr[left], &arr[range.end]);
        else
            left++;
        r[p++] = new_Range(range.start, left - 1);
        r[p++] = new_Range(left + 1, range.end);
    }
}

void bubble_sort(int arr[], int len) {
    int i, j, temp;
    for (i = 0; i < len - 1; i++)
        for (j = 0; j < len - 1 - i; j++)
            if (arr[j] > arr[j + 1]) {
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
}

static void max_heapify(int arr[], int start, int end) {
    //建立父節點指標和子節點指標
    int dad = start;
    int son = dad * 2 + 1;
    while (son <= end) { //若子節點指標在範圍內才做比較
        if (son + 1 <= end && arr[son] < arr[son + 1]) //先比較兩個子節點大小，選擇最大的
            son++;
        if (arr[dad] > arr[son]) //如果父節點大於子節點代表調整完畢，直接跳出函數
            return;
        else { //否則交換父子內容再繼續子節點和孫節點比較
            swap(&arr[dad], &arr[son]);
            dad = son;
            son = dad * 2 + 1;
        }
    }
}

void heap_sort(int arr[], int len) {
    int i;
    //初始化，i從最後一個父節點開始調整
    for (i = len / 2 - 1; i >= 0; i--)
        max_heapify(arr, i, len - 1);
    //先將第一個元素和已排好元素前一位做交換，再從新調整，直到排序完畢
    for (i = len - 1; i > 0; i--) {
        swap(&arr[0], &arr[i]);
        max_heapify(arr, 0, i - 1);
    }
}

/*复数的定义*/
typedef struct
{
    double re;
    double im;
} COMPLEX;
/*复数的加运算*/
COMPLEX Add(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re + c2.re;
    c.im = c1.im + c2.im;
    return c;
}
/*负数的减运算*/
COMPLEX Sub(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re - c2.re;
    c.im = c1.im - c2.im;
    return c;
}
/*复数的乘运算*/
COMPLEX Mul(COMPLEX c1, COMPLEX c2)
{
    COMPLEX c;
    c.re = c1.re*c2.re - c1.im*c2.im;
    c.im = c1.re*c2.im + c1.im*c2.re;
    return c;
}

/*快速傅立叶变换
TD为时域值，FD为频域值，power为2的幂数*/
void FFT(COMPLEX *TD, COMPLEX *FD, int power)
{
    int count;
    int i,j,k,bfsize,p;
    double angle;
    COMPLEX *W,*X1,*X2,*X;
    /*计算傅立叶变换点数*/
    count=1<<power;
    /*分配运算器所需存储器*/
    W=(COMPLEX *)malloc(sizeof(COMPLEX)*count/2);
    X1=(COMPLEX *)malloc(sizeof(COMPLEX)*count);
    X2=(COMPLEX *)malloc(sizeof(COMPLEX)*count);
    /*计算加权系数*/
    for(i=0; i<count/2; i++)
    {
        angle = -i * 3.14159265359 * 2 / count;
        W[i].re=cos(angle);
        W[i].im=sin(angle);
    }
    /*将时域点写入存储器*/
    memcpy(X1, TD, sizeof(COMPLEX)*count);
    /*蝶形运算*/
    for(k=0; k<power; k++)
    {
        for(j=0; j<1<<k; j++)
        {
            bfsize=1<<power-k;
            for(i=0; i<bfsize/2; i++)
            {
                p=j*bfsize;
                X2[i+p]=Add(X1[i+p], X1[i+p+bfsize/2]);
                X2[i+p+bfsize/2]=Mul(Sub(X1[i+p], X1[i+p+bfsize/2]),W[i*(1<<k)]);
            }
        }
        X=X1;
        X1=X2;
        X2=X;
    }
    /*重新排序*/
    for(j=0; j<count; j++)
    {
        p=0;
        for(i=0; i<power; i++)
        {
            if(j&(1<<i))
                p+=1<<power-i-1;
        }
        FD[j]=X1[p];
    }
    /*释放存储器*/
    free(W);
    free(X1);
    free(X2);
}

/*快速傅立叶反变换，利用快速傅立叶变换
FD为频域值，TD为时域值，power为2的幂数*/
void IFFT(COMPLEX *FD, COMPLEX *TD, int power)
{
    int i,count;
    COMPLEX *x;
    /*计算傅立叶反变换点数*/
    count=1<<power;
    /*分配运算所需存储器*/
    x=(COMPLEX *)malloc(sizeof(COMPLEX)*count);
    /*将频域点写入存储器*/
    memcpy(x,FD,sizeof(COMPLEX)*count);
    /*求频域点的共轭*/
    for(i=0; i<count; i++)
    {
        x[i].im=-x[i].im;
    }
    /*调用快速傅立叶变换*/
    FFT(x,TD,power);
    /*求时域点的共轭*/
    for(i=0; i<count; i++)
    {
        TD[i].re/=count;
        TD[i].im=-TD[i].im/count;
    }
    /*释放存储器*/
    free(x);
}

int conf[10]; /* Element conf[d] gives the current position of disk d. */

void move(int d, int t) {
    /* move disk d to peg t */
    conf[d] = t;
}

void hanoi(int h, int t) {
    if (h > 0) {
        int f = conf[h-1];
        if (f != t) {
            int r = 3 - f - t ;
            hanoi(h-1, r);
            move(h-1, t);
        }
        hanoi(h-1, t);
    }
}

/*
    This polynomial ( 0xEDB88320L) DOES generate the same CRC values as ZMODEM and PKZIP
 */
#include <stdint.h>
static const uint32_t crc32tab[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL,
    0x076dc419L, 0x706af48fL, 0xe963a535L, 0x9e6495a3L,
    0x0edb8832L, 0x79dcb8a4L, 0xe0d5e91eL, 0x97d2d988L,
    0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L, 0x90bf1d91L,
    0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L,
    0x136c9856L, 0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL,
    0x14015c4fL, 0x63066cd9L, 0xfa0f3d63L, 0x8d080df5L,
    0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L, 0xa2677172L,
    0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L,
    0x32d86ce3L, 0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L,
    0x26d930acL, 0x51de003aL, 0xc8d75180L, 0xbfd06116L,
    0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L, 0xb8bda50fL,
    0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL,
    0x76dc4190L, 0x01db7106L, 0x98d220bcL, 0xefd5102aL,
    0x71b18589L, 0x06b6b51fL, 0x9fbfe4a5L, 0xe8b8d433L,
    0x7807c9a2L, 0x0f00f934L, 0x9609a88eL, 0xe10e9818L,
    0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL,
    0x6c0695edL, 0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L,
    0x65b0d9c6L, 0x12b7e950L, 0x8bbeb8eaL, 0xfcb9887cL,
    0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L, 0xfbd44c65L,
    0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL,
    0x4369e96aL, 0x346ed9fcL, 0xad678846L, 0xda60b8d0L,
    0x44042d73L, 0x33031de5L, 0xaa0a4c5fL, 0xdd0d7cc9L,
    0x5005713cL, 0x270241aaL, 0xbe0b1010L, 0xc90c2086L,
    0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L,
    0x59b33d17L, 0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL,
    0xedb88320L, 0x9abfb3b6L, 0x03b6e20cL, 0x74b1d29aL,
    0xead54739L, 0x9dd277afL, 0x04db2615L, 0x73dc1683L,
    0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L,
    0xf00f9344L, 0x8708a3d2L, 0x1e01f268L, 0x6906c2feL,
    0xf762575dL, 0x806567cbL, 0x196c3671L, 0x6e6b06e7L,
    0xfed41b76L, 0x89d32be0L, 0x10da7a5aL, 0x67dd4accL,
    0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L,
    0xd1bb67f1L, 0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL,
    0xd80d2bdaL, 0xaf0a1b4cL, 0x36034af6L, 0x41047a60L,
    0xdf60efc3L, 0xa867df55L, 0x316e8eefL, 0x4669be79L,
    0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL,
    0xc5ba3bbeL, 0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L,
    0xc2d7ffa7L, 0xb5d0cf31L, 0x2cd99e8bL, 0x5bdeae1dL,
    0x9b64c2b0L, 0xec63f226L, 0x756aa39cL, 0x026d930aL,
    0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L,
    0x92d28e9bL, 0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L,
    0x86d3d2d4L, 0xf1d4e242L, 0x68ddb3f8L, 0x1fda836eL,
    0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L, 0x18b74777L,
    0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L,
    0xa00ae278L, 0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L,
    0xa7672661L, 0xd06016f7L, 0x4969474dL, 0x3e6e77dbL,
    0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L, 0x37d83bf0L,
    0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L,
    0xbad03605L, 0xcdd70693L, 0x54de5729L, 0x23d967bfL,
    0xb3667a2eL, 0xc4614ab8L, 0x5d681b02L, 0x2a6f2b94L,
    0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL, 0x2d02ef8dL
};

uint32_t crc32( const unsigned char *buf, uint32_t size)
{
    uint32_t i, crc;
    crc = 0xFFFFFFFF;
    for (i = 0; i < size; i++)
        crc = crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    return crc^0xFFFFFFFF;
}




