#include <iostream> //Basic print functions
#include <unistd.h> //To close file descriptors
#include <sys/socket.h> //Socket functionality
#include <arpa/inet.h>  //To use the inet_addr function to convert from human readable IP addresses to integer for computer
#include <string> //String Processing
#include <sys/types.h>  //For accept4
#include <errno.h>//For catching accept4 errorrs
#include <csignal>//To handle cleanup during debugging.
#include <format>//Format strings
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


using namespace std;

#define IP "127.0.0.1"
#define PORT 8080

volatile int exit_flag=0;


string COLOR(const string& text, const char* color_code) { //Because I grow weary of black and white.
    return string(color_code) + text + RST;
}

enum HttpVerbs{
    GET,
    POST,
    DELETE,
    PUT,
    PATCH
};

/// @brief An object for managing incoming HTTP requests
class HttpRequest{
    private:
        string http_host;
        HttpVerbs http_verb;
        string http_path;
        string http_protocol;
        time_t req_time;
    public:
        /// @brief Constructor that takes in a string and produces an HttpRequest Object
        /// @param str //The result of running str() on a char buffer of bytes received via websocket
        HttpRequest();

        HttpRequest(string str) {  
            this->req_time = time(0);
            int next_break = 0;

            next_break = str.find(' ');
            string http_verb_str = str.substr(0,next_break);
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
            this->http_host = str.substr(host_idx+string("Host: ").length(),next_break-string("Host: ").length()); 
        }

        string getHttpHost(){
            return this->http_host;
        }

        string getHttpVerbString(){
            switch (http_verb){
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
            }
        }

        HttpVerbs getHttpVerb(){
            return this->http_verb;
        }

        string getHttpPath(){
            return this->http_path;
        }

        string getHttpProtocol(){
            return this->http_protocol;
        }

        string toString(){
            string str = this->getHttpVerbString() + " " + http_path + " " + http_host;
            return str;
        }

        void printToTerminal(){
            
            tm* localTime = localtime(&(this->req_time)); // Convert to local time
            char t_buffer[80];
            strftime(t_buffer, sizeof(t_buffer), "%H:%M:%S", localTime); //Format time
            printf(COLOR("%s ==> [%s] %s\n", KYEL).c_str(),t_buffer,this->getHttpVerbString().c_str(), this->getHttpPath().c_str());
        }
};

/// @brief An object for creating and sending HTTP responses
class HttpResponse{
    private:
        string http_protocol = "HTTP";
        string status_code;
        string status_message;
        std::map<string,string> headers;
        string body;
        int clientSocket;
        
        string format_http_response(){
            string resp = this->http_protocol + " " + this->status_code + " " + this->status_message + "\r\n"
                    + this->format_headers()
                    + "Content-Length:"+to_string(this->body.length()) +"\r\n"
                    + "\r\n"
                    + this->body;
            return resp;
        }

        string format_headers(){
            string str = "";
            for(auto it = headers.cbegin(); it != headers.cend(); it++){
                str+=it->first + ":" + it->second + "\r\n";
            }
            return str;
        }
    public:
        HttpResponse();

        HttpResponse(int clientSocket){
            this->clientSocket = clientSocket;
        }
        
        HttpResponse(string http_protocol, int clientSocket, string status_code, string status_message, std::map<string,string> headers, string body){
            this->http_protocol = http_protocol;
            this->status_code = status_code;
            this->status_message = status_message;
            this->headers = headers;
            this->body = body;
            this->clientSocket=clientSocket;
        }

        void send(){
            string formatted_response = this->format_http_response();
            ssize_t bytes_sent = ::send(this->clientSocket, formatted_response.c_str(), formatted_response.length(), 0);
                        
            close(this->clientSocket);
        }

        void sendFile(string filePath){
            ifstream MyReadFile(filePath);
            string file_line;
            string file_text = "";
            while (getline (MyReadFile, file_line)) {
                file_text += file_line;
            }
            this->body = file_text;
            this->send();
        }

        void setHeaders(std::map<string,string> new_headers){
            this->headers.insert(new_headers.begin(), new_headers.end());
        }

        void setCode(string code, string message){
            this->status_code = code;
            this->status_message = message;
        }

        void setProtocol(string protocol){
            this->http_protocol = protocol;
        }
};

class HttpRoute{
    private:
        string http_verb;
        string http_route;
};

/// @brief A class for routing endpoint paths
class Router{
    private:
        bool is_dynamic;
        bool is_end_of_path;

        void addRoute(std::deque<string> route, HttpVerbs method, function<void (HttpRequest, HttpResponse)> callback){
            //TODO: Implement recursive cases
            if (route.empty()){
                route_methods[method] = callback;
            }
        }

    protected:
        map<HttpVerbs,function<void (HttpRequest, HttpResponse)>> route_methods;
        map<string,Router> routes;
    public:

        std::deque<string> splitPathStringToRouteVector(string path){
            std::deque<string> route;
            if(!path.empty()){
                int start = 0;
                do{
                    int idx = path.find('/',start); 
                    if(idx==string::npos){
                        break;
                    }

                    int length = idx-start;
                    route.push_back(path.substr(start,length));
                    start += length + string("/").size();

                } while(true);
            }
            return route;
        }

        void addRoute(string path, HttpVerbs method, function<void (HttpRequest, HttpResponse)> callback){
            //Implement base case
            std::deque<string> route = this->splitPathStringToRouteVector(path);

            if(route.empty()){
                this->addRoute(route, method, callback);
                return;
            }
            
            if(!routes.count(route[0])){
                routes[route[0]] = Router();
            }
            route.pop_front();

            routes[route[0]].addRoute(route, method, callback);
        }
        
        void routeRequest(HttpRequest req, HttpResponse res){
            cout << COLOR("RECEIVED REQUEST ON PATH " + req.getHttpPath(), KGRN) << endl;
            std::deque<string> route = this->splitPathStringToRouteVector(req.getHttpPath());
            Router curr_router = *this;
            while(!route.empty()){
                if(curr_router.routes.count(route[0]) == 0){
                    throw runtime_error("Invalid path.");//Replace with 405 error
                }
                curr_router = curr_router.routes[route[0]];
                route.pop_front();
            }
            curr_router.route_methods[req.getHttpVerb()](req,res);
        }
    };

class Server{
    private:
        Router router;
        int serverSocket;
        int reuse_addr_val = 1;
        sockaddr_in serverAddress;
        int clientSocket = -1;
        bool started = false;
    public:

        Server(){
            this->serverSocket = socket(AF_INET, SOCK_STREAM| SOCK_NONBLOCK, 0); //serverSocket is a file descriptor
            setsockopt(serverSocket,SOL_SOCKET, SO_REUSEADDR, (char *) &this->reuse_addr_val, sizeof(int)); //Sets the socket option of reusing address to 1
            this->serverAddress.sin_family = AF_INET; //We are using IPv4 (XX.XX.XX.XX)
            this->serverAddress.sin_port = htons(PORT); //Converts host short 8080 to network order
            this->serverAddress.sin_addr.s_addr = inet_addr(IP); //Any IP Address
            this->router = Router();
        }

        static void signal_handler(int signum) {
            cout << COLOR("\nInterruped receieved.", KRED) << endl;
            if (signum == SIGINT) { // Handle Ctrl+C
                exit_flag = 1;
            }
        }

        void listen(){
            if(started){
                throw std::runtime_error("Cannot start server that's alreadys started.");
            }
            bind(this->serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); //Connects the file descriptor to port 8080, for all IP addresses
            ::listen(this->serverSocket, 5);
            signal(SIGINT, Server::signal_handler);  
            
            string message = string("Listening for connections on ") + IP + ":" + to_string(PORT) + "...";
            cout << COLOR(message,KRED) << endl;
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
                    cout << COLOR("Client disconnected.",KGRY) << endl;
                    close(clientSocket);
                    continue;
                }
                
                string str(buffer);
                HttpRequest req(str); //Stack allocation for now, until we need something more complicated.
                HttpResponse res(clientSocket);
                this->router.routeRequest(req,res);

           
            }

            //Don't want to leak file descriptors
            cout << COLOR("Gracefully Shutting Down...\n",KGRN);
            if(clientSocket != -1){
                close(clientSocket);
            }
            close(serverSocket); 
        }

        //METHODS FOR ADDING ENDPOINTS
        void GET(string path, std::function<void(HttpRequest,HttpResponse)> callback){
            this->router.addRoute(path,HttpVerbs::GET,callback);
        }
        void POST(string path, std::function<void(HttpRequest,HttpResponse)> callback){
            this->router.addRoute(path,HttpVerbs::POST,callback);
        }
        void PUT(string path, std::function<void(HttpRequest,HttpResponse)> callback){
            this->router.addRoute(path,HttpVerbs::PUT,callback);
        }
        void PATCH(string path, std::function<void(HttpRequest,HttpResponse)> callback){
            this->router.addRoute(path,HttpVerbs::PATCH,callback);
        }
        void DELETE(string path, std::function<void(HttpRequest,HttpResponse)> callback){
            this->router.addRoute(path,HttpVerbs::DELETE,callback);
        }
};

int main(void)  {
    
    Server app = Server();
    app.GET("/", [](HttpRequest req, HttpResponse res){
        res.setHeaders({ {"content-type","text/html"}});
        res.setCode("200","OK");
        res.setProtocol(req.getHttpProtocol());
        res.sendFile("index.html");
    });

    app.listen();

    
    return 0;
}