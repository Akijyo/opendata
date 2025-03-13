#include "include/fileio.h"
using namespace std;

int main()
{
    wtfile wr;
    wr.open("/temp/aaa/test222.txt");
    wr.writeLine("date=%s", "2021-01-01");
    wr << "hello world"<<"你好";
    wr.close();

    // rdfile rd;
    // rd.open("/temp/aaa/test.txt");
    // string buf;
    // while(rd.readLine(buf))
    // {
    //     cout << buf << endl;
    //     buf.clear();
    // }
    // rd.close();
    return 0;
}