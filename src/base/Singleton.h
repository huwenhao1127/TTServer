#pragma once
#include "Noncopyable.h"

template <class ClassType>
class Singleton : public Noncopyable
{
public:
    // 成员函数内定义静态变量,该类的所有对象在调用该成员函数时都会共享这个变量
    static ClassType& Inst()
    {
        static ClassType staticInst;
        return staticInst;
    }
};
