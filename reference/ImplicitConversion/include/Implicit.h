#include <iostream>
class Person{
public:
    explicit Person(int age);
private:
    int m_age;
};
Person::Person(int age){
    this->m_age =age;
    std::cout<<"PersonAge:"<<this->m_age<<std::endl;
}
class Dog{
public:
    Dog(int age);
private:
    int m_age;
};
Dog::Dog(int age){
    this->m_age =age;
    std::cout<<"DogAge:"<<this->m_age<<std::endl;
}