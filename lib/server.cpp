#include "../include/pinguin.h"
#include <iostream> //Basic print functions
#include <unistd.h> //To close file descriptors
#include <sys/socket.h> //Socket functionality
#include <arpa/inet.h>  //To use the inet_addr function to convert from human readable IP addresses to integer for computer
#include <string> //String Processing
#include <sys/types.h>  //For accept4
#include <errno.h>//For catching accept4 errorrs
#include <csignal>//To handle cleanup during debugging.
#include <format>//Format std::strings
#include <ctime> //Logging
#include <map> //For creating a function - route map
#include <functional> //For passing in functions for mapping to certain routes/verbs
#include <regex>
#include <fstream>

//Colors headers from https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
#ifndef _COLORS_
#define _COLORS_

/* FOREGROUND */
#define RST  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[97m"
#define KGRY  "\x1B[90m" //This is 'light black' for some freaking reason.

#define BOLD(x) "\x1B[1m" x RST
#define UNDL(x) "\x1B[4m" x RST

#endif  /* _COLORS_ */

#define IP "127.0.0.1"
#define PORT 8080

volatile int exit_flag=0;


std::string COLOR(const std::string& text, const char* color_code) { //Because I grow weary of black and white.
    return std::string(color_code) + text + RST;
}

//HTTP REQUEST

HttpRequest::HttpRequest(std::string str) 
{  
    this->req_time = time(0);
    int next_break = 0;

    next_break = str.find(' ');
    std::string http_verb_str = str.substr(0,next_break);
    if(http_verb_str == "GET"){
        this->http_verb = HttpVerbs::GET;
    } else if(http_verb_str=="POST"){
        this->http_verb = HttpVerbs::POST;
    } else if(http_verb_str=="PUT"){
        this->http_verb = HttpVerbs::PUT;
    } else if(http_verb_str=="PATCH"){
        this->http_verb = HttpVerbs::PATCH;
    } else if(http_verb_str=="DELETE"){
        this->http_verb = HttpVerbs::DELETE;
    }

    int path_idx = str.find('/');
    next_break = str.substr(path_idx).find(' ');
    this->http_path = str.substr(path_idx).substr(0,next_break);

    int protocol_idx = path_idx + next_break +1;
    next_break = str.substr(protocol_idx).find('\r');
    this->http_protocol = str.substr(protocol_idx).substr(0,next_break);
        
    int host_idx = str.find("Host");
    next_break = str.substr(str.find("Host")).find('\r');
    this->http_host = str.substr(host_idx+std::string("Host: ").length(),next_break-std::string("Host: ").length()); 
}
std::string HttpRequest::getHttpHost(){
    return this->http_host;
}

HttpVerbs HttpRequest::getHttpVerb(){
    return this->http_verb;
}
std::string HttpRequest::getHttpVerbString(){
    switch (this->http_verb){
        case HttpVerbs::GET:
            return "GET";
        case HttpVerbs::DELETE:
            return "DELETE";
        case HttpVerbs::POST:
            return "POST";
        case HttpVerbs::PUT:
            return "PUT";
        case HttpVerbs::PATCH:
            return "PATCH";
        default:
            return "";
    }
}   


std::string HttpRequest::getHttpPath(){
    return this->http_path;
}
std::string HttpRequest::getHttpProtocol(){
    return this->http_protocol;
}
std::string HttpRequest::toString(){
    std::string str = this->getHttpVerbString() + " " + http_path + " " + http_host;
    return str;
}
void HttpRequest::printToTerminal(){
    
    tm* localTime = localtime(&(this->req_time)); // Convert to local time
    char t_buffer[80];
    strftime(t_buffer, sizeof(t_buffer), "%H:%M:%S", localTime); //Format time
    printf(COLOR("%s ==> [%s] %s\n", KYEL).c_str(),t_buffer,this->getHttpVerbString().c_str(), this->getHttpPath().c_str());
}     

//HTTP RESPONSE

std::string HttpResponse::format_http_response(){
    std::string resp = this->http_protocol + " " + this->status_code + " " + this->status_message + "\r\n"
            + this->format_headers()
            + "Content-Length:"+std::to_string(this->body.length()) +"\r\n"
            + "\r\n"
            + this->body;
    return resp;
}
std::string HttpResponse::format_headers(){
    std::string str = "";
    for(auto it = headers.cbegin(); it != headers.cend(); it++){
        str+=it->first + ":" + it->second + "\r\n";
    }
    return str;
}
HttpResponse::HttpResponse(int clientSocket){
    this->clientSocket = clientSocket;
}
HttpResponse::HttpResponse(std::string http_protocol, int clientSocket, std::string status_code, std::string status_message, std::map<std::string,std::string> headers, std::string body){
        this->http_protocol = http_protocol;
        this->status_code = status_code;
        this->status_message = status_message;
        this->headers = headers;
        this->body = body;
        this->clientSocket=clientSocket;
    }
void HttpResponse::send(){
    std::string formatted_response = this->format_http_response();
    ssize_t bytes_sent = ::send(this->clientSocket, formatted_response.c_str(), formatted_response.length(), 0);
                
    close(this->clientSocket);
}

void HttpResponse::sendFile(std::string filePath){
    std::ifstream MyReadFile(filePath);
    std::string file_line;
    std::string file_text = "";
    while (getline (MyReadFile, file_line)) {
        file_text += file_line;
    }
    this->body = file_text;
    this->send();
}

void HttpResponse::setHeaders(std::map<std::string,std::string> new_headers){
    this->headers.insert(new_headers.begin(), new_headers.end());
}

void HttpResponse::setCode(std::string code, std::string message){
    this->status_code = code;
    this->status_message = message;
}

void HttpResponse::setProtocol(std::string protocol){
    this->http_protocol = protocol;
}

//ROUTER

void Router::addRoute(std::deque<std::string> route, HttpVerbs method, std::function<void (HttpRequest, HttpResponse)> callback){
    //TODO: Implement recursive cases
    
    if (route.empty()){
        route_methods[method] = callback;
        return;
    }
    std::string route_root = route[0];
    this->path = route_root;
    route.pop_front();
    if(!routes.count(route_root)){
        routes[route_root] = new Router();
    }
    if(!routes[route_root]){
        std::cout<< COLOR("BAD REF!!!!", KRED)<<std::endl;
    }
    routes[route_root]->addRoute(route, method, callback); //Calls the private method
}

std::deque<std::string> Router::splitPathStringToRouteVector(std::string path){
    std::deque<std::string> route;
    if(!path.empty()){
        int start = 0;
        do{
            int idx = path.find('/',start); 
            if(idx==std::string::npos){
                break;
            }

            int length = idx-start;
            route.push_back(path.substr(start,length));
            start += length + std::string("/").size();

        } while(true);
        int length = path.length() - start;
        route.push_back(path.substr(start,length));
    }

    return route;
}

void Router::addRoute(std::string path, HttpVerbs method, std::function<void (HttpRequest, HttpResponse)> callback){
    //Implement base case
    if(path[0] != '/'){
        throw std::runtime_error("All routes must begin with /");
    }
    std::deque<std::string> route = this->splitPathStringToRouteVector(path);
    if(*route.cbegin()==""){
        route.pop_front();
    }
this->addRoute(route, method, callback); //Calls the private method
}

void Router::routeRequest(HttpRequest req, HttpResponse res){
    std::cout << COLOR("RECEIVED REQUEST ON PATH " + req.getHttpPath(), KGRN) << std::endl;
    std::deque<std::string> route = this->splitPathStringToRouteVector(req.getHttpPath());
    Router *curr_router = this;
    route.pop_front();
    while(!route.empty()){
        if(curr_router->routes.count(route[0]) == 0){
            std::cout << COLOR("Invalid path at " +  route[0],KRED) << std::endl;//Replace with 405 error
            res.setHeaders({ {"content-type","text/html"}});
            res.setCode("405","METHOD NOT ALLOWED");
            res.setProtocol(req.getHttpProtocol());
            res.send();
            return;
        }
        curr_router = curr_router->routes[route[0]];
        route.pop_front();
    }
    if (curr_router->route_methods.count(req.getHttpVerb())){
        if(curr_router->route_methods[HttpVerbs::GET]){
            curr_router->route_methods[req.getHttpVerb()](req,res);
        } else {
            std::cout << COLOR("This function call is unsafe", KRED) <<  std::endl;
        }
    } else {
        res.setHeaders({ {"content-type","text/html"}});
        res.setCode("405","METHOD NOT ALLOWED");
        res.setProtocol(req.getHttpProtocol());
        res.send();
    }
    
}

// SERVER

Server::Server(){
    this->serverSocket = socket(AF_INET, SOCK_STREAM| SOCK_NONBLOCK, 0); //serverSocket is a file descriptor
    setsockopt(serverSocket,SOL_SOCKET, SO_REUSEADDR, (char *) &this->reuse_addr_val, sizeof(int)); //Sets the socket option of reusing address to 1
    this->serverAddress.sin_family = AF_INET; //We are using IPv4 (XX.XX.XX.XX)
    this->serverAddress.sin_port = htons(PORT); //Converts host short 8080 to network order
    this->serverAddress.sin_addr.s_addr = inet_addr(IP); //Any IP Address
    this->router = Router();
}

void Server::signal_handler(int signum) {
    std::cout << COLOR("\nInterruped receieved.", KRED) << std::endl;
    if (signum == SIGINT) { // Handle Ctrl+C
        exit_flag = 1;
    }
}

void Server::listen(){
    if(started){
        throw std::runtime_error("Cannot start server that's alreadys started.");
    }
    bind(this->serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); //Connects the file descriptor to port 8080, for all IP addresses
    ::listen(this->serverSocket, 5);
    signal(SIGINT, Server::signal_handler);  
    
    std::string message = std::string("Listening for connections on ") + IP + ":" + std::to_string(PORT) + "...";
    std::cout << COLOR(message,KRED) << std::endl;
    this->clientSocket = -1;

        
    while (!exit_flag){ //Keep looping till SIGINT

        socklen_t addrlen = sizeof(serverAddress);
        //We use accept4 rather than accept because we can make it non-blocking.
        clientSocket = accept4(serverSocket, (struct sockaddr*)&serverAddress, &addrlen, SOCK_NONBLOCK); //Accept it if we get a connection and get a file descriptor for the requesting socket
        
        if (clientSocket < 0) { //No valid file descriptor created
            if (errno == EAGAIN || errno == EWOULDBLOCK) { //In the case of there being no connection available
                usleep(100000); // 100ms sleep to avoid busy loop
                continue;
            } else { //Something went wrong
                perror("accept4 failed");
                break;
            }
        }

        char buffer[1024] = {0}; //Init a buffer for accepting data from the requestings socket 
        ssize_t bytes = recv(clientSocket, buffer, sizeof(buffer), 0); //Copy data into buffer 
        
        if (bytes < 1){
            std::cout << COLOR("Client disconnected.",KGRY) << std::endl;
            close(clientSocket);
            continue;
        }
        
        std::string str(buffer);
        HttpRequest req(str); //Stack allocation for now, until we need something more complicated.
        HttpResponse res(clientSocket);
        this->router.routeRequest(req,res);

    
    }

    //Don't want to leak file descriptors
    std::cout << COLOR("Gracefully Shutting Down...\n",KGRN);
    if(clientSocket != -1){
        close(clientSocket);
    }
    close(serverSocket); 
}

//METHODS FOR ADDING ENDPOINTS
void Server::GET(std::string path, std::function<void(HttpRequest,HttpResponse)> callback){
    this->router.addRoute(path,HttpVerbs::GET,callback);
}
void Server::POST(std::string path, std::function<void(HttpRequest,HttpResponse)> callback){
    this->router.addRoute(path,HttpVerbs::POST,callback);
}
void Server::PUT(std::string path, std::function<void(HttpRequest,HttpResponse)> callback){
    this->router.addRoute(path,HttpVerbs::PUT,callback);
}
void Server::PATCH(std::string path, std::function<void(HttpRequest,HttpResponse)> callback){
    this->router.addRoute(path,HttpVerbs::PATCH,callback);
}
void Server::DELETE(std::string path, std::function<void(HttpRequest,HttpResponse)> callback){
    this->router.addRoute(path,HttpVerbs::DELETE,callback);
}

