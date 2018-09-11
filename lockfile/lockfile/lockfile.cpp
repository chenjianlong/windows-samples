#include <Windows.h>
#include <stdio.h>

int wmain(int argc, wchar_t *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %ls <path to lock>\n", argv[0]);
		return -1;
	}

	const HANDLE h = CreateFileW(argv[1], GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Failed to open %ls, last_error=%d\n", argv[1], GetLastError());
		return -1;
	}

	while (true) {
		Sleep(10000);
	}

	CloseHandle(h);
	return 0;
}