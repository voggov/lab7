// Lab7.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cstdio>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <locale>

using namespace std;

#define MAX_SIZE_BUFFER 64*1024
#define READ_SIZE MAX_SIZE_BUFFER
#define MAX_FILENAME_SIZE 256


void swap(char** buf1, char** buf2)
{
	char* temp = *buf1;
	*buf1 = *buf2;
	*buf2 = temp;
}
void AssyncFileCopyTaskStatic()
{
	setlocale(0, "rus");
	wchar_t path[MAX_FILENAME_SIZE];
	wchar_t path2[MAX_FILENAME_SIZE];
	printf("Введите имя файла в директории: ");
	scanf("%Ls", path);
	printf("Введите итоговое имя файла в директории: ");
	scanf("%Ls", path2);
	HANDLE hFileRead = 0, hFileWrite = 0;

	OVERLAPPED gOverlappedWrite, gOverlappedRead;

	DWORD writed, readed;
	LONGLONG fileLength = 0;

	char* buf1 = (char*)malloc(sizeof(char) * MAX_SIZE_BUFFER), * buf2 = (char*)malloc(sizeof(char) * MAX_SIZE_BUFFER);

	gOverlappedRead.hEvent = 0;
	gOverlappedRead.Offset = 0;
	gOverlappedRead.OffsetHigh = NULL;

	gOverlappedWrite.hEvent = 0;
	gOverlappedWrite.Offset = 0;
	gOverlappedWrite.OffsetHigh = NULL;

	hFileRead = CreateFile(path,
		FILE_READ_DATA,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL);
	hFileWrite = CreateFile(path2,
		FILE_WRITE_DATA,
		NULL,
		NULL,
		CREATE_ALWAYS,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL);

	if (hFileRead == INVALID_HANDLE_VALUE || hFileWrite == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD dwSizeH = 0, dwSizeL = 0;
	dwSizeL = GetFileSize(hFileRead, &dwSizeH);
	fileLength = ((LONGLONG)dwSizeH * (MAXDWORD + 1)) + dwSizeL;


	ReadFile(hFileRead, buf1, READ_SIZE, &readed, &gOverlappedRead);
	GetOverlappedResult(hFileRead, &gOverlappedRead, &readed, true);
	gOverlappedRead.Offset += readed;

	DWORD dwError;

	while (true)
	{
		fileLength -= readed;
		if (fileLength > 0) {
			ReadFile(hFileRead, buf2, MAX_SIZE_BUFFER, NULL, &gOverlappedRead);
		}
		WriteFile(hFileWrite, buf1, MAX_SIZE_BUFFER, &writed, &gOverlappedWrite);

		GetOverlappedResult(hFileWrite, &gOverlappedWrite, &writed, true);
		GetOverlappedResult(hFileRead, &gOverlappedRead, &readed, true);

		gOverlappedWrite.Offset += writed;
		gOverlappedRead.Offset += readed;
		if (fileLength <= 0) {
			printf("assync work with file ends| Task in main is running\n");
			break;
		}

		writed = readed;
		swap(&buf1, &buf2);
	}
	CloseHandle(hFileWrite);
	CloseHandle(hFileRead);

	hFileRead = CreateFile(path2,
		FILE_WRITE_DATA,
		NULL,
		NULL,
		OPEN_EXISTING,
		NULL,
		NULL);
	SetFilePointer(hFileRead, dwSizeL, (PLONG)&dwSizeH, FILE_BEGIN);
	SetEndOfFile(hFileRead);
	CloseHandle(hFileRead);
}
char buf1[10000] = { '\0' }, buf2[10000] = { '\0' };
char folders[10000] = { '\0' };
int count1 = 0;
int sizeTextBuf = 0;
bool openFile = false;

void returnLastFoundFolder(char* temp)
{
	int iStart = 0;
	int iEnd = strlen(folders);

	iStart = strrchr(folders, '|') - folders;
	char* c = &folders[iStart + 1];

	strcpy(temp, c);
	folders[iStart] = 0;
}

void recursive_find_files(char* path, OVERLAPPED ovr_write, HANDLE file = INVALID_HANDLE_VALUE)
{
	if (!openFile) {
		file = CreateFile(L"file.txt", GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, 0);
		if (file == INVALID_HANDLE_VALUE)
			return;
		else {
			openFile = true;
			sprintf_s(folders, "|%s", path);
		}
	}
	char temp[MAX_PATH] = { '\0' };
	char pathh[MAX_PATH] = { '\0' };
	char dir[MAX_PATH] = { '\0' };
	char fileName[MAX_PATH] = { '\0' };

	HANDLE handle;

	if (strlen(folders) > 1)
	{
		returnLastFoundFolder(temp);
		sprintf_s(pathh, "%s\\*.*", temp);
		WIN32_FIND_DATA fd;
		handle = FindFirstFile((LPCWSTR)pathh, &fd);
		if (handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (strcmp(".", (const char*)fd.cFileName) != 0 && strcmp("..", (const char*)fd.cFileName) != 0)
				{
					if (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
					{
						printf("dir \t%s\\%s\n", temp, fd.cFileName);
						sprintf_s(dir, "|%s\\%s", temp, fd.cFileName);
						strcat(folders, dir);
					}
					else
					{
						count1++;
						printf("file \t%s\\%s\n", temp, fd.cFileName);
						sprintf_s(fileName, "%d %s\\%s\r\n", count1, temp, fd.cFileName);
						strcat(buf1, fileName);
					}
				}
			} while (FindNextFile(handle, &fd));

			FindClose(handle);

			if (strlen(buf1) > 0)
			{
				DWORD dwWritten;
				int size;
				WaitForSingleObject(ovr_write.hEvent, INFINITE);

				ovr_write.Offset = sizeTextBuf;

				strcpy(buf2, buf1);
				buf1[0] = 0;

				size = strlen(buf2);
				sizeTextBuf += size;

				WriteFile(file, buf2, size, &dwWritten, &ovr_write);

			}

			recursive_find_files(temp, ovr_write, file);
		}
	}

	if (openFile)
	{
		CloseHandle(file);
		openFile = false;
	}
}

int FindFileAndWriteFile(char* dirName) {

	HANDLE file;
	setlocale(0, "rus");
	char* path = { dirName };
	OVERLAPPED ovr_write = { 0 };
	ovr_write.hEvent = CreateEvent(NULL, false, true, NULL);
	ovr_write.Offset = 0;
	recursive_find_files(path, ovr_write);
	printf("\n %d\n", count1);
	getchar();
	return 0;

}

int main()
{
	AssyncFileCopyTaskStatic();

}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
