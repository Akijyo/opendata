#include "timeframe.h"
#include <thread>
using namespace std;

int main()
{
    cout << getCurTime(TimeType::TIME_TYPE_ONE, 0) << endl;
    cout << getCurTime(TimeType::TIME_TYPE_FIVE, 0) << endl;
    cout << getCurTime(TimeType::TIME_TYPE_NINE, 0) << endl;
    cout << getCurTime(TimeType::TIME_TYPE_THIRTEEN, 30*60) << endl;
    cout << "------------------------------------------" << endl;
    cout << "timetostr1:" << timeToStr(1500000, TimeType::TIME_TYPE_ONE) << endl;
    cout << "timetostr2:" << timeToStr(1500000, TimeType::TIME_TYPE_FIVE) << endl;
    cout << "------------------------------------------" << endl;
    string now = getCurTime();
    cout<<"addtime1:"<<addTime(now, TimeType::TIME_TYPE_ONE,-(30 * 60))<<endl;
    cout << "addtime2:" << addTime(now, TimeType::TIME_TYPE_FIVE, 60 * 60) << endl;
    char nowc[100];
    getCurTime(nowc, TimeType::TIME_TYPE_ONE, 0);
    cout << "addtime3:" << addTime(nowc, TimeType::TIME_TYPE_ONE, 30 * 60) << endl;
    cout << "addtime4:" << addTime(nowc, TimeType::TIME_TYPE_FIVE, -(30 * 60)) << endl;

    cout << "----------------------计时器--------------------" << endl;
    timeCount tc;
    tc.startCount();
    this_thread::sleep_for(chrono::seconds(6));
    this_thread::sleep_for(chrono::milliseconds(560));
    cout<<"time:"<<tc.endCount()<<endl;
    return 0;
}