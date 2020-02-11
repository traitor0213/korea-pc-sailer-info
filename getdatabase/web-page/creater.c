#include <stdio.h>
#include <windows.h>

int main()
{
    const char *FiltedDataBasePath = "../service/filter-append.csv";
    const char *NotFiltedDataBasePath = "../service/not-filter-append.csv";

    const char *HtmlPath = "./index.html";

    FILE *fp = NULL;

    fp = fopen(HtmlPath, "w");
    
    fclose(fp);

    return 0;
}