//
// Created by akijyo on 25-3-1.
//

#include "stringop.h"
#include "jsonns.h" 
using namespace std;

int main()
{
    string str;
    string name = "akijyo";
    int age = 20;
    double score = 99.55555;
    sFomat(str, "name:%s, age:%d, score:%.1f", name.c_str(), age, score);
    cout << str << endl;
    return 0;
}