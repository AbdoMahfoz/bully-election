#include "controller/controller.h"
#include "unit/unit.h"

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        controller::main();
    }
    else
    {
        unit::main(std::stoi(argv[1]));
    }
}