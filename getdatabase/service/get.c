#define WIN32_LEAN_AND_MEAN

#include "../url/url.h"
#include "../native/wrap.h"
#include "../network/http.h"
#include <math.h>
#include <windows.h>

struct THREAD_ARGS
{
    char *_link;
    int index;
};

void DownloadLink(struct THREAD_ARGS *args)
{
    char link[1024];
    strcpy(link, args->_link);
    int NameIndex = args->index;

    char Method[1024];
    char HttpRaw[4094];

    char Header[4096];

    char lpFileName[1024];

    int length = strlen("http://www.ftc.go.kr");

    SOCKET hSocket = socket_(PF_INET, SOCK_STREAM, 0);
    int r = AttemptTcpRequest(hSocket, "www.ftc.go.kr", "80");

    wsprintfA(Method, "GET %s HTTP/1.1", link + length);

    if (r == TRUE)
    {
        //connect success

        CreateHttpRaw(HttpRaw, sizeof(HttpRaw), Method,
                      "Host", "www.ftc.go.kr",
                      "Connection", "Keep-Alive",
                      "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/80.0.3987.87 Safari/537.36",
                      "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9",
                      "Referer", "http://www.ftc.go.kr/www/dataOpen.do?key=259",
                      "Content", "");

        //printf("%s\n", HttpRaw);

        send_(hSocket, HttpRaw, strlen(HttpRaw), 0);

        wsprintf(lpFileName, "database/%d.csv", NameIndex);

        FILE *data = fopen(lpFileName, "w");
        int CrlfIndex = 0;
        for (;;)
        {
            //fill zero
            ZeroMemory(Header, sizeof(Header));

            //RecvLine function is return index of line feed ( CR 'LF' )
            CrlfIndex = RecvLine(hSocket, Header, sizeof(Header));

            if (CrlfIndex < 2)
            {
                RecvLine(hSocket, Header, sizeof(Header));

                DWORD dwContentLength = strtol(Header, NULL, 16);
                if (dwContentLength == 0)
                    break;

                BYTE *lpContent = GlobalAlloc(GPTR, dwContentLength);
                Recv(hSocket, lpContent, dwContentLength);

                //printf("%s\n", lpContent);

                fwrite(lpContent, dwContentLength, 1, data);

                GlobalFree(lpContent);
            }
        }

        RecvLine(hSocket, Header, sizeof(Header));

        fclose(data);

        shutdown_(hSocket, SD_BOTH);
    }
    
    closesocket_(hSocket);
}

//main thread
int main()
{
    printf("[+] Startup database download..\n");

    /*
    This function is using for initialize windows socket api function
    This function is call LoadLibraryA function 
    So, is not for linux OS, Only for windows
    */
    HMODULE SocketModule = InitializeTcpLibrary();

    /*
    This WSAAPI function is using for initialize windows socket api module
    
    first argument is version number
    second argument is address of WSADATA variable
    */
    WSADATA wsa;
    WSAStartup_(MAKEWORD(2, 2), &wsa);

    /*
    first argument is port number, (using for bind function)
    second argument is backlog, (using for listen function)
    third argument is blocking mode

    return value is opened socket 
    */

    /*
    client
    */
    int LineCount = 0;
    int NameIndex = 0;
    char link[4096];

    FILE *fp = fopen("links.txt", "r");
    if (fp == NULL)
    {
        printf("open faild..\n");
    }
    
    for (; fgets(link, sizeof(link), fp) != NULL; LineCount += 1);
    fclose(fp);

    fp = fopen("links.txt", "r");
    HANDLE *hThread = (HANDLE*)GlobalAlloc(GPTR, LineCount * sizeof(HANDLE));

    for (; fgets(link, sizeof(link), fp) != NULL;)
    {
        struct THREAD_ARGS arg;
        arg._link = link;
        arg.index = NameIndex;

        strtok(link, "\r\n");
        hThread[NameIndex] = CreateThread_((FUNCTION_ADDRESS)DownloadLink, &arg);
        
        NameIndex += 1;

        Sleep(10);
    }

    printf("[+] wait..\n");
    for(int i = 0; i != NameIndex; i += 1)
        WaitForSingleObject(hThread[i], INFINITE);

    printf("[+] done!\n");

    //if called closesocket function, server thread is return
    //free windows socket api
    WSACleanup_();
    //unmapping socket module
    FreeLibrary(SocketModule);

    //end of program

    system("append.exe");

    return 0;
}
