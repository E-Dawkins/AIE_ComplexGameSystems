#include <iostream>
#include <vector>

int main()
{
    std::vector<const char*> test;
    test.push_back("Eat");
    test.push_back("tea");

    const char* e = "Eat";
}