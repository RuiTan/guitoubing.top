#include <iostream>
#define L 100
using namespace std;

// // const指的是将这个量定义为常量，也就是说，它不能变了
// const int a = 10;
// int b = 10;

struct T{
    int next;
    char e;
};

int main()
{ 
    T t[L];
    // 通过数组模拟线性表
    int head = 2;
    t[0].e = 'e';
    t[0].next = 1;
    t[1].e = 'l';
    t[1].next = 3;
    t[2].e = 'h';
    t[2].next = 0;
    t[3].e = 'l';
    t[3].next = 4;
    t[4].e = 'o';
    t[4].next = -1;

    int i = head;
    while (1){
        cout << t[i].e;
        i = t[i].next;
        if (i == -1)
            break;
    }
 
}