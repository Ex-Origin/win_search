#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <psapi.h>

struct MemoryInfo
{
    void* start;
    size_t length;
#define ENABLE 1
    char r, w, x;
    char* maps;
};

int getMemoryInfo(HANDLE hProcess, struct MemoryInfo* memory, int amount);
void printMemoryInfo(struct MemoryInfo* memory, int amount);
int getMaxLength(struct MemoryInfo* memory, int amount);
int findstr(char* content, char* search, int content_length);


int main(int argc, char* argv[])
{
    struct MemoryInfo* memory;
    int amount, i, ii, j, jj;
    HANDLE hProcess;
    int pid;
    int max_len, length;
    char* addr;
    size_t result, * size_ptr;
    int* int_ptr;
    char* char_ptr, * search;
    int offset, int_r;
    char* num1, * num2;
    int number;
    char buf[0x100];
    char* trim;

    FILE* fp;

    fp = fopen("pid", "r");
    if (fp == NULL)
    {
        setbuf(stdout, NULL);
        printf("Input pid: ");
        scanf("%s", buf);
        pid = atoi(buf);

        fp = fopen("pid", "w");
        fwrite(buf, 1, sizeof(buf), fp);
        fclose(fp);
    }
    else
    {
        ZeroMemory(buf, sizeof(buf));
        fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
        pid = atoi(buf);
    }


    printf("Input num: ");
    scanf("%s", buf);
    trim = strchr(buf, '-');
    if (trim != NULL)
    {
        /* 0abcd000-0abce000 */
        number = 2;
        sscanf(buf, "%p", &num1);
        sscanf(trim + 1, "%p", &num2);
    }
    else
    {
        /* 0abcd000 */
        number = 1;
        sscanf(buf, "%p", &num1);
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess == NULL)
    {
        printf("OpenProcess GetLastError %d\n", GetLastError());
        ExitProcess(EXIT_FAILURE);
    }
    memory = (struct MemoryInfo*)malloc(0x1000000);
    amount = getMemoryInfo(hProcess, memory, 0x1000000 / sizeof(memory[0]));
    // printMemoryInfo(memory, amount);

    max_len = getMaxLength(memory, amount);
    addr = (char*)malloc(max_len);

    for (i = 0; i < amount; i++)
    {
        if (memory[i].r == ENABLE || memory[i].w == ENABLE || memory[i].x == ENABLE)
        {
            if (ReadProcessMemory(hProcess, memory[i].start, addr, memory[i].length, &result) == FALSE)
            {
                if (GetLastError() != 299)
                {
                    if (memory[i].maps)
                    {
                        printf("%20p%20p%10x  %s\n", memory[i].start, (char*)memory[i].start + memory[i].length, (unsigned int)memory[i].length, memory[i].maps);
                    }
                    else
                    {
                        printf("%20p%20p%10x\n", memory[i].start, (char*)memory[i].start + memory[i].length, (unsigned int)memory[i].length);
                    }
                    printf("ReadProcessMemory GetLastError %d\n", GetLastError());
                }
                continue;
            }
            length = result;
            setvbuf(stdout, NULL, _IOLBF, 1024);

            /* Search code begin */
            char_ptr = (char*)addr;
            for (j = 0; j < length; j += sizeof(size_t))
            {
                if (number == 1)
                {
                    if (*(size_t*)(char_ptr + j) == (size_t)num1)
                    {
                        printf("%p\n", (char*)memory[i].start + j);
                    }
                }
                else if (number == 2)
                {
                    if (*(size_t*)(char_ptr + j) >= (size_t)num1 && *(size_t*)(char_ptr + j) <= (size_t)num2)
                    {
                        if (((size_t)memory[i].start + j) < (size_t)num1 || ((size_t)memory[i].start + j) > (size_t)num2)
                        {
                            printf("%p -> %p\n", (char*)memory[i].start + j, *(size_t**)(char_ptr + j));
                        }
                    }
                }
            }
            /* Search code end */
        }
    }
    return 0;
}





int getMemoryInfo(HANDLE hProcess, struct MemoryInfo* memory, int amount)
{
    LPCVOID pAddress = 0x00;
    MEMORY_BASIC_INFORMATION memInfo;
    DWORD r;
    char dlpath[1024];
    int i = 0;

    ZeroMemory(memory, sizeof(struct MemoryInfo) * amount);

    while (VirtualQueryEx(hProcess, pAddress, &memInfo, sizeof(memInfo)) != 0)
    {
        memory[i].start = memInfo.BaseAddress;
        memory[i].length = memInfo.RegionSize;

        if (memInfo.AllocationProtect & PAGE_NOACCESS)
        {

        }
        if (memInfo.AllocationProtect & PAGE_READONLY)
        {
            memory[i].r = ENABLE;
        }
        if (memInfo.AllocationProtect & PAGE_READWRITE)
        {
            memory[i].r = ENABLE;
            memory[i].w = ENABLE;
        }
        if (memInfo.AllocationProtect & PAGE_WRITECOPY)
        {
            memory[i].r = ENABLE;

            ZeroMemory(dlpath, sizeof(dlpath));
            r = GetModuleFileNameA((HMODULE)memInfo.AllocationBase, dlpath, sizeof(dlpath));
            memory[i].maps = malloc(strlen(dlpath) + 1);
            if (memory[i].maps)
            {
                memcpy(memory[i].maps, dlpath, strlen(dlpath) + 1);
            }
        }
        if (memInfo.AllocationProtect & PAGE_EXECUTE)
        {
            memory[i].x = ENABLE;
        }
        if (memInfo.AllocationProtect & PAGE_EXECUTE_READ)
        {
            memory[i].x = ENABLE;
            memory[i].r = ENABLE;
        }
        if (memInfo.AllocationProtect & PAGE_EXECUTE_READWRITE)
        {
            memory[i].x = ENABLE;
            memory[i].r = ENABLE;
            memory[i].w = ENABLE;
        }
        if (memInfo.AllocationProtect & PAGE_EXECUTE_WRITECOPY)
        {
            memory[i].r = ENABLE;

            ZeroMemory(dlpath, sizeof(dlpath));
            r = GetModuleFileNameA((HMODULE)memInfo.AllocationBase, dlpath, sizeof(dlpath));
            memory[i].maps = malloc(strlen(dlpath) + 1);
            if (memory[i].maps)
            {
                memcpy(memory[i].maps, dlpath, strlen(dlpath) + 1);
            }
        }

        pAddress = (PVOID)((PBYTE)pAddress + memInfo.RegionSize);
        i++;
    }
    return i;
}

void printMemoryInfo(struct MemoryInfo* memory, int amount)
{
    char rwx[0x100];
    int i;
    for (i = 0; i < amount; i++)
    {
        strncpy(rwx, "----", sizeof(rwx));
        if (memory[i].r == ENABLE)
        {
            rwx[0] = 'r';
        }
        if (memory[i].w == ENABLE)
        {
            rwx[1] = 'w';
        }
        if (memory[i].x == ENABLE)
        {
            rwx[2] = 'x';
        }
        rwx[3] = 'p';
        if (memory[i].maps)
        {
            printf("%20p%20p%5s%10x  %s\n", memory[i].start, (char*)memory[i].start + memory[i].length, rwx, (unsigned int)memory[i].length, memory[i].maps);
        }
        else
        {
            printf("%20p%20p%5s%10x\n", memory[i].start, (char*)memory[i].start + memory[i].length, rwx, (unsigned int)memory[i].length);
        }
    }
}

int getMaxLength(struct MemoryInfo* memory, int amount)
{
    int maxLength = 0;
    int i;
    for (i = 0; i < amount; i++)
    {
        if (memory[i].r == ENABLE || memory[i].w == ENABLE || memory[i].x == ENABLE)
        {
            if (memory[i].length > maxLength)
            {
                maxLength = memory[i].length;
            }
        }
    }
    return maxLength;
}

void get_next(int next[], char* str, int length)
{
    int j = 0, k = -1;
    next[0] = -1;
    while (j < length - 1)
    {
        if (k == -1 || str[j] == str[k])
        {
            j++;
            k++;
            next[j] = k;
        }
        else
            k = next[k];
    }
}

void get_nextval(int next[], char* str, int length)
{
    int i;
    for (i = 1; i < length; i++)
    {
        if (str[i] == str[next[i]])
        {
            next[i] = next[next[i]];
        }
    }
}

int KMP(char* content, int content_length, char* search, int search_length, int* extern_space)
{
    int* next = extern_space, i = 0, j = 0;
    get_next(next, search, search_length);
    get_nextval(next, search, search_length);
    while (i < content_length && j < search_length)
    {
        if (j == -1 || content[i] == search[j])
        {
            i++;
            j++;
        }
        else
            j = next[j];
    }
    if ((unsigned int)j >= search_length)
        return (i - search_length);
    else
        return (-1);
}

void add_next(int next[], int length)
{
    int i;
    for (i = 0; i < length; i++)
    {
        next[i]++;
    }
}

int findstr(char* content, char* search, int content_length)
{
    int next[0x100];
    return KMP(content, content_length, search, strlen(search), next);
}