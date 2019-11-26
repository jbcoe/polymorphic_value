#include <polymorphic_value.h>
#include <iostream>

using namespace jbcoe;

class BaseType {
public:
    virtual ~BaseType() = default;
    virtual std::string message() const = 0;
};

class DerivedType : public BaseType {
public:
    DerivedType() = default;
private:
    std::string message() const override { return "Calling the derived class's message function"; }
};

int main()
{
    polymorphic_value<BaseType> p = make_polymorphic_value<BaseType, DerivedType>();
    std::cout << p->message() << std::endl;
    return EXIT_SUCCESS;
}