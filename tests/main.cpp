#include "../include/server.h"
#include <unistd.h>
#include <sys/wait.h>
#include <curl\curl.h>

int main(){
    pid_t pid = fork();
    int status;
    if(pid == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }else if (pid > 0) {
        std::cout << "printed from parent process " << getpid() << std::endl;
        std::CURL *curl = curl_easy_init();
        if(curl) {
            const char *data = "submit = 1";

            curl_easy_setopt(curl, CURLOPT_URL, "http://10.5.10.200/website/WebFrontend/backend/posttest.php");

            /* size of the POST data */
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 10L);

            /* pass in a pointer to the data - libcurl will not copy */
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

            curl_easy_perform(curl);
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
            res.sendFile("index.html");
        });
        app.listen();
    }
    return 0;
}