#include "../BitWorks/Expression.h"
#include <iostream>
#include <string>
#include <string_view>
int main()
{
    int failures = 0;
    auto check = [&](std::string_view text, int base, int width, bool sign, std::uint32_t expected)
    {
        try
        {
            if (bitwork::Expression(text, base, width, sign).evaluate() != expected)
            { std::cerr << "Wrong result: " << text << '\n'; ++failures; }
        }
        catch (const std::exception& e) { std::cerr << text << ": " << e.what() << '\n'; ++failures; }
    };
    auto reject = [&](std::string_view text)
    {
        try { bitwork::Expression(text, 10, 32, false).evaluate(); ++failures; std::cerr << "Accepted: " << text << '\n'; }
        catch (const std::runtime_error&) {}
    };
    check("16*2",10,32,false,32);
    check("0x80 & 0x01",16,8,false,0);
    check("2+3*4",10,32,false,14);
    check("(2+3)*4",10,32,false,20);
    check("1 << 4 + 1",10,32,false,32);
    check("8-3-2",10,32,false,3);
    check("255+1",10,8,false,0);
    check("~0",10,16,false,65535);
    check("-1",10,32,true,UINT32_MAX);
    check("-128 >> 1",10,8,true,192);
    check("128 >> 1",10,8,false,64);
    check("-7 / 2",10,8,true,253);
    check("-7 % 2",10,8,true,255);
    check("-2147483648 / -1",10,32,true,2147483648u);
    check("FFFFFFFF*FFFFFFFF",16,32,false,1);
    check("10",16,8,false,16);
    check("0b1010 | 0x01",10,8,false,11);
    check("1|2^3&1",10,32,false,3);
    reject(""); reject("1/0"); reject("1%0"); reject("1<<32"); reject("1<<-1");
    reject("4294967296"); reject("1.5"); reject("(1+2"); reject("1+2)"); reject("1&&2"); reject("0x");
    reject(std::string(65,'(') + "1" + std::string(65,')'));
    std::cout << (failures ? "FAILED" : "All expression tests passed") << '\n';
    return failures ? 1 : 0;
}

