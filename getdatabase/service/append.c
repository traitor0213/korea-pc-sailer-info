#include <stdio.h>
#include <windows.h>

int GetCsvMember(char *writen, int size, const char *buffer, int colum)
{
    int length = strlen(buffer);

    int y = 0;
    for (int i = 0; i != length; i += 1)
    {
        if(i >= size) break;

        if (buffer[i] == '\n')
            break;

        if (buffer[i] == ',')
        {
            colum -= 1;
            y = 0;

            if (colum == 0)
            {
                break;
            }
        }
        else
        {
            writen[y] = buffer[i];
            writen[y + 1] = 0;
            y += 1;
        }
    }

    return 0;
}


int StartTime;
int EndTime;

char domain[1024];

int GetSetting()
{
    const char *find = "Find-Link=";
    static char readed[4096];
    char *ptr = NULL;

    FILE *fp = fopen("../setting.ini", "r");
    if (fp == NULL)
        return TRUE;

    if(fgets(readed, sizeof(readed) - 1, fp) == NULL)
    {
        fclose(fp);
        return TRUE;
    }

    ptr = strstr(readed, find);
    ptr += strlen(find);
    strtok(ptr, "\r\n");
    
    //copy domain
    strcpy(domain, ptr);

    if(fgets(readed, sizeof(readed) - 1, fp) == NULL)
    {
        fclose(fp);
        return TRUE;
    }
    
    char *time = NULL;

    find = "Time-Range=";
    time = strstr(readed, find);
    time += strlen(find);

    char buffer[1024];
    int BufferIndex = 0;
    ZeroMemory(buffer, sizeof(buffer));

    int length = strlen(time);
    int i = 0;
    for(; i != length; i += 1)
    {   
        if(time[i] == '~') break;

        if(time[i] != '-')
        {
            buffer[BufferIndex] = time[i];
            BufferIndex += 1;
        }
    }

    StartTime = atoi(buffer);

    BufferIndex = 0;
    i += 1;
    for(; i != length; i += 1)
    {
        if(time[i] != '-')
        {
            buffer[BufferIndex] = time[i];
            BufferIndex += 1;
        }
    }

    EndTime = atoi(buffer);

    printf("current-filter-link=%s\n", domain);
    printf("current-filter-time=%d~%d\n", StartTime, EndTime);

    fclose(fp);

    return FALSE;
}

void bubble_sort(char **arr, int count) // 매개변수로 정렬할 배열과 요소의 개수를 받음
{
    char *temp;

    for (int i = 0; i < count; i++) // 요소의 개수만큼 반복
    {
        for (int j = 0; j < count - 1; j++) // 요소의 개수 - 1만큼 반복
        {
            if (atoi(arr[j] + 26) < atoi(arr[j + 1] + 26) ) // 현재 요소의 값과 다음 요소의 값을 비교하여
            {                                    // 작은 값을

                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp; // 다음 요소로 보냄
            }
        }
    }
}

#define BUFFER_SIZE 1024

int WriteBindDataBase(const char *BindDataBaseFilePath, int *Filter, int FilterSize)
{
    int AddLineCount = 0;

    FILE *BindDataBaseFilePointer = NULL;
    BindDataBaseFilePointer = fopen(BindDataBaseFilePath, "w");

    FILE *DataBaseFilePointer = NULL;
    const char *DataBasePath = "./database/%d.csv";

    int DataBaseFileIndex = 0;
    char DataBaseFilePath[BUFFER_SIZE];
    memset(DataBaseFilePath, 0, sizeof(DataBasePath));

    printf("[+] merge database..\n");

    for(;;)
    {
        sprintf(DataBaseFilePath, DataBasePath, DataBaseFileIndex);

        char LineBuffer[BUFFER_SIZE];
        char CsvBuffer[BUFFER_SIZE];
        char WriteBuffer[BUFFER_SIZE];

        char TimeBuffer[BUFFER_SIZE];

        DataBaseFilePointer = fopen(DataBaseFilePath, "r");
        //The file doe's not exist
        if(DataBaseFilePointer == NULL) break;

        while(fgets(LineBuffer, sizeof(LineBuffer), DataBaseFilePointer) != NULL)
        {
            memset(WriteBuffer, 0, sizeof(WriteBuffer));
            
            int FilterIndex = 0;
            for(; FilterIndex != FilterSize; FilterIndex += 1)
            {
                ZeroMemory(CsvBuffer, sizeof(CsvBuffer));
                GetCsvMember(CsvBuffer, sizeof(CsvBuffer), LineBuffer, Filter[FilterIndex]);
                
                if(FilterIndex == 0) //time
                {
                    if(atoi(CsvBuffer) == 0 || atoi(CsvBuffer + 5) == 0 || atoi(CsvBuffer + 8) == 0)
                    {
                        FilterIndex = -1;
                        break;
                    }
                    
                    sprintf(TimeBuffer, "%0004d%02d%02d", atoi(CsvBuffer), atoi(CsvBuffer + 5), atoi(CsvBuffer + 8));
                    sprintf(WriteBuffer, "%s%s", WriteBuffer, TimeBuffer);
                    
                    int CurrentTime = atoi(TimeBuffer);

                    if( (CurrentTime >= StartTime && CurrentTime <= EndTime) == FALSE)
                    {
                        FilterIndex = -1;
                        break;
                    }

                    //TimeBuffer
                }
                else if(FilterIndex + 1 == FilterSize)
                {
                    if(strstr(CsvBuffer, "//") == NULL)
                    {
                        FilterIndex = -1;
                        break;
                    }

                    sprintf(WriteBuffer, "%s<a href=\"%s\">%s</a>", WriteBuffer, CsvBuffer, CsvBuffer);
                }
                else 
                {
                    sprintf(WriteBuffer, "%s<td>%s</td>", WriteBuffer, CsvBuffer);
                }
            }
            
            if(FilterIndex != -1)
            {
                fprintf(BindDataBaseFilePointer,    "<br><table border=\"1\"><tr>%s</tr>" 
                "<br>"
                "<form action=\"_%%s\" method=\"GET\">"
                "<input type=\"text\" name=\"comment-%%d\" />"
                "<input type=\"submit\" />"
                "</form>"
                "</table>\r\n", WriteBuffer);
                                                    
                AddLineCount += 1;
            }
        }

        fclose(DataBaseFilePointer);
        
        DataBaseFileIndex += 1;
    }
    fclose(BindDataBaseFilePointer);
    
    printf("[+] done!\n");


    return AddLineCount;
}

int main()
{
    GetSetting();

    int Filter[] = {
        9,  //신고일자
        3,  //상호
        6,  //대표자명
        15, //취급품목
        16 //link
    };
    int FilterSize = sizeof(Filter) / sizeof(int);
    const char *BindDataBaseFilePath = "./bind.csv";

    int DBLINE_COUNT = WriteBindDataBase(BindDataBaseFilePath, Filter, FilterSize);

    char buffer[BUFFER_SIZE];

    int FAddLineCount = 0;
    int NAddLineCount = 0;

    const char *FilterDataBaseFilePath = "../../server/backend/filter-append.csv";
    const char *NotFilterDataBaseFilePath = "../../server/backend/not-filter-append.csv";

    const char *FilterDataBaseFileName = "filter-append.csv";
    const char *NotFilterDataBaseFileName = "not-filter-append.csv";

    FILE *DataBaseFile = fopen(BindDataBaseFilePath, "r");
    FILE *FilterDataBase = FilterDataBase = fopen(FilterDataBaseFilePath, "w");;
    FILE *NotFilterDataBase = fopen(NotFilterDataBaseFilePath, "w");;
    for(;;)
    {
        if(fgets(buffer, sizeof(buffer) - 1, DataBaseFile) == NULL) break;

        if(strstr(buffer, domain) != NULL)
        {
            fprintf(FilterDataBase, "%s", buffer);
            FAddLineCount += 1;
        }
        else 
        {
            fprintf(NotFilterDataBase, "%s", buffer);
            NAddLineCount += 1;
        }
    }
    fclose(NotFilterDataBase);
    fclose(FilterDataBase);
    fclose(DataBaseFile);

    //sort
    
    char **LineBuffer = (char **)malloc(FAddLineCount * sizeof(char *));
    for(int i = 0; i != FAddLineCount; i += 1)
    {
        LineBuffer[i] = malloc(BUFFER_SIZE);
    }

    FilterDataBase = fopen(FilterDataBaseFilePath, "r");
    for(int i = 0;fgets(LineBuffer[i], BUFFER_SIZE, FilterDataBase) != NULL; i++);
    fclose(FilterDataBase);

    printf("[+] start bubble-sort..\n");
    bubble_sort(LineBuffer, FAddLineCount);
    printf("[+] done!\n");

    printf("%s\n", FilterDataBaseFileName);

    FilterDataBase = fopen(FilterDataBaseFilePath, "w");
    for(int i = 0; i != FAddLineCount != 0; i ++)
    {
        fprintf(FilterDataBase, LineBuffer[i], FilterDataBaseFileName, i);
    }
    fclose(FilterDataBase);

    for(int i = 0; i != FAddLineCount; i += 1)
    {
        free(LineBuffer[i]);
    }
    free(LineBuffer);

    //sort

    LineBuffer = (char **)malloc(NAddLineCount * sizeof(char *));
    for(int i = 0; i != NAddLineCount; i += 1)
    {
        LineBuffer[i] = malloc(BUFFER_SIZE);
    }

    FilterDataBase = fopen(NotFilterDataBaseFilePath, "r");
    for(int i = 0;fgets(LineBuffer[i], BUFFER_SIZE, FilterDataBase) != NULL; i++);
    fclose(FilterDataBase);

    printf("[+] start bubble-sort..\n");
    bubble_sort(LineBuffer, NAddLineCount);
    printf("[+] done!\n");

    printf("%s\n", NotFilterDataBaseFileName);

    FilterDataBase = fopen(NotFilterDataBaseFilePath, "w");
    for(int i = 0; i != NAddLineCount != 0; i ++)
    {
        if(strstr(LineBuffer[i], domain) == NULL)
        {
            fprintf(FilterDataBase, LineBuffer[i], NotFilterDataBaseFileName, i);
        }
    }
    fclose(FilterDataBase);

    for(int i = 0; i != NAddLineCount; i += 1)
    {
        free(LineBuffer[i]);
    }
    free(LineBuffer);

    return 0;
}

//done