#ifndef server
#define server

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

#define IP "127.0.0.1"
#define PORT 8080

std::string COLOR(const std::string& text, const char* color_code);

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
        std::string http_host;
        HttpVerbs http_verb = HttpVerbs::GET;
        std::string http_path = "/home/about";
        std::string http_protocol;
        time_t req_time;
    public:
        /// @brief Constructor that takes in a std::string and produces an HttpRequest Object
        /// @param str //The result of running str() on a char buffer of bytes received via websocket
        HttpRequest();

        HttpRequest(std::string str);
        
        std::string getHttpHost();

        std::string getHttpVerbString();

        HttpVerbs getHttpVerb();

        std::string getHttpPath();

        std::string getHttpProtocol();

        std::string toString();

        void printToTerminal();
};

/// @brief An object for creating and sending HTTP responses
class HttpResponse{
    private:
        std::string http_protocol = "HTTP";
        std::string status_code;
        std::string status_message;
        std::map<std::string,std::string> headers;
        std::string body;
        int clientSocket;
        
        std::string format_http_response();

        std::string format_headers();

    public:
        HttpResponse();
        HttpResponse(int clientSocket);
        HttpResponse(std::string http_protocol, int clientSocket, std::string status_code, std::string status_message, std::map<std::string,std::string> headers, std::string body);


        void send();

        void sendFile(std::string filePath);

        void setHeaders(std::map<std::string,std::string> new_headers);

        void setCode(std::string code, std::string message);

        void setProtocol(std::string protocol);
};

class HttpRoute{
    private:
        std::string http_verb;
        std::string http_route;
};

/// @brief A class for routing endpoint paths
class Router{
    private:
        bool is_dynamic;
        bool is_end_of_path;

        void addRoute(std::deque<std::string> route, HttpVerbs method, std::function<void (HttpRequest, HttpResponse)> callback);

    public:
        std::map<HttpVerbs,std::function<void (HttpRequest, HttpResponse)>> route_methods;
        std::map<std::string,Router *> routes;
        std::string path;

        std::deque<std::string> splitPathStringToRouteVector(std::string path);

        void addRoute(std::string path, HttpVerbs method, std::function<void (HttpRequest, HttpResponse)> callback);
        
        void routeRequest(HttpRequest req, HttpResponse res);
    };

class Server{
    private:
        
        int serverSocket;
        int reuse_addr_val = 1;
        sockaddr_in serverAddress;
        int clientSocket = -1;
        bool started = false;
    public:
        Router router;
        Server();
        static void signal_handler(int signum);

        void listen();

        //METHODS FOR ADDING ENDPOINTS
        void GET(std::string path, std::function<void(HttpRequest,HttpResponse)> callback);
        void POST(std::string path, std::function<void(HttpRequest,HttpResponse)> callback);
        void PUT(std::string path, std::function<void(HttpRequest,HttpResponse)> callback);
        void PATCH(std::string path, std::function<void(HttpRequest,HttpResponse)> callback);
        void DELETE(std::string path, std::function<void(HttpRequest,HttpResponse)> callback);
};
#endif