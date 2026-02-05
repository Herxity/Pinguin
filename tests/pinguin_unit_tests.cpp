#include "../include/pinguin.h"
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>

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

int main(){
    std::cout<<KBLU<<"Running Pinguin Unit Tests...\n"<<RST;

    std::cout<<KYEL<<"Testing GET Request Parsing...\n"<<RST;
    std::string get_request = 
    "GET /index.html?user=123 HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: Mozilla/5.0\r\n"
    "Accept: text/html\r\n"
    "\r\n";
    HttpRequest req(get_request);
    //Does it parse the http_path correctly?
    if(req.getHttpPath() == "/"){
        std::cout<<KGRN<<"[PASS]"<<RST<<" Successfully parsed request path\n";
    } else {
        std::cout<<KRED<<"[FAIL]"<<RST<<" Incorrectly parsed request path\n"<<RST;
        std::cout<<"    Parsed -> \"" << req.getHttpPath() << "\" instead\n";
    }
    //Does it parse the file name correctly?
    if(req.getFileName() == "index.html"){
        std::cout<<KGRN<<"[PASS]"<<RST<<" Successfully parsed request file name\n";
    } else {
        std::cout<<KRED<<"[FAIL]"<<RST<<" Incorrectly parsed request file name\n"<<RST;
        std::cout<<"    Parsed -> \"" << req.getFileName() << "\" instead\n";
    }
    //Does it parse the verb correctly?
    if(req.getHttpVerb() == HttpVerbs::GET){
        std::cout<<KGRN<<"[PASS]"<<RST<<" Successfully parsed request verb\n";
    } else {    
        std::cout<<KRED<<"[FAIL]"<<RST<<" Incorrectly parsed request verb\n"<<RST;
        std::cout<<"    Parsed -> \"" << req.getHttpVerbString() << "\" instead\n";
    }
    //Does it parse the query parameters correctly?
    std::map<std::string,std::string> q_params = req.parseQueryParamsFromString("user=123&token=abc");
    if(q_params["user"] == "123" && q_params["token"] == "abc"){
        std::cout<<KGRN<<"[PASS]"<<RST<<" Successfully parsed query parameters\n";
    } else {
        std::cout<<KRED<<"[FAIL]"<<RST<<" Incorrectly parsed query parameters\n"<<RST;
        for(auto it = q_params.cbegin(); it != q_params.cend(); it++){
            std::cout<<"    Parsed -> \"" << it->first << "\" : \"" << it->second << "\"\n";
        }
    }


    HttpResponse res(0);

    Server app;
    app.GET("/", [](HttpRequest req, HttpResponse res){
        res.setProtocol(req.getHttpProtocol());
        res.setHeaders({ {"content-type","text/html"}});
        res.setCode("200","OK");
        res.sendFile("../success.txt");
    });
    app.router.routeRequest(req,res);
    return 0;
}