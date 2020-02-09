#include <stdio.h>
#include <windows.h>

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
            if (atoi(arr[j]) < atoi(arr[j + 1])) // 현재 요소의 값과 다음 요소의 값을 비교하여
            {                                    // 작은 값을
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp; // 다음 요소로 보냄
            }
        }
    }
}

int main()
{
    GetSetting();

    int filter[] = {
        9,  //신고일자
        3,  //상호
        6,  //대표자명
        15, //취급품목
        16  //domain
    };
    int l = sizeof(filter) / sizeof(int);

    DWORD TotalLines = 0;
    DWORD TotalSize = 0;

    DWORD OpenCount = 0;

    FILE *fp;
    char line[1024];
    char parsed[1024];

    char WriteBuffer[4096];

    char path[1024];
    int PathIndex = 0;

    FILE *append = fopen("../../server/backend/selected-append.csv", "w");
    FILE *all = fopen("../../server/backend/all-append.csv", "w");

    printf("[+] startup parsing..");

    for (;;)
    {
        wsprintfA(path, "database/%d.csv", PathIndex++);

        fp = fopen(path, "r");
        if (fp == NULL)
            break;

        OpenCount += 1;

        //printf("[+] path='%s'\n", path);

        if (fgets(line, sizeof(line), fp) != NULL)
        {
            for (;;)
            {
                if (fgets(line, sizeof(line), fp) == NULL)
                    break;

                for (int i = 0; i != l; i += 1)
                {
                    GetCsvMember(parsed, line, filter[i]);

                    if (filter[i] == 16)
                    {
                        if (strstr(parsed, domain) != NULL)
                        {
                            fprintf(append, "%s", WriteBuffer);
                            fprintf(append, "%s\r\n", parsed);
                            TotalLines += 1;
                        }
                        else if (strstr(parsed, "http://") != NULL)
                        {
                            fprintf(all, "%s", WriteBuffer);
                            fprintf(all, "%s\r\n", parsed);
                        }

                        TotalSize += strlen(WriteBuffer) + strlen(parsed);

                        ZeroMemory(WriteBuffer, sizeof(WriteBuffer));
                        break;
                    }
                    else
                    {
                        if (filter[i] == 9)
                        {
                            sprintf(WriteBuffer, "%s%0004d%02d%02d, ", WriteBuffer, atoi(parsed), atoi(parsed + 6), atoi(parsed + 8));
                        }
                        else
                        {
                            sprintf(WriteBuffer, "%s%s, ", WriteBuffer, parsed);
                        }
                    }
                }
            }
        }
        fclose(fp);
    }
    fclose(append);
    fclose(all);

    printf("\n[+] done!\n");

    const int size = 1024;

    int *years = (int *)malloc(TotalLines * sizeof(int));
    char **list = (char **)malloc(TotalLines * sizeof(char *));
    for (int i = 0; i != TotalLines; i += 1)
    {
        list[i] = (char *)malloc(size);
    }

    //Swap
    DWORD ListIndex = 0;

    append = fopen("../../server/backend/sort-append.csv", "r");
    for (;;)
    {
        if (fgets(list[ListIndex], size - 1, append) == NULL)
            break;

        ListIndex += 1;
        //printf("%s\n", list[ListIndex]);
    }
    fclose(append);

    printf("[+] startup sorting.. line=%d\n", TotalLines);
    bubble_sort(list, TotalLines);
    printf("[+] done!\n");

    printf("[+] write '../../server/backend/sort-append.csv'\n");

    append = fopen("../../server/backend/sort-append.csv", "w");

    for (int index = 0; index != TotalLines; index += 1)
    { 
        int CurrentDataTime = atoi(list[index]);

        if(CurrentDataTime >= StartTime && CurrentDataTime <= EndTime)
        {
            fprintf(append, "%s", list[index]);
        }
    }
    fclose(append);
    printf("[+] done!\n");

    printf("[+] copying..\n");


    for (int i = 0; i != TotalLines; i += 1)
    {
        list[i] = (char *)malloc(1024);
    }
    free(list);

    return 0;
}

//done