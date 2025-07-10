#include "../include/server.h"
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

int main(){
    pid_t pid = fork();
    int status;
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }else if (pid > 0) {
        std::cout << "printed from parent process " << getpid() << std::endl;
        CURL *curl = curl_easy_init();
        if(curl) {

            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/");

            /* size of the POST data */
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
            std::string response_string;
            std::string header_string;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
            /* pass in a pointer to the data - libcurl will not copy */

            curl_easy_perform(curl);
            std::cout << response_string << std::endl;
        }
        waitpid(pid, &status, 0);
        if (WIFSIGNALED(status)){
            printf("Error\n");
        }
        else if (WEXITSTATUS(status)){
            printf("Exited Normally\n");
        }
    }
    else {
        std::cout << "printed from child process " << getpid()<< std::endl;
        Server app = Server();
        app.GET("/", [](HttpRequest req, HttpResponse res){
            res.setProtocol(req.getHttpProtocol());
            res.setHeaders({ {"content-type","text/html"}});
            res.setCode("200","OK");
            res.sendFile("../success.txt");
        });
        app.listen();
    }
    return 0;
}