#pragma once
// TP_STUB_MODULE: CoreUObject
#include "CoreMinimal.h"
enum EBulkDataLockFlags { LOCK_READ_ONLY = 1, LOCK_READ_WRITE = 2 };
class FUntypedBulkData
{
public:
	void* Lock(uint32 LockFlags); const void* LockReadOnly() const; void Unlock() const;
	void* Realloc(int64 InElementCount);
	int64 GetElementCount() const; int32 GetElementSize() const; int64 GetBulkDataSize() const; bool IsLocked() const;
	void GetCopy(void** Dest, bool bDiscardInternalCopy = true);
	void SetBulkDataFlags(uint32 BulkDataFlagsToSet);
};
class FByteBulkData : public FUntypedBulkData {};
class FWordBulkData : public FUntypedBulkData {};
class FFloatBulkData : public FUntypedBulkData {};
