#include "Singleton.h"
#include <iostream>

using namespace std;
using namespace tts;

class TestClass : public Singleton<TestClass>
{
public:
    TestClass()
    {
        cout << "Construct TestClass." << endl;
    }
    ~TestClass()
    {
        cout << "Destruct TestClass." << endl;
    }
    void Echo()
    {
        cout << "Echo." << endl;
    }
};

// 测试一下定义局部变量为静态变量
void TestStatic()
{
    static int a = 0;
    cout << "static a: " << a++ << endl;
}

int main()
{
    // TestClass多次调用Inst只会被构造一次
    TestClass::Inst().Echo();
    TestClass::Inst().Echo();
    TestClass::Inst().Echo();
    TestStatic();
    TestStatic();
    static int a = 0;
    // 局部定义的静态变量作用范围只在函数内部
    cout << "static main a: " << a++ << endl;
    TestStatic();
    return 0;
}