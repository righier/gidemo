#include "assets.h"
#include "window.h"
#include "utils.h"

#include <queue>
#include <windows.h>



std::queue<string> fileChanges;

HANDLE mutex;
HANDLE changeHandle;
FILE_NOTIFY_INFORMATION infoBuffer[1024];

DWORD WINAPI worker(LPVOID lpParam) {
	UNUSED(lpParam);
	while(true) {
		DWORD bytesReturned;
		if (ReadDirectoryChangesW(changeHandle, infoBuffer, sizeof(infoBuffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytesReturned, NULL, NULL)) {

			auto *p = &infoBuffer[0];
			do {
				std::wstring wname(p->FileName, p->FileNameLength/2);
				string name = ws2s(wname);
				name.resize(p->FileNameLength / 2);

				WaitForSingleObject(mutex, INFINITE);

				LOG("CHANGED: ", name);
				fileChanges.push(name);

				ReleaseMutex(mutex);

				p += p->NextEntryOffset;
			} while(p->NextEntryOffset);
		}
	}
}

void AssetStore::init() {

	LOG("ASMIODFAMSIODFASIODFJ");

	mutex = CreateMutex(NULL, FALSE, NULL);

	changeHandle = CreateFile("C:/users/ermanno/desktop/gidemo/assets/shaders", 
		FILE_LIST_DIRECTORY,
		FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
		NULL, 
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL);

	if (changeHandle == INVALID_HANDLE_VALUE) {
		LOG("invalid handle error");
	} else {
		CreateThread( NULL, 0, worker, NULL, 0, NULL );
	}
}

void AssetStore::update() {
	WaitForSingleObject(mutex, INFINITE);

	while (!fileChanges.empty()) {
		string file = fileChanges.front();
		fileChanges.pop();

		auto it = deps.find(file);
		if (it != deps.end()) {
			Asset *asset = it->second;
			asset->load();
		}

	}

	ReleaseMutex(mutex);
}