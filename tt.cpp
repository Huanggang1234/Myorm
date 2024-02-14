#include<iostream>
#include<string>

#define take(...) #__VA_ARGS__


void print(auto args){
     std::cout<<args<<std::endl;
}


int main(){

    std::string str=take(1,2,3);
    std::cout<<str<<std::endl;

    print("dfg");

    return 0;
}
