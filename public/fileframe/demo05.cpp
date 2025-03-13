#include "include/logfile.h"
#include <thread>

using namespace std;

void work(logfile &lf)
{
    for (int i = 0; i < 1000000; i++)
    {
        lf<< "this is a test " << i << endl;
    }
}

int main()
{
    logfile lf;
    if(lf.open("/temp/aaa/test.log") == false)
    {
        cout << "open file failed" << endl;
        return -1;
    }
    thread t1(work, ref(lf));
    //thread t2(work, ref(lf));
    t1.join();
    //t2.join();
    return 0;
}