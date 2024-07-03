#include <stdio.h>
#include <iostream>
#include "prot.hpp"

int main(int, char**){
    printf("Hello, from Virtual-IED!\n");

    std::cout << "Hello, from Virtual-IED!" << std::endl;

    mult(5, 6);

    return 0;
}
