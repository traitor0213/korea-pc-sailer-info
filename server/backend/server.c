#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "../url/url.h"
#include "../native/wrap.h"
#include "../network/http.h"

#define CRLF "\r\n"
const int CRLF_LENGTH = 4;
const int HEADER_SIZE = 1024;
const int HTML_SIZE = 4096;

int ErrorResponse(int hSocket)
{
    const static char* PAGE =
        "<!DOCTYPE html>" CRLF
        "<html lang=\"en\">" CRLF
        "<head>" CRLF
        "<meta charset=\"UTF-8\">" CRLF
        "<title>database list </title>" CRLF
        "</head>" CRLF
        "<body>" CRLF
        "<h1>ERROR Page</h1>" CRLF
        "</body>" CRLF
        "</html>" CRLF;
    char* ResponseRaw = (char*)GlobalAlloc(GPTR, strlen(PAGE) + HEADER_SIZE);
    if (ResponseRaw == NULL) return FALSE;

    int length = CreateHttpRaw(ResponseRaw, GlobalSize(ResponseRaw),
        "HTTP/1.1 200 OK",
        "Host", "database-server",
        "Content-Type", "text/html; charset=utf-8",
        "Content-Length", strlen(PAGE),
        "Content", PAGE);

    send_(hSocket, ResponseRaw, length, 0);
    GlobalFree(ResponseRaw);

    return FALSE;
}

int TextRangeResponse(int hSocket, const char* Raw, const char* path, const char* arg)
{
    /*
    Get requested file size
    */

    const char* StartLine = strstr(arg, "Start=");
    if (StartLine == NULL)
        return TRUE;
    StartLine += 6;

    DWORD dwStartLine = atoi(StartLine);
    DWORD dwEndLine = dwStartLine + 20;

    if (dwStartLine <= 0)
        return TRUE;


    DWORD FileSize = 0;
    DWORD ContentSize = 0;

    //read file

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }

    FileSize = GetFileSize(hFile, NULL);

    char* FileData = (char*)GlobalAlloc(GPTR, FileSize * 2);
    if (FileData == NULL)
        return TRUE;

    if (ReadFile(hFile, FileData, FileSize, &ContentSize, NULL) == FALSE)
    {
        CloseHandle(hFile);
        return TRUE;
    }
    CloseHandle(hFile);

    //content is ready

    char* EditedFileData = FileData;

    DWORD TotalCrlfCount = 0;
    DWORD CrlfCount = 0;
    DWORD StringLength = 0;

    for (int x = 0; x <= ContentSize; x += 1)
    {
        if (EditedFileData[x] == '\n')
        {
            TotalCrlfCount += 1;
        }
    }

    if (TotalCrlfCount > dwStartLine)
    {
        for (;;)
        {
            if (*EditedFileData == '\n')
                CrlfCount += 1;

            if (CrlfCount == dwStartLine)
                break;

            EditedFileData += 1;
        }

        char* temp = EditedFileData;

        for (;;)
        {
            if (ContentSize <= StringLength) break;

            if (*EditedFileData == '\n')
                CrlfCount += 1;

            if (CrlfCount == dwEndLine)
                break;

            StringLength += 1;
            EditedFileData += 1;
        }

        EditedFileData = temp;

        char* Page = (char*)GlobalAlloc(GPTR, FileSize * 2 + HTML_SIZE);
        if (Page == NULL)
            return TRUE;
        ZeroMemory(Page, GlobalSize(Page));

        int n = 0;

        if (dwStartLine > 20)
        {
            n = -20;
        }
        for (int i = 0; i != 10; i += 1)
        {
            sprintf(Page, "%s\r\n<a href=\"/%s?Start=%d\">%d</a> &nbsp;", Page, path, dwStartLine + n, dwStartLine + n);
            n += 20;
        }
        strcat(Page, "\r\n\r\n");
        memcpy(Page + strlen(Page), EditedFileData, StringLength);

        //printf("%s", Page);

        char* ResponseRaw = (char*)GlobalAlloc(GPTR, GlobalSize(Page));

        const char* type = "text/html; charset=euc-kr";

        int length = CreateHttpRaw(ResponseRaw, GlobalSize(ResponseRaw),
            "HTTP/1.1 200 OK",
            "Host", "database-server",
            "Content-Type", type,
            "Content-Length", strlen(Page),
            "Content", Page);

        send_(hSocket, ResponseRaw, length, 0);

        GlobalFree(ResponseRaw);
        GlobalFree(Page);
    }
    else
    {
        ErrorResponse(hSocket);
    }

    GlobalFree(FileData);

    return FALSE;
}

int FileResponse(int hSocket, const char* path)
{
    /*
    Get requested file size
    */

    DWORD FileSize = 0;
    DWORD ContentSize = 0;

    //read file

    HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return TRUE;

    FileSize = GetFileSize(hFile, NULL);

    char* FileData = (char*)GlobalAlloc(GPTR, FileSize);
    if (FileData == NULL)
        return TRUE;

    ReadFile(hFile, FileData, FileSize, &ContentSize, NULL);
    CloseHandle(hFile);

    //content is ready

    char* ResponseRaw = (char*)GlobalAlloc(GPTR, ContentSize + HEADER_SIZE);

    const char* type = "text/html; charset=euc-kr";

    int length = CreateHttpRaw(ResponseRaw, GlobalSize(ResponseRaw),
        "HTTP/1.1 200 OK",
        "Host", "database-server",
        "Content-Type", type,
        "Content-Length", ContentSize,
        "Content", FileData);

    send_(hSocket, ResponseRaw, length, 0);

    GlobalFree(FileData);

    return FALSE;
}

void server(int* fd)
{
    SOCKADDR_IN info = {
        0,
    };

    const char* Crlf = CRLF CRLF;
    const int CrlfLength = GetStringLength(Crlf);

    const char* Space = " ";
    const char SpaceWord = ' ';
    const int SpaceLength = GetStringLength(Space);

    const char* Parameter = "?";
    const char ParameterWord = '?';
    const int ParameterLength = GetStringLength(Parameter);

    const char* And = "*&";
    const char AndWord = '&';
    const int AndLength = GetStringLength(And);

    const char* FullPathParameter = "path=";
    const int FullPathParameterLength = GetStringLength(FullPathParameter);

    const char* LineParamter = "line=";
    const int LineParamterLength = GetStringLength(LineParamter);

    //http header
    char header[4096];

    //request buffer
    char EncodeString[1024];
    char DecodeString[1024];

    char RequestPath[1024];
    char RequestParameters[1024];

    ZeroMemory(RequestParameters, sizeof(RequestParameters));

    //accept loop
    for (;;)
    {
        int ResponseSocket = AcceptTcpRequest(&info, *fd);
        if (ResponseSocket < 0)
        {
            //case of socket error

            if (WSAGetLastError_() != WSAEWOULDBLOCK)
            {
                //thread exit
                break;
            }
        }
        else
        {


            /*
            Recv first line of request header

            (example request header)
            GET /windows/system32/cmd.exe?download HTTP/1.1\r\n
            User-Agent: ...
            */

            ZeroMemory(RequestPath, sizeof(RequestPath));

            char Raw[sizeof(RequestPath)];

            //recv
            ZeroMemory(header, sizeof(header));
            RecvLine(ResponseSocket, header, sizeof(header));

            /*
            find ' '(Space)

            before              = " /windows/system32/cmd.exe?download HTTP/1.1\r\n"
            after(EncodeString) = "/windows/system32/cmd.exe?download HTTP/1.1\r\n"
            */

            int r = SeparateString(EncodeString, sizeof(EncodeString), header, " ");
            if (r == -1)
            {
                continue;
            }

            /*
            find ' '(Space)

            before              = "/windows/system32/cmd.exe?download HTTP/1.1\r\n"
            after(EncodeString) = "/windows/system32/cmd.exe?download"
            */

            int EncodeStringLength = GetStringLength(EncodeString);

            for (int x = 0; x <= EncodeStringLength && x <= sizeof(EncodeString); x += 1)
            {
                if (EncodeString[x] == 0)
                    break;

                if (EncodeString[x] == SpaceWord)
                {
                    EncodeString[x] = 0;
                    break;
                }
            }

            /*
            replace url encode to url decode

            before                  = "/users/hello%20world?dir"
            after(DecodeString)     = "/users/hello world?dir"
            */

            DecodeUrl(DecodeString, sizeof(DecodeString), EncodeString);

            /*
            find request path

            before = "/windows/system32/cmd.exe?download"
            after(RequestPath) = "./windows/system32/cmd.exe"
            */

            int DecodeStringLength = GetStringLength(DecodeString);

            RequestPath[0] = '.';
            for (int x = 0; x <= DecodeStringLength + 1 && x <= sizeof(DecodeString); x += 1)
            {
                if (DecodeString[x] == 0)
                    break;
                if (DecodeString[x] == ParameterWord)
                {
                    RequestPath[x + 1] = 0;
                    break;
                }

                RequestPath[x + 1] = DecodeString[x];
            }

            /*
            find request path

            before = "/windows/system32/cmd.exe?download"
            after(RequestParameters) = "?download"
            */

            int i = 0;
            for (int x = 0; x <= DecodeStringLength && x <= sizeof(RequestParameters); x += 1)
            {
                if (DecodeString[x] == 0)
                    break;

                if (DecodeString[x - 1] == ParameterWord)
                {
                    for (int y = 0;; y += 1)
                    {
                        RequestParameters[i] = DecodeString[x + y];
                        if (DecodeString[x + y] == 0)
                            break;
                        i += 1;
                    }
                }
            }

            /*
            find 'path' command

            before = "path=C:/windows/system32/cmd.exe&value=1234..."
            after  = "C:/windows/system32/cmd.exe"
            */

            int RequestParametersLength = GetStringLength(RequestParameters);
            const char* FullPath = Kmp(RequestParameters, RequestParametersLength, FullPathParameter, FullPathParameterLength);

            int AndLocation = GetStringLength(FullPath);
            if (FullPath != NULL)
            {
                FullPath += FullPathParameterLength;

                for (int x = 0;; x += 1)
                {
                    if (FullPath[x] == 0)
                        break;

                    if (FullPath[x] == AndWord)
                    {
                        if (x > 0)
                        {
                            AndLocation = x;
                        }

                        break;
                    }
                }

                memcpy(RequestPath, FullPath, AndLocation);
            }

            //read other http request header
            for (;;)
            {
                //fill zero
                ZeroMemory(header, sizeof(header));

                //RecvLine function is return index of line feed ( CR 'LF' )
                int CrlfIndex = RecvLine(ResponseSocket, header, sizeof(header));

                if (CrlfIndex < 2)
                {
                    /*
                    CRLF or LF is first word
                    situation of index is 0, 1 or error
                    */

                    break;
                }
            }

        RESPONSE:;

            char* TargetParam = NULL;

            printf("Request File = '%s'\n", RequestPath);
            printf("Request Params = '%s'\n", RequestParameters);

            if (strcmp(RequestPath, "./") == 0)
            {
                strcpy(RequestPath, "./index.html");
                memset(RequestParameters, 0, sizeof(RequestParameters));
            }

            if (strstr(RequestParameters, "comment-") != NULL)
            {
                FILE* CommentDataBase = fopen(RequestPath, "a");
                fprintf(CommentDataBase, "<a>%s</a>" CRLF, RequestParameters);
                fclose(CommentDataBase);

                ErrorResponse(ResponseSocket);
            }
            else if ((TargetParam = strstr(RequestParameters, "commentview-")) != NULL)
            {
                char data[1024];
                char TargetData[1024];
                int Finded = 0;
                int TargetIndex = atoi(TargetParam + 12);

                FILE* CommentDataBase = fopen(RequestPath, "r");
                for (;;)
                {
                    if (fgets(data, sizeof(data) - 1, CommentDataBase) != NULL)
                    {
                        if (atoi(data + 11) == TargetIndex)
                        {
                            strtok(data, CRLF);
                            memcpy(TargetData, data, sizeof(TargetData));
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                fclose(CommentDataBase);

                const char* PAGE =
                    "<!DOCTYPE html>" CRLF
                    "<html lang=\"en\">" CRLF
                    "<head>" CRLF
                    "<meta charset=\"UTF-8\">" CRLF
                    "<title>database list </title>" CRLF
                    "</head>" CRLF
                    "<body>" CRLF
                    "%s" CRLF
                    "</body>" CRLF
                    "</html>" CRLF;

                sprintf(data, PAGE, TargetData);

                char* ResponseRaw = (char*)GlobalAlloc(GPTR, strlen(data) + HEADER_SIZE);
                if (ResponseRaw == NULL)
                {
                    ErrorResponse(ResponseSocket);
                    continue;
                }

                int length = CreateHttpRaw(ResponseRaw, GlobalSize(ResponseRaw),
                    "HTTP/1.1 200 OK",
                    "Host", "database-server",
                    "Content-Type", "text/html; charset=utf-8",
                    "Content-Length", strlen(data),
                    "Content", data);

                send_(ResponseSocket, ResponseRaw, length, 0);
                GlobalFree(ResponseRaw);
            }
            else
            {
                if (TextRangeResponse(ResponseSocket, DecodeString, RequestPath, RequestParameters) == TRUE)
                {
                    if (FileResponse(ResponseSocket, RequestPath) == TRUE)
                    {
                        ErrorResponse(ResponseSocket);
                    }
                }
            }

            shutdown_(ResponseSocket, SD_BOTH);
            closesocket_(ResponseSocket);
        }
    }
}

//main thread
int main()
{
    /*
    This function is using for initialize windows socket api function
    This function is call LoadLibraryA function
    So, is not for linux OS, Only for windows
    */
    printf("[+] intialize tcp/ip library..\n");
    HMODULE SocketModule = (HMODULE)InitializeTcpLibrary();

    /*
    This WSAAPI function is using for initialize windows socket api module

    first argument is version number
    second argument is address of WSADATA variable
    */
    printf("[+] startup windows socket api\n");
    WSADATA wsa;
    WSAStartup_(MAKEWORD(2, 2), &wsa);

    /*
    first argument is port number, (using for bind function)
    second argument is backlog, (using for listen function)
    third argument is blocking mode

    return value is opened socket
    */

    const int Port = 80;
    const int Backlog = 1024;
    printf("[+] open port=%d; backlog=%d\n", Port, Backlog);
    int ServerSocket = OpenTcpPort(Port, Backlog, TRUE);

    /*
    error check
    if socket value is small than zero, error case
    */

    if (ServerSocket < 0)
    {
        /*
        bind error || listen error

        defined "../network/tcp.h"

        #define ERROR_FROM_SOCKET 0x1
        #define ERROR_FROM_BIND 0x2
        #define ERROR_FROM_LISTEN 0x3
        #define ERROR_FROM_ACCEPT 0x4
        #define ERROR_FROM_CONNECT 0x5
        #define ERROR_FROM_IO 0x6
        */

        if (ServerSocket == ERROR_FROM_BIND || ServerSocket == ERROR_FROM_LISTEN)
        {
            printf("error from port open\n");
            closesocket_(ServerSocket);
        }

        WSACleanup_();
        return 0;

        //end of program
    }

    /*
    create server thread
    'hServer' variable is server thread handle
    */

    printf("[+] create server thread..\n");
    HANDLE hServer = CreateThread_((FUNCTION_ADDRESS)server, (LPVOID)&ServerSocket);

    //user shell
    char input[126];
    for (;;)
    {
        //get stdin stream
        fgets(input, sizeof(input) - 1, stdin);

        //separate CRLF
        strtok(input, CRLF);

        //check 'exit' command
        if (lstrcmpi(input, "exit") == 0)
        {
            //exit server
            break;
        }
    }

    //if called closesocket_ function, server thread is return
    closesocket_(ServerSocket);
    //free windows socket api
    WSACleanup_();
    //unmapping socket module
    FreeLibrary(SocketModule);

    /*
    wait for server thread return
    WaitForSingleObject function is blocking function

    first argument is server thread handle
    second argument is wait time
    */

    WaitForSingleObject(hServer, INFINITE);

    //end of program

    return 0;
}
