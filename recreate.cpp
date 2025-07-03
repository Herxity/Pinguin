#include <map> //For creating a function - route map
#include <iostream> //Basic print functions
#include <functional> //Basic print functions
#include <any>

class Container{
    public:
        std::map<int,Container> routes;
        std::map<int,std::function<void ()>> methods;
};

std::map<int,Container> routes;
void add_route(std::function<void ()> func){
    routes[0] = *new Container();
    routes[0].routes[0] = *new Container();
    routes[0].routes[0].methods[0] = func;
}

int main(){
    add_route([](){
        printf("HI\n");
    });
    if(routes[0].routes[0].methods[0]){
        routes[0].routes[0].methods[0]();
    }
    
    return 0;
}