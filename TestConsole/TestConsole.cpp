#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

int main()
{
    for (float t = 0; t <= 1; t += 0.05f)
    {
        std::cout << (1 - cosf(t * M_PI)) * 0.5f << "\n";
    }

    std::cout << "\n------\n\n";

    for (float t = 0; t <= 1; t += 0.05f)
    {
        std::cout << 1 - ((cosf(t * M_PI)) + 1) * 0.5f << "\n";
    }
}