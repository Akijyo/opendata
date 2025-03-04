//
// Created by akijyo on 25-3-1.
//

#include "stringop.h"
#include <string>
using namespace std;

int main()
{
    jsonns j("test.json");
    jsonns j1(j, "subjects");
    string value;
    j1.getForArr(0, value);
    cout << value << endl;
    return 0;
}