#ifdef _WIN32
#include <windows.h>
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){Sleep(7000);return 0;}
#else
#include <chrono>
#include <thread>
int main(){std::this_thread::sleep_for(std::chrono::seconds(7));return 0;}
#endif
