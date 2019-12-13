#include <iostream>
#define MAXSTEP 1000
using namespace std;

int main(){
    int meter;
    cout << "请输入游泳池长度：";
    cin >> meter;
    if (meter <= 0 || meter >= 100) {
        cout << "长度不可用";
        return 1;
    } 
    // 米数
    int sum = 0;
    // 步数
    int step = 0;
    int current = 2;

    for (; step < MAXSTEP; step++) {
        sum = sum + current*RATIO;
        if (sum >= meter) {
            break;
        } else {
            continue;
        }
    }
    cout << "共需要" << step << "步" << endl;
    return 0;
}