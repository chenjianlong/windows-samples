#include <Windows.h>
#include <objbase.h>
#include <vss.h>
#include <vswriter.h>
#include <vsbackup.h>
#include <comdef.h>
#include <cstdio>
#include <string>

using std::wstring;

#define CHECK_HR_RETURN(HR, FUNC, RET) \
	do { \
		_com_error err(HR); \
		if (HR != S_OK) { \
			fprintf(stderr, "Failed to call " ## FUNC ## ", %s\n", err.ErrorMessage()); \
			return RET; \
		} \
	} while (0)

int wmain(int argc, wchar_t *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %ls <src> <dst>\n", argv[0]);
		return -1;
	}

	WCHAR volume[MAX_PATH + 1] = { '\0' };
	if (GetVolumePathNameW(argv[1], volume, MAX_PATH) == FALSE) {
		fprintf(stderr, "Failed to GetVolumePathNameW\n");
		return -1;
	}

	HRESULT rc = CoInitialize(NULL);
	CHECK_HR_RETURN(rc, "CoInitialize", -1);

	IVssBackupComponents *components = NULL;
	rc = CreateVssBackupComponents(&components);
	CHECK_HR_RETURN(rc, "CreateVssBackupComponents", -1);

	rc = components->InitializeForBackup();
	CHECK_HR_RETURN(rc, "InitializeForBackup", -1);

	IVssAsync *async;
	rc = components->GatherWriterMetadata(&async);
	CHECK_HR_RETURN(rc, "GatherWriterMetadata", -1);
	rc = async->Wait();
	CHECK_HR_RETURN(rc, "async->Wait", -1);

	rc = components->SetContext(VSS_CTX_BACKUP);
	CHECK_HR_RETURN(rc, "SetContext", -1);

	VSS_ID snapshot_set_id;
	rc = components->StartSnapshotSet(&snapshot_set_id);
	CHECK_HR_RETURN(rc, "StartSnapshotSet", -1);

	VSS_ID snapshot_id;
	rc = components->AddToSnapshotSet(volume, GUID_NULL, &snapshot_id);
	CHECK_HR_RETURN(rc, "AddToSnapshotSet", -1);

	rc = components->SetBackupState(true, false, VSS_BT_FULL, false);
	CHECK_HR_RETURN(rc, "SetBackupState", -1);

	rc = components->PrepareForBackup(&async);
	CHECK_HR_RETURN(rc, "PrepareForBackup", -1);
	rc = async->Wait();
	CHECK_HR_RETURN(rc, "async->Wait", -1);

	rc = components->DoSnapshotSet(&async);
	CHECK_HR_RETURN(rc, "DoSnapshotSet", -1);
	rc = async->Wait();
	CHECK_HR_RETURN(rc, "async->Wait", -1);

	VSS_SNAPSHOT_PROP snapshot_prop;
	rc = components->GetSnapshotProperties(snapshot_id, &snapshot_prop);
	CHECK_HR_RETURN(rc, "GetSnapshotProperties", -1);

	wstring src = snapshot_prop.m_pwszSnapshotDeviceObject;
	src += L"\\";
	src += (argv[1] + lstrlenW(volume));

	if (CopyFileW(src.c_str(), argv[2], true) == FALSE) {
		fprintf(stderr, "Failed to copyfile, src=%ls, dst=%ls, last_error=%d\n",
                src.c_str(), argv[2], GetLastError());
		return -1;
	}

	rc = components->BackupComplete(&async);
	CHECK_HR_RETURN(rc, "BackupComplete", -1);
	rc = async->Wait();
	CHECK_HR_RETURN(rc, "async->Wait", -1);

	components->Release();

	fprintf(stdout, "Success copy file\n");
	return 0;
}
