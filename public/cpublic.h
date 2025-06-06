//
// Created by akijyo on 25-3-1.
// 这是一个共用的头文件
// 包含了c/c++一些常用的系统头文件，
// 所有需要的系统头文件都在此文件中包含
#pragma once

// c风格系统头文件
#include <stdio.h>
#include <utime.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <locale.h>
#include <dirent.h>
#include <termios.h>
#include <pthread.h>
#include <stddef.h>
#include <poll.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/timerfd.h>
#include <sys/signalfd.h>

// c++风格系统头文件
#include <atomic>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <forward_list>
#include <vector>
#include <deque>
#include <memory>
#include <mutex> // 互斥锁类的头文件。
#include <shared_mutex>
#include <queue>              // queue容器的头文件。
#include <condition_variable> // 条件变量的头文件。
#include <algorithm>
#include <thread> // 线程类头文件。
#include <regex>  // 正则表达式头文件。
#include <chrono> // 时间头文件。
#include <filesystem>
#include <complex>
#include <type_traits>
#include <future>
#include <functional>
#include "nlohmann/json.hpp"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Infos.hpp"
#include "curlpp/Exception.hpp"
