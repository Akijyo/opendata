#include "include/fileframe.h"
#include "include/cdir.h"
using namespace std;

int main()
{
    // if(newdir("/temp/test/2test/file.txt", true) == false)
    // {
    //     cout << "newdir failed" << endl;
    // }
    // if(renamefile("/temp/aaa/test.txt","/temp/bbb/test.txt")==false)
    // {
    //     cout << "rename failed" << endl;
    // }
    // if(renamefile("/temp/ccc", "/temp/aaa")==false)
    // {
    //     cout << "rename failed" << endl;
    // }
    // if(copyfile("/temp/bbb/test.txt", "/temp/aaa/test.txt")==false)
    // {
    //     cout << "copy failed" << endl;
    // }
    // if (copyfile("/temp/aaa/2aaa", "/temp/bbb/2aaa")==false)
    // {
    //     cout << "copy failed" << endl;
    // }
    // if (deletefile("/temp/bbb/testaaa.txt") == false)
    // {
    //     cout << "delete failed" << endl;
    // }
    // cout << "file size: " << fileSize("/temp/aaa/test.txt") << endl;
    // string time;
    // if (fileTime("/temp/aaa/test.txt", time, TimeType::TIME_TYPE_ONE) == false)
    // {
    //     cout << "fileTime failed" << endl;
    // }
    // cout << "file time: " << time << endl;

    // string modifyTime = "2021-07-01 12:00:00";
    // if(setFileTime("/temp/aaa/test.txt", modifyTime,TimeType::TIME_TYPE_ONE) == false)
    // {
    //     cout << "setFileTime failed" << endl;
    // }
    // string time;
    // if (fileTime("/temp/aaa/test.txt", time, TimeType::TIME_TYPE_ONE) == false)
    // {
    //     cout << "fileTime failed" << endl;
    // }
    // cout << "file time: " << time << endl;

    // cdir dir;
    // if (dir.openDir("/temp/aaa", ".*\\.txt\\b")==false)
    // {
    //     cout << "openDir failed" << endl;
    // }
    // while (dir.readFile())
    // {
    //     cout << "dir.filename: " << dir.filename << endl;
    //     cout << "dir.dirname: " << dir.dirname << endl;
    //     cout << "dir.fullpath: " << dir.fullpath << endl;
    //     cout << "dir.filesize: " << dir.filesize << endl;
    //     cout << "dir.modifytime: " << dir.modifytime << endl;
    //     cout << "dir.createtime: " << dir.createtime << endl;
    //     cout << "dir.accesstime: " << dir.accesstime << endl;
    // }

    cdir dir;
    if (dir.openDir("/temp/aaa", ".*\\.txt\\b",10000, true) == false)
    {
        cout << "openDir failed" << endl;
    }
    while (dir.readFile())
    {
        cout << "dir.filename: " << dir.filename << endl;
        cout << "dir.dirname: " << dir.dirname << endl;
        cout << "dir.fullpath: " << dir.fullpath << endl;
        cout << "dir.filesize: " << dir.filesize << endl;
        cout << "dir.modifytime: " << dir.modifytime << endl;
        cout << "dir.createtime: " << dir.createtime << endl;
        cout << "dir.accesstime: " << dir.accesstime << endl;
        cout<<"----------------------"<<endl;
    }
    return 0;
}