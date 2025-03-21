#include "crtsurfdata.h"
using namespace std;

// 数据时间的字符串全局变量
string strdatatime = getCurTime(TimeType::TIME_TYPE_TWO) + ":00";

list<st_stcode> stcodelist; // 存放站点参数的容器

list<st_surfdata> surfdatalist; // 保存站点观测数据的列表

//创建日志对象的全局变量
logfile lg;


// 把站点文件参数加载到容器中
bool loadstcode(const string &inifile)
{
    // 创建文件读取对象
    rdfile rf;
    if (rf.open(inifile) == false)
    {
        lg.writeLine("打开站点参数文件失败：%s", inifile.c_str());
        return false;
    }
    // 读掉标题类
    string temp;
    // rf.readLine(temp);

    ccmdstr spl;
    struct st_stcode stcode;

    while (rf.readLine(temp))
    {
        // lg.writeLine("stcode:%s\n", temp.c_str());
        // cout<<temp<<endl;
        spl.split(temp, ",", true);

        memset(&stcode, 0, sizeof(stcode));

        spl.getValue(0, stcode.provname);
        spl.getValue(1, stcode.obtid);
        spl.getValue(2, stcode.obtname);
        spl.getValue(3, stcode.lat);
        spl.getValue(4, stcode.lon);
        spl.getValue(5, stcode.alt);
        stcodelist.push_back(stcode);
    }

    // for (auto &it : stcodelist)
    // {
    //     lg.writeLine("provname:%s, obtid:%s, obtname:%s, lat:%f, lon:%f, alt:%f\n", it.provname.c_str(),
    //                  it.obtid.c_str(), it.obtname.c_str(), it.lat, it.lon, it.alt);
    // }
    return true;
}

// 生成站点观测数据
void crtsurfdata()
{
    srand(time(0));

    struct st_surfdata surfdata; // 创建站点观测数据结构体

    // 获取当前时间，给予全局变量值
    getCurTime(strdatatime, TimeType::TIME_TYPE_TWO);
    strdatatime += ":00";

    for (auto &it : stcodelist)
    {
        memset(&surfdata, 0, sizeof(surfdata));
        surfdata.obtid = it.obtid;
        surfdata.datetime = strdatatime;
        surfdata.temperature = rand() % 600 - 200;
        surfdata.pressure = rand() % 2650 + 10000;
        surfdata.humidity = rand() % 100;
        surfdata.winddir = rand() % 360;
        surfdata.windspeed = rand() % 100;
        surfdata.rain = rand() % 600;
        surfdata.vis = rand() % 10000 + 100000;
        surfdatalist.push_back(surfdata);
    }
    // for (auto &it : surfdatalist)
    // {
    //     lg.writeLine("obtid:%s, datetime:%s, temperature:%d, pressure:%d, humidity:%d, winddir:%d, windspeed:%d, "
    //                  "rain:%d, vis:%d\n",
    //                  it.obtid.c_str(), it.datetime.c_str(), it.temperature, it.pressure, it.humidity, it.winddir,
    //                  it.windspeed, it.rain, it.vis);
    // }
}

// 将观测数据写入文件.json
void crtsurffile(const string &outpath)
{
    // 构建文件名
    string filename;
    filename = outpath + "/CSTF_ZH_" + pickNum(strdatatime) + ".json";

    // 创建文件写入对象
    wtfile wf;
    if (wf.open(filename) == false)
    {
        lg.writeLine("打开文件失败：%s", filename.c_str());
        return;
    }

    // 构建json根对象
    nlohmann::json root;
    // 创建json子对象
    nlohmann::json data;
    for (auto &it : surfdatalist)
    {
        data["obtid"] = it.obtid;
        data["datetime"] = it.datetime;
        data["temperature"] = it.temperature / 10.0;
        data["pressure"] = it.pressure / 10.0;
        data["humidity"] = it.humidity;
        data["winddir"] = it.winddir;
        data["windspeed"] = it.windspeed / 10.0;
        data["rain"] = it.rain / 10.0;
        data["vis"] = it.vis / 10.0;
        // 将子对象添加到根对象中
        root.push_back(data);
    }

    // 将json对象写入文件
    string str = root.dump(4);
    wf << str << "\n";
    wf.close();

    lg.writeLine("生成文件：%s", filename.c_str());
}