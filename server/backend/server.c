#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include "../url/url.h"
#include "../native/wrap.h"
#include "../network/http.h"

#define CRLF "\r\n"
const int CRLF_LENGTH = 4;

/*
server thread
*/

void GetCsvMember(char *writen, const char *buffer, int colum)
{
    int length = strlen(buffer);

    int y = 0;
    for (int i = 0; i != length; i += 1)
    {
        if (buffer[i] == '\n')
            break;

        if (buffer[i] == ',')
        {
            colum -= 1;
            y = 0;

            if (colum == 0)
                break;
        }
        else
        {
            writen[y] = buffer[i];
            writen[y + 1] = 0;
            y += 1;
        }
    }

    return;
}

void server(int *fd)
{
    SOCKADDR_IN info = {
        0,
    };

    const char *Crlf = CRLF CRLF;
    const int CrlfLength = GetStringLength(Crlf);

    const char *Space = " ";
    const char SpaceWord = ' ';
    const int SpaceLength = GetStringLength(Space);

    const char *Parameter = "?";
    const char ParameterWord = '?';
    const int ParameterLength = GetStringLength(Parameter);

    const char *And = "*&";
    const char AndWord = '&';
    const int AndLength = GetStringLength(And);

    const char *FullPathParameter = "path=";
    const int FullPathParameterLength = GetStringLength(FullPathParameter);

    const char *LineParamter = "line=";
    const int LineParamterLength = GetStringLength(LineParamter);

    //http header
    char header[4096];

    //request buffer
    char EncodeString[1024];
    char DecodeString[1024];

    char RequestPath[1024];
    char RequestParameters[1024];

    char *Response;
    char *FileData;
    char *FileDataOriginal;

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

            Sleep(1);
        }
        else
        {
            FileData = NULL;
            Response = NULL;

            /*
            Recv first line of request header

            (example request header)
            GET /windows/system32/cmd.exe?download HTTP/1.1\r\n
            User-Agent: ...
            */

            ZeroMemory(RequestPath, sizeof(RequestPath));

            //recv
            ZeroMemory(header, sizeof(header));
            RecvLine(ResponseSocket, header, sizeof(header));

            printf("\n\n\n");
            printf("%s\n", header);

            /*
            find ' '(Space)

            before              = " /windows/system32/cmd.exe?download HTTP/1.1\r\n"
            after(EncodeString) = "/windows/system32/cmd.exe?download HTTP/1.1\r\n"
            */

            int r = SeparateString(EncodeString, sizeof(EncodeString), header, " ");
            if (r != -1)
            {
                ZeroMemory(RequestParameters, sizeof(RequestParameters));

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
                const char *FullPath = Kmp(RequestParameters, RequestParametersLength, FullPathParameter, FullPathParameterLength);

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

                    printf("\n");
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
            }

            printf("Request File = '%s'\n", RequestPath);
            printf("Request Params = '%s'\n", RequestParameters);

            char *start = NULL;
            char *end = NULL;

            char *eindex = NULL;
            char *edit = NULL;

            char *Confirmed = NULL;

            char buffer[1024];
            char target[1024];

            if ((Confirmed = strstr(RequestParameters, "Confirm=")) != NULL)
            {
                char DataBasePath[1024];
                sprintf(DataBasePath, "%s_Confirm.db", RequestPath);

                printf("%s\n", DataBasePath);

                FILE *fp = fopen(DataBasePath, "a");
                fprintf(fp, "%d\r\n", atoi(Confirmed + 8));
                fclose(fp);
            }

            if ((eindex = strstr(RequestParameters, "edit-line=")) != NULL && (edit = strstr(RequestParameters, "add=")) != NULL)
            {
                //add comment

                eindex += 10;
                int CurrentIndex = atoi(eindex);

                char CommentBuffer[1024];
                sprintf(CommentBuffer, "%d=%s", CurrentIndex, edit + 4);

                if (CurrentIndex != 0)
                {
                    int IsCommented = FALSE;

                    char DataBasePath[1024];
                    sprintf(DataBasePath, "./%s.db", RequestPath);

                    FILE *fp = fopen(DataBasePath, "r+");
                    for (;;)
                    {
                        if (fgets(buffer, sizeof(buffer) - 1, fp) != NULL)
                        {
                            strtok(buffer, "\r\n");

                            if (lstrcmpi(CommentBuffer, buffer) == 0)
                            {
                                IsCommented = TRUE;
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    fclose(fp);

                    if (IsCommented == FALSE)
                    {
                        char DecodeString[sizeof(CommentBuffer)];
                        DecodeUrl(DecodeString, sizeof(DecodeString), CommentBuffer);

                        fp = fopen(DataBasePath, "a");
                        fprintf(fp, "%s\r\n", DecodeString);
                        fclose(fp);
                    }
                }

                const char *BackToThePage = "" CRLF
                "<html>" CRLF
                "<script>history.back();" CRLF
                "</script>" CRLF
                "</html>" CRLF;

                send_(ResponseSocket, BackToThePage, strlen(BackToThePage), 0);

            }
            else if ((start = strstr(RequestParameters, "line=")) != NULL && (end = strstr(RequestParameters, "end=")) != NULL)
            {
                char Date[1024];
                char Name[1024];
                char Domain[1024];

                WIN32_FIND_DATA FindData;
                ZeroMemory(&FindData, sizeof(FindData));

                HANDLE hFind = NULL;

                if ((hFind = FindFirstFile(RequestPath, &FindData)) != FALSE)
                {
                    CloseHandle(hFind);

                    const char *html1 = "<!DOCTYPE html>" CRLF
                                        "<html lang=\"en\">" CRLF
                                        "<head>"
                                        "<meta charset=\"UTF-8\">" CRLF
                                        "<meta name=\"viewprot\" content=\"width=device-width, initial-scale=1.0\">" CRLF
                                        "<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">" CRLF
                                        "<title>Document</title>" CRLF
                                        "</head>" CRLF
                                        "<body>" CRLF
                                        "<div class=\"container\" id = \"divPosition\"> " CRLF
                                        "<br>";

                    const char *html2 = "<br>" CRLF
                                        "<tr>" CRLF
                                        "<td>Date</td>&nbsp;" CRLF
                                        "%s&nbsp;"
                                        "<br>" CRLF
                                        "<td>Company Name</td>&nbsp;" CRLF
                                        "%s&nbsp;"
                                        "<br>" CRLF
                                        "<form action=\"%s\" method=\"get\">" CRLF
                                        "<input type=\"hidden\" name=\"Confirm\" value=\"%d\">" CRLF
                                        "<input type=\"hidden\" name=\"line\" value=\"%d\">" CRLF
                                        "<input type=\"hidden\" name=\"end\" value=\"%d\">" CRLF
                                        "<button type=\"submit\">Confirm</button>" CRLF
                                        "</form>" CRLF
                                        "%s" CRLF
                                        "<br>" CRLF
                                        "%s&nbsp;" CRLF
                                        "<br>" CRLF
                                        "<form action=\"/%s\">"
                                        "<label for=\"add\"> </label><input type=\"text\" id=\"add\" name=\"edit-line=%d&add=\" placeholder=\"%s\">&nbsp&nbsp <input type=\"submit\" value=\"submit\">"
                                        "</form>"
                                        "</td>" CRLF
                                        "<tr>";

                    const char *bar = "" CRLF
                                      "<form action=\"%s\" method=\"get\">" CRLF
                                      "<input type=\"hidden\" name=\"line\" value=\"%d\">" CRLF
                                      "<input type=\"hidden\" name=\"end\" value=\"%d\">" CRLF
                                      "<button type=\"submit\">back</button>" CRLF
                                      "</form>" CRLF
                                      "<form action=\"%s\" method=\"get\">" CRLF
                                      "<input type=\"hidden\" name=\"line\" value=\"%d\">" CRLF
                                      "<input type=\"hidden\" name=\"end\" value=\"%d\">" CRLF
                                      "<button type=\"submit\">next</button>" CRLF
                                      "</form>" CRLF
                                      "</tr>";

                    const char *html3 = "</tr>" CRLF
                                        "<br>" CRLF
                                        "</div>" CRLF
                                        "</body>" CRLF
                                        "</html>";

                    start += 5;
                    end += 4;

                    int StartLine = atoi(start);
                    int EndLine = atoi(end);

                    FileData = (char *)GlobalAlloc(GPTR, FindData.nFileSizeLow + (strlen(html2) * (EndLine - StartLine)));
                    Response = (char *)GlobalAlloc(GPTR, FindData.nFileSizeLow + (strlen(html2) * (EndLine - StartLine)) + 1024);

                    FileDataOriginal = FileData;

                    if (Response != NULL && FileData != NULL)
                    {
                        if (StartLine != 0 && EndLine != 0)
                        {
                            printf("line=%d; end=%d\n", StartLine, EndLine);

                            FILE *fp = fopen(RequestPath, "r");
                            if (fp != NULL)
                            {

                                for (int i = 0; i != StartLine;)
                                {
                                    if (fgets(buffer, sizeof(buffer) - 1, fp) == NULL)
                                        break;
                                    else
                                    {
                                        if (buffer[0] != '\r' && buffer[0] != '\n')
                                        {
                                            i += 1;
                                        }
                                    }
                                }

                                char *_FileData = FileData;
                                int ContentLength = 0;

                                sprintf(FileData, "%s", html1);
                                FileData += strlen(FileData);

                                int LineCount = EndLine - StartLine;
                                for (int i = 0; i <= LineCount;)
                                {
                                    if (fgets(FileData, FindData.nFileSizeLow - 1, fp) == NULL)
                                        break;
                                    else
                                    {
                                        if (FileData[0] != '\r' && FileData[0] != '\n')
                                        {
                                            GetCsvMember(Date, FileData, 1);
                                            GetCsvMember(Name, FileData, 2);
                                            GetCsvMember(Domain, FileData, 5);

                                            int f = FALSE;

                                            char DataBasePath[1024];
                                            sprintf(DataBasePath, "./%s.db", RequestPath);

                                            FILE *rfp = fopen(DataBasePath, "r");
                                            for (;;)
                                            {
                                                if (fgets(buffer, sizeof(buffer), rfp) == NULL)
                                                    break;

                                                int index = atoi(buffer);

                                                if (index == StartLine + i)
                                                {
                                                    strtok(buffer, CRLF);
                                                    int CommentLength = strlen(buffer);

                                                    int j = 0;
                                                    for (;; ++j)
                                                    {
                                                        if (buffer[j] == '=')
                                                        {
                                                            for (;;)
                                                            {
                                                                if (buffer[j] != '=')
                                                                    break;
                                                                ++j;
                                                            }

                                                            break;
                                                        }
                                                    }

                                                    printf("comment=%s\n", buffer + j);

                                                    sprintf(DataBasePath, "%s_Confirm.db", RequestPath);

                                                    sprintf(FileData, html2,
                                                            Date,
                                                            Name,
                                                            RequestPath,
                                                            index,
                                                            StartLine,
                                                            EndLine,
                                                            "NO",
                                                            Domain,
                                                            RequestPath,
                                                            index,
                                                            buffer + j);
                                                    
                                                    char _buffer[sizeof(buffer)];
                                                    FILE *ConfirmCheckFile = fopen(DataBasePath, "r");
                                                    for (;;)
                                                    {
                                                        if (fgets(_buffer, sizeof(_buffer), ConfirmCheckFile) != NULL)
                                                        {
                                                            if (index == atoi(_buffer))
                                                            {
                                                                sprintf(FileData, html2,
                                                                        Date,
                                                                        Name,
                                                                        RequestPath,
                                                                        index,
                                                                        StartLine,
                                                                        EndLine,
                                                                        "YES",
                                                                        Domain,
                                                                        RequestPath,
                                                                        index,
                                                                        buffer + j);
                                                                break;
                                                            }
                                                        }
                                                        else
                                                        {
                                                            break;
                                                        }
                                                    }
                                                    fclose(ConfirmCheckFile);

                                                    FileData += strlen(FileData);

                                                    f = TRUE;
                                                }
                                            }
                                            fclose(rfp);

                                            if (f == FALSE)
                                            {

                                                sprintf(DataBasePath, "%s_Confirm.db", RequestPath);

                                                sprintf(FileData, html2,
                                                        Date,
                                                        Name,
                                                        RequestPath,
                                                        StartLine + i,
                                                        StartLine,
                                                        EndLine,
                                                        "NO",
                                                        Domain,
                                                        RequestPath,
                                                        StartLine + i,
                                                        "");

                                                FILE *ConfirmCheckFile = fopen(DataBasePath, "r");
                                                for (;;)
                                                {
                                                    if (fgets(buffer, sizeof(buffer), ConfirmCheckFile) != NULL)
                                                    {
                                                        if (StartLine + i == atoi(buffer))
                                                        {
                                                            sprintf(FileData, html2,
                                                                    Date,
                                                                    Name,
                                                                    RequestPath,
                                                                    StartLine + i,
                                                                    StartLine,
                                                                    EndLine,
                                                                    "YES",
                                                                    Domain,
                                                                    RequestPath,
                                                                    StartLine + i,
                                                                    "");
                                                            break;
                                                        }
                                                    }
                                                    else
                                                    {
                                                        break;
                                                    }
                                                }
                                                fclose(ConfirmCheckFile);

                                                FileData += strlen(FileData);
                                            }
                                            i += 1;
                                        }
                                    }
                                }

                                if (StartLine - 20 < 1)
                                {
                                    sprintf(FileData, bar, DecodeString, 1, 20, DecodeString, 21, 40);
                                }

                                if (StartLine - 20 >= 1)
                                {
                                    sprintf(FileData, bar,
                                            DecodeString, StartLine - 20, EndLine - 20,
                                            DecodeString, StartLine + 40, EndLine + 40);
                                }

                                strcat(FileData, html3);

                                fclose(fp);

                                int length = CreateHttpRaw(Response, GlobalSize(Response),
                                                           "HTTP/1.1 200 OK",
                                                           "Server", "admin-page",
                                                           "Content-Type", "text/html; charset=euc-kr",
                                                           "Content-Length", strlen(_FileData),
                                                           "Content", _FileData);

                                send_(ResponseSocket, Response, length, 0);

                                printf("%s", Response);

                                FileData = _FileData;
                            }
                        }
                    }
                }
            }
            else
            {
                WIN32_FIND_DATA FindData;
                ZeroMemory(&FindData, sizeof(FindData));

                if (FindFirstFile(RequestPath, &FindData) != FALSE)
                {
                    FileData = (char *)GlobalAlloc(GPTR, FindData.nFileSizeLow);
                    Response = (char *)GlobalAlloc(GPTR, FindData.nFileSizeLow + 1024);

                    FileDataOriginal = FileData;

                    if (FileData != NULL && Response != NULL)
                    {
                        HANDLE hFile = CreateFile(RequestPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                        ReadFile(hFile, FileData, FindData.nFileSizeLow, NULL, NULL);
                        CloseHandle(hFile);

                        int length = CreateHttpRaw(Response, GlobalSize(Response),
                                                   "HTTP/1.1 200 OK",
                                                   "Server", "admin-page",
                                                   "Content-Type", "text/html; charset=euc-kr",
                                                   "Content-Length", FindData.nFileSizeLow,
                                                   "Content", FileData);

                        if (lstrcmpi(RequestPath, "./index.html") == 0)
                        {
                            char id[1024];
                            char pw[1024];

                            char *ptr = strstr(RequestParameters, "id=");
                            if (ptr != NULL)
                            {
                                ptr += 3;
                                memcpy(id, ptr, strlen(ptr) + 1);

                                int IdLength = strlen(id);
                                for (int i = 0; i != IdLength; i += 1)
                                {
                                    if (id[i] == '&')
                                    {
                                        id[i] = 0;
                                    }
                                }
                            }

                            ptr = strstr(RequestParameters, "pw=");
                            if (ptr != NULL)
                            {
                                ptr += 3;
                                memcpy(pw, ptr, strlen(ptr) + 1);
                            }

                            printf("id=%s\n", id);
                            printf("pw=%s\n", pw);

                            char _id[sizeof(id)];
                            char _pw[sizeof(pw)];

                            FILE *loginDB = fopen("./login.db", "r");
                            if (loginDB != NULL)
                            {

                                for (;;)
                                {
                                    if (fgets(_id, sizeof(_id) - 1, loginDB) == NULL)
                                        break;
                                    if (fgets(_pw, sizeof(_pw) - 1, loginDB) == NULL)
                                        break;

                                    strtok(_id, CRLF);
                                    strtok(_pw, CRLF);

                                    printf("%s\n", _id + 3);
                                    printf("%s\n", _pw + 3);

                                    if (lstrcmpi(_id + 3, id) == 0)
                                    {
                                        if (lstrcmpi(_pw + 3, pw) == 0)
                                        {
                                            send_(ResponseSocket, Response, length, 0);
                                        }
                                    }
                                }
                                fclose(loginDB);
                            }
                            else
                            {
                                printf("login.db error\n");
                            }
                        }
                        else
                        {
                            send_(ResponseSocket, Response, length, 0);
                        }
                    }
                }
            }

            shutdown_(ResponseSocket, SD_BOTH);
            closesocket_(ResponseSocket);

            printf("free=%p\n", FileDataOriginal);

            if (FileDataOriginal != NULL)
            {
                GlobalFree(FileDataOriginal);
                FileDataOriginal = NULL;
            }

            if (Response != NULL)
            {
                GlobalFree(Response);
                FileData = NULL;
            }
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
        else
        {
            system(input);
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
