/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2015 - 2018 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
  Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

  http://dokan-dev.github.io

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the Free
Software Foundation; either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "dokan.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DokanDispatchCreate)
#pragma alloc_text(PAGE, DokanCheckShareAccess)
#endif

// We must NOT call without VCB lock
PDokanFCB DokanAllocateFCB(__in PDokanVCB Vcb, __in PWCHAR FileName,
                           __in ULONG FileNameLength) {
  PDokanFCB fcb = ExAllocateFromLookasideListEx(&g_DokanFCBLookasideList);

  if (fcb == NULL) {
    return NULL;
  }

  ASSERT(Vcb != NULL);

  RtlZeroMemory(fcb, sizeof(DokanFCB));

  fcb->AdvancedFCBHeader.Resource =
      ExAllocateFromLookasideListEx(&g_DokanEResourceLookasideList);
  if (fcb->AdvancedFCBHeader.Resource == NULL) {
    ExFreeToLookasideListEx(&g_DokanFCBLookasideList, fcb);
    return NULL;
  }

  fcb->Identifier.Type = FCB;
  fcb->Identifier.Size = sizeof(DokanFCB);

  fcb->Vcb = Vcb;

  ExInitializeResourceLite(&fcb->PagingIoResource);
  ExInitializeResourceLite(fcb->AdvancedFCBHeader.Resource);

  ExInitializeFastMutex(&fcb->AdvancedFCBHeaderMutex);

#if _WIN32_WINNT >= 0x0501
  FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader,
                           &fcb->AdvancedFCBHeaderMutex);
#else
  if (DokanFsRtlTeardownPerStreamContexts) {
    FsRtlSetupAdvancedHeader(&fcb->AdvancedFCBHeader,
                             &fcb->AdvancedFCBHeaderMutex);
  }
#endif

  fcb->AdvancedFCBHeader.ValidDataLength.LowPart = 0xffffffff;
  fcb->AdvancedFCBHeader.ValidDataLength.HighPart = 0x7fffffff;

  fcb->AdvancedFCBHeader.PagingIoResource = &fcb->PagingIoResource;

  fcb->AdvancedFCBHeader.AllocationSize.QuadPart = 4096;
  fcb->AdvancedFCBHeader.FileSize.QuadPart = 4096;

  fcb->AdvancedFCBHeader.IsFastIoPossible = FastIoIsNotPossible;
  FsRtlInitializeOplock(DokanGetFcbOplock(fcb));

  fcb->FileName.Buffer = FileName;
  fcb->FileName.Length = (USHORT)FileNameLength;
  fcb->FileName.MaximumLength = (USHORT)FileNameLength;

  InitializeListHead(&fcb->NextCCB);
  InsertTailList(&Vcb->NextFCB, &fcb->NextFCB);

  InterlockedIncrement(&Vcb->FcbAllocated);

  return fcb;
}

PDokanFCB DokanGetFCB(__in PDokanVCB Vcb, __in PWCHAR FileName,
                      __in ULONG FileNameLength, BOOLEAN CaseSensitive) {
  PLIST_ENTRY thisEntry, nextEntry, listHead;
  PDokanFCB fcb = NULL;

  UNICODE_STRING fn;

  fn.Length = (USHORT)FileNameLength;
  fn.MaximumLength = fn.Length + sizeof(WCHAR);
  fn.Buffer = FileName;

  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&Vcb->Resource, TRUE);

  // search the FCB which is already allocated
  // (being used now)
  listHead = &Vcb->NextFCB;

  for (thisEntry = listHead->Flink; thisEntry != listHead;
       thisEntry = nextEntry) {

    nextEntry = thisEntry->Flink;

    fcb = CONTAINING_RECORD(thisEntry, DokanFCB, NextFCB);
    DokanFCBLockRO(fcb);
    DDbgPrint("  DokanGetFCB has entry FileName: %wZ FileCount: %lu. Looking "
              "for %ls\n",
              &fcb->FileName, fcb->FileCount, FileName);
    if (fcb->FileName.Length == FileNameLength // FileNameLength in bytes
        && RtlEqualUnicodeString(&fn, &fcb->FileName, !CaseSensitive)) {
      // we have the FCB which is already allocated and used
      DDbgPrint("  Found existing FCB for %ls\n", FileName);
      DokanFCBUnlock(fcb);
      break;
    }
    DokanFCBUnlock(fcb);

    fcb = NULL;
  }

  // we don't have FCB
  if (fcb == NULL) {
    DDbgPrint("  Allocate FCB for %ls\n", FileName);

    fcb = DokanAllocateFCB(Vcb, FileName, FileNameLength);

    // no memory?
    if (fcb == NULL) {
      DDbgPrint("    Was not able to get FCB for FileName %ls\n", FileName);
      ExFreePool(FileName);
      ExReleaseResourceLite(&Vcb->Resource);
      KeLeaveCriticalRegion();
      return NULL;
    }

    ASSERT(fcb != NULL);

    // we already have FCB
  } else {
    // FileName (argument) is never used and must be freed
    ExFreePool(FileName);
  }

  InterlockedIncrement(&fcb->FileCount);

  ExReleaseResourceLite(&Vcb->Resource);
  KeLeaveCriticalRegion();

  return fcb;
}

NTSTATUS
DokanFreeFCB(__in PDokanFCB Fcb) {
  PDokanVCB vcb;

  ASSERT(Fcb != NULL);

  vcb = Fcb->Vcb;

  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&vcb->Resource, TRUE);
  DokanFCBLockRW(Fcb);

  if (InterlockedDecrement(&Fcb->FileCount) == 0) {

    RemoveEntryList(&Fcb->NextFCB);
    InitializeListHead(&Fcb->NextCCB);

    DDbgPrint("  Free FCB:%p\n", Fcb);

    ExFreePool(Fcb->FileName.Buffer);
    Fcb->FileName.Buffer = NULL;
    Fcb->FileName.Length = 0;
    Fcb->FileName.MaximumLength = 0;

    FsRtlUninitializeOplock(DokanGetFcbOplock(Fcb));

#if _WIN32_WINNT >= 0x0501
    FsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
#else
    if (DokanFsRtlTeardownPerStreamContexts) {
      DokanFsRtlTeardownPerStreamContexts(&Fcb->AdvancedFCBHeader);
    }
#endif

    DokanFCBUnlock(Fcb);
    ExDeleteResourceLite(Fcb->AdvancedFCBHeader.Resource);
    ExFreeToLookasideListEx(&g_DokanEResourceLookasideList,
                            Fcb->AdvancedFCBHeader.Resource);
    ExDeleteResourceLite(&Fcb->PagingIoResource);

    InterlockedIncrement(&vcb->FcbFreed);
    ExFreeToLookasideListEx(&g_DokanFCBLookasideList, Fcb);
  } else {
    DokanFCBUnlock(Fcb);
  }

  ExReleaseResourceLite(&vcb->Resource);
  KeLeaveCriticalRegion();

  return STATUS_SUCCESS;
}

// DokanAllocateCCB must be called with exlusive Fcb lock held.
PDokanCCB DokanAllocateCCB(__in PDokanDCB Dcb, __in PDokanFCB Fcb) {
  PDokanCCB ccb = ExAllocateFromLookasideListEx(&g_DokanCCBLookasideList);

  if (ccb == NULL)
    return NULL;

  ASSERT(ccb != NULL);
  ASSERT(Fcb != NULL);

  RtlZeroMemory(ccb, sizeof(DokanCCB));

  ccb->Identifier.Type = CCB;
  ccb->Identifier.Size = sizeof(DokanCCB);

  ccb->Fcb = Fcb;
  DDbgPrint("   Allocated CCB \n");
  ExInitializeResourceLite(&ccb->Resource);

  InitializeListHead(&ccb->NextCCB);

  InsertTailList(&Fcb->NextCCB, &ccb->NextCCB);

  ccb->MountId = Dcb->MountId;

  InterlockedIncrement(&Fcb->Vcb->CcbAllocated);
  return ccb;
}

NTSTATUS
DokanFreeCCB(__in PDokanCCB ccb) {
  PDokanFCB fcb;

  ASSERT(ccb != NULL);

  fcb = ccb->Fcb;
  if (!fcb) {
    return STATUS_SUCCESS;
  }

  DokanFCBLockRW(fcb);

  DDbgPrint("   Free CCB \n");

  if (IsListEmpty(&ccb->NextCCB)) {
    DDbgPrint("  WARNING. &ccb->NextCCB is empty. \n This should never happen, "
              "so check the behavior.\n Would produce BSOD "
              "\n");
    DokanFCBUnlock(fcb);
    return STATUS_SUCCESS;
  } else {
    RemoveEntryList(&ccb->NextCCB);
    InitializeListHead(&ccb->NextCCB);
  }

  DokanFCBUnlock(fcb);

  ExDeleteResourceLite(&ccb->Resource);

  if (ccb->SearchPattern) {
    ExFreePool(ccb->SearchPattern);
  }

  ExFreeToLookasideListEx(&g_DokanCCBLookasideList, ccb);
  InterlockedIncrement(&fcb->Vcb->CcbFreed);

  return STATUS_SUCCESS;
}

// Creates a buffer from ExAllocatePool() containing
// the parent dir of file/dir pointed to by fileName.
// the buffer IS null terminated
// in *parentDirLength returns length in bytes of string (not counting null
// terminator)
// fileName MUST be null terminated
// if last char of fileName is \, then it is ignored but a slash
// is appened to the returned path
//
//  e.g. \foo\bar.txt becomes \foo
//       \foo\bar\ bcomes \foo\
//
// if there is no parent, then it return STATUS_ACCESS_DENIED
// if ExAllocatePool() fails, then it returns STATUS_INSUFFICIENT_RESOURCES
// otherwise returns STATUS_SUCCESS

NTSTATUS DokanGetParentDir(__in const WCHAR *fileName, __out WCHAR **parentDir,
                           __out ULONG *parentDirLength) {
  // first check if there is a parent

  LONG len = (LONG)wcslen(fileName);

  LONG i;

  BOOLEAN trailingSlash;

  *parentDir = NULL;
  *parentDirLength = 0;

  if (len < 1) {
    return STATUS_INVALID_PARAMETER;
  }

  if (!wcscmp(fileName, L"\\"))
    return STATUS_ACCESS_DENIED;

  trailingSlash = fileName[len - 1] == '\\';

  *parentDir = (WCHAR *)ExAllocatePool((len + 1) * sizeof(WCHAR));

  if (!*parentDir)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(*parentDir, len + 1);
  RtlStringCchCopyW(*parentDir, len, fileName);

  for (i = len - 1; i >= 0; i--) {
    if ((*parentDir)[i] == '\\') {
      if (i == len - 1 && trailingSlash) {
        continue;
      }
      (*parentDir)[i] = 0;
      break;
    }
  }

  if (i <= 0) {
    i = 1;
    (*parentDir)[0] = '\\';
    (*parentDir)[1] = 0;
  }

  *parentDirLength = i * sizeof(WCHAR);
  if (trailingSlash && i > 1) {
    (*parentDir)[i] = '\\';
    (*parentDir)[i + 1] = 0;
    *parentDirLength += sizeof(WCHAR);
  }

  return STATUS_SUCCESS;
}

LONG DokanUnicodeStringChar(__in PUNICODE_STRING UnicodeString,
                            __in WCHAR Char) {
  ULONG i = 0;
  for (; i < UnicodeString->Length / sizeof(WCHAR); ++i) {
    if (UnicodeString->Buffer[i] == Char) {
      return i;
    }
  }
  return -1;
}

VOID SetFileObjectForVCB(__in PFILE_OBJECT FileObject, __in PDokanVCB Vcb) {
  FileObject->SectionObjectPointer = &Vcb->SectionObjectPointers;
  FileObject->FsContext = &Vcb->VolumeFileHeader;
}

NTSTATUS
DokanCheckShareAccess(_In_ PFILE_OBJECT FileObject, _In_ PDokanFCB FcbOrDcb,
                      _In_ ACCESS_MASK DesiredAccess, _In_ ULONG ShareAccess)

/*++
Routine Description:
This routine checks conditions that may result in a sharing violation.
Arguments:
FileObject - Pointer to the file object of the current open request.
FcbOrDcb - Supplies a pointer to the Fcb/Dcb.
DesiredAccess - Desired access of current open request.
ShareAccess - Shared access requested by current open request.
Return Value:
If the accessor has access to the file, STATUS_SUCCESS is returned.
Otherwise, STATUS_SHARING_VIOLATION is returned.

--*/

{
  NTSTATUS status;
  PAGED_CODE();

  // Cannot open a flag with delete pending without share delete
  if ((FcbOrDcb->Identifier.Type == FCB) &&
      !FlagOn(ShareAccess, FILE_SHARE_DELETE) &&
      DokanFCBFlagsIsSet(FcbOrDcb, DOKAN_DELETE_ON_CLOSE))
    return STATUS_DELETE_PENDING;

#if (NTDDI_VERSION >= NTDDI_VISTA)
  //
  //  Do an extra test for writeable user sections if the user did not allow
  //  write sharing - this is necessary since a section may exist with no
  //  handles
  //  open to the file its based against.
  //
  if ((FcbOrDcb->Identifier.Type == FCB) &&
      !FlagOn(ShareAccess, FILE_SHARE_WRITE) &&
      FlagOn(DesiredAccess, FILE_EXECUTE | FILE_READ_DATA | FILE_WRITE_DATA |
                                FILE_APPEND_DATA | DELETE | MAXIMUM_ALLOWED) &&
      MmDoesFileHaveUserWritableReferences(&FcbOrDcb->SectionObjectPointers)) {

    DDbgPrint("  DokanCheckShareAccess FCB has no write shared access\n");
    return STATUS_SHARING_VIOLATION;
  }
#endif

  //
  //  Check if the Fcb has the proper share access.
  //
  //  Pass FALSE for update.  We will update it later.
  status = IoCheckShareAccess(DesiredAccess, ShareAccess, FileObject,
                              &FcbOrDcb->ShareAccess, FALSE);

  return status;
}

NTSTATUS
DokanDispatchCreate(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp)

/*++

Routine Description:

                This device control dispatcher handles create & close IRPs.

Arguments:

                DeviceObject - Context for the activity.
                Irp          - The device control argument block.

Return Value:

                NTSTATUS

--*/
{
  PDokanVCB vcb = NULL;
  PDokanDCB dcb = NULL;
  PIO_STACK_LOCATION irpSp = NULL;
  NTSTATUS status = STATUS_INVALID_PARAMETER;
  PFILE_OBJECT fileObject = NULL;
  ULONG info = 0;
  PEVENT_CONTEXT eventContext = NULL;
  PFILE_OBJECT relatedFileObject;
  ULONG fileNameLength = 0;
  ULONG eventLength;
  PDokanFCB fcb = NULL;
  PDokanFCB relatedFcb = NULL;
  PDokanCCB ccb = NULL;
  PWCHAR fileName = NULL;
  PWCHAR parentDir = NULL; // for SL_OPEN_TARGET_DIRECTORY
  ULONG parentDirLength = 0;
  BOOLEAN needBackSlashAfterRelatedFile = FALSE;
  BOOLEAN alternateDataStreamOfRootDir = FALSE;
  ULONG securityDescriptorSize = 0;
  ULONG alignedEventContextSize = 0;
  ULONG alignedObjectNameSize =
      PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE));
  ULONG alignedObjectTypeNameSize =
      PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE));
  PDOKAN_UNICODE_STRING_INTERMEDIATE intermediateUnicodeStr = NULL;
  PUNICODE_STRING relatedFileName = NULL;
  PSECURITY_DESCRIPTOR newFileSecurityDescriptor = NULL;
  BOOLEAN OpenRequiringOplock = FALSE;
  BOOLEAN UnwindShareAccess = FALSE;
  BOOLEAN BackoutOplock = FALSE;
  BOOLEAN EventContextConsumed = FALSE;
  DWORD disposition = 0;
  BOOLEAN fcbLocked = FALSE;

  PAGED_CODE();

  __try {
    DDbgPrint("==> DokanCreate\n");

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    if (irpSp->FileObject == NULL) {
      DDbgPrint("  irpSp->FileObject == NULL\n");
      status = STATUS_INVALID_PARAMETER;
      __leave;
    }

    fileObject = irpSp->FileObject;
    relatedFileObject = fileObject->RelatedFileObject;

    disposition = (irpSp->Parameters.Create.Options >> 24) & 0x000000ff;

    DDbgPrint("  Create: ProcessId %lu, FileName:%wZ\n",
              IoGetRequestorProcessId(Irp), &fileObject->FileName);

    vcb = DeviceObject->DeviceExtension;
    if (vcb == NULL) {
      DDbgPrint("  No device extension\n");
      status = STATUS_SUCCESS;
      __leave;
    }

    PrintIdType(vcb);

    if (GetIdentifierType(vcb) != VCB) {
      DDbgPrint("  IdentifierType is not vcb\n");
      status = STATUS_SUCCESS;
      __leave;
    }

    if (IsUnmountPendingVcb(vcb)) {
      DDbgPrint("  IdentifierType is vcb which is not mounted\n");
      status = STATUS_NO_SUCH_DEVICE;
      __leave;
    }

    dcb = vcb->Dcb;

    BOOLEAN isNetworkFileSystem =
        (dcb->VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM);

    if (!isNetworkFileSystem) {
      if (relatedFileObject != NULL) {
        fileObject->Vpb = relatedFileObject->Vpb;
      } else {
        fileObject->Vpb = dcb->DeviceObject->Vpb;
      }
    }

    if (!vcb->HasEventWait) {
      DDbgPrint("  Here we only go in if some antivirus software tries to "
                "create files before startup is finished.\n");
      if (fileObject->FileName.Length > 0) {
        DDbgPrint("  Verify if the system tries to access System Volume\n");
        UNICODE_STRING systemVolume;
        RtlInitUnicodeString(&systemVolume, L"\\System Volume Information");
        BOOLEAN isSystemVolumeAccess =
            StartsWith(&fileObject->FileName, &systemVolume);
        if (isSystemVolumeAccess) {
          DDbgPrint("  It's an access to System Volume, so don't return "
                    "SUCCESS. We don't have one.\n");
          status = STATUS_NO_SUCH_FILE;
          __leave;
        }
      }
      status = STATUS_SUCCESS;
      __leave;
    }

    DDbgPrint("  IrpSp->Flags = %d\n", irpSp->Flags);
    if (irpSp->Flags & SL_CASE_SENSITIVE) {
      DDbgPrint("  IrpSp->Flags SL_CASE_SENSITIVE\n");
    }
    if (irpSp->Flags & SL_FORCE_ACCESS_CHECK) {
      DDbgPrint("  IrpSp->Flags SL_FORCE_ACCESS_CHECK\n");
    }
    if (irpSp->Flags & SL_OPEN_PAGING_FILE) {
      DDbgPrint("  IrpSp->Flags SL_OPEN_PAGING_FILE\n");
    }
    if (irpSp->Flags & SL_OPEN_TARGET_DIRECTORY) {
      DDbgPrint("  IrpSp->Flags SL_OPEN_TARGET_DIRECTORY\n");
    }

    if ((fileObject->FileName.Length > sizeof(WCHAR)) &&
        (fileObject->FileName.Buffer[1] == L'\\') &&
        (fileObject->FileName.Buffer[0] == L'\\')) {

      fileObject->FileName.Length -= sizeof(WCHAR);

      RtlMoveMemory(&fileObject->FileName.Buffer[0],
                    &fileObject->FileName.Buffer[1],
                    fileObject->FileName.Length);
    }

    if (relatedFileObject != NULL // Get RelatedFileObject filename.
        && relatedFileObject->FsContext2) {
      // Using relatedFileObject->FileName is not safe here, use cached filename
      // from context.
      PDokanCCB relatedCcb = (PDokanCCB)relatedFileObject->FsContext2;
      if (relatedCcb->Fcb) {
        relatedFcb = relatedCcb->Fcb;
        DokanFCBLockRO(relatedFcb);
        if (relatedFcb->FileName.Length > 0 &&
            relatedFcb->FileName.Buffer != NULL) {
          relatedFileName = ExAllocatePool(sizeof(UNICODE_STRING));
          if (relatedFileName == NULL) {
            DDbgPrint("    Can't allocatePool for relatedFileName\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
            DokanFCBUnlock(relatedFcb);
            __leave;
          }
          relatedFileName->Buffer =
              ExAllocatePool(relatedFcb->FileName.MaximumLength);
          if (relatedFileName->Buffer == NULL) {
            DDbgPrint("    Can't allocatePool for relatedFileName buffer\n");
            ExFreePool(relatedFileName);
            relatedFileName = NULL;
            status = STATUS_INSUFFICIENT_RESOURCES;
            DokanFCBUnlock(relatedFcb);
            __leave;
          }
          relatedFileName->MaximumLength = relatedFcb->FileName.MaximumLength;
          relatedFileName->Length = relatedFcb->FileName.Length;
          RtlUnicodeStringCopy(relatedFileName, &relatedFcb->FileName);
        }
        DokanFCBUnlock(relatedFcb);
      }
    }

    if (relatedFileName == NULL && fileObject->FileName.Length == 0) {

      DDbgPrint("   request for FS device\n");

      if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
        status = STATUS_NOT_A_DIRECTORY;
      } else {
        SetFileObjectForVCB(fileObject, vcb);
        info = FILE_OPENED;
        status = STATUS_SUCCESS;
      }
      __leave;
    }

    if (fileObject->FileName.Length > sizeof(WCHAR) &&
        fileObject->FileName
                .Buffer[fileObject->FileName.Length / sizeof(WCHAR) - 1] ==
            L'\\') {
      fileObject->FileName.Length -= sizeof(WCHAR);
    }

    fileNameLength = fileObject->FileName.Length;
    if (relatedFileName != NULL) {
      fileNameLength += relatedFileName->Length;

      if (fileObject->FileName.Length > 0 &&
          fileObject->FileName.Buffer[0] == '\\') {
        DDbgPrint("  when RelatedFileObject is specified, the file name should "
                  "be relative path\n");
        status = STATUS_INVALID_PARAMETER;
        __leave;
      }
      if (relatedFileName->Length > 0 && fileObject->FileName.Length > 0 &&
          relatedFileName->Buffer[relatedFileName->Length / sizeof(WCHAR) -
                                  1] != '\\' &&
          fileObject->FileName.Buffer[0] != ':') {
        needBackSlashAfterRelatedFile = TRUE;
        fileNameLength += sizeof(WCHAR);
      }
      // for if we're trying to open a file that's actually an alternate data
      // stream of the root dircetory as in "\:foo"
      // in this case we won't prepend relatedFileName to the file name
      if (relatedFileName->Length / sizeof(WCHAR) == 1 &&
          fileObject->FileName.Length > 0 &&
          relatedFileName->Buffer[0] == '\\' &&
          fileObject->FileName.Buffer[0] == ':') {
        alternateDataStreamOfRootDir = TRUE;
      }
    }

    // don't open file like stream
    if (!dcb->UseAltStream &&
        DokanUnicodeStringChar(&fileObject->FileName, L':') != -1) {
      DDbgPrint("    alternate stream\n");
      status = STATUS_INVALID_PARAMETER;
      info = 0;
      __leave;
    }

    // this memory is freed by DokanGetFCB if needed
    // "+ sizeof(WCHAR)" is for the last NULL character
    fileName = ExAllocatePool(fileNameLength + sizeof(WCHAR));
    if (fileName == NULL) {
      DDbgPrint("    Can't allocatePool for fileName\n");
      status = STATUS_INSUFFICIENT_RESOURCES;
      __leave;
    }

    RtlZeroMemory(fileName, fileNameLength + sizeof(WCHAR));

    if (relatedFileName != NULL && !alternateDataStreamOfRootDir) {
      DDbgPrint("  RelatedFileName:%wZ\n", relatedFileName);

      // copy the file name of related file object
      RtlCopyMemory(fileName, relatedFileName->Buffer, relatedFileName->Length);

      if (needBackSlashAfterRelatedFile) {
        ((PWCHAR)fileName)[relatedFileName->Length / sizeof(WCHAR)] = '\\';
      }
      // copy the file name of fileObject
      RtlCopyMemory((PCHAR)fileName + relatedFileName->Length +
                        (needBackSlashAfterRelatedFile ? sizeof(WCHAR) : 0),
                    fileObject->FileName.Buffer, fileObject->FileName.Length);
    } else {
      // if related file object is not specifed, copy the file name of file
      // object
      RtlCopyMemory(fileName, fileObject->FileName.Buffer,
                    fileObject->FileName.Length);
    }

    // Fail if device is read-only and request involves a write operation

    if (IS_DEVICE_READ_ONLY(DeviceObject) &&
        ((disposition == FILE_SUPERSEDE) || (disposition == FILE_CREATE) ||
         (disposition == FILE_OVERWRITE) ||
         (disposition == FILE_OVERWRITE_IF) ||
         (irpSp->Parameters.Create.Options & FILE_DELETE_ON_CLOSE))) {

      DDbgPrint("    Media is write protected\n");
      status = STATUS_MEDIA_WRITE_PROTECTED;
      ExFreePool(fileName);
      __leave;
    }

    if (irpSp->Flags & SL_OPEN_TARGET_DIRECTORY) {
      status = DokanGetParentDir(fileName, &parentDir, &parentDirLength);
      if (status != STATUS_SUCCESS) {
        ExFreePool(fileName);
        fileName = NULL;
        __leave;
      }
      fcb = DokanGetFCB(vcb, parentDir, parentDirLength,
                        FlagOn(irpSp->Flags, SL_CASE_SENSITIVE));
    } else {
      fcb = DokanGetFCB(vcb, fileName, fileNameLength,
                        FlagOn(irpSp->Flags, SL_CASE_SENSITIVE));
    }
    if (fcb == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      __leave;
    }
    DDbgPrint("  Create: FileName:%wZ got fcb %p\n", &fileObject->FileName,
              fcb);

    // Cannot create a file already open
    if (fcb->FileCount > 1 && disposition == FILE_CREATE) {
      status = STATUS_OBJECT_NAME_COLLISION;
      __leave;
    }

    // Cannot create a directory temporary
    if (FlagOn(irpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE) &&
        FlagOn(irpSp->Parameters.Create.FileAttributes,
               FILE_ATTRIBUTE_TEMPORARY) &&
        (FILE_CREATE == disposition || FILE_OPEN_IF == disposition)) {
      status = STATUS_INVALID_PARAMETER;
      __leave;
    }

    fcbLocked = TRUE;
    DokanFCBLockRW(fcb);

    if (irpSp->Flags & SL_OPEN_PAGING_FILE) {
      status = STATUS_ACCESS_DENIED;
      __leave;
      // Paging file is not supported
      /*
       fcb->AdvancedFCBHeader.Flags2 |= FSRTL_FLAG2_IS_PAGING_FILE;
       fcb->AdvancedFCBHeader.Flags2 &= ~FSRTL_FLAG2_SUPPORTS_FILTER_CONTEXTS;
       */
    }

    ccb = DokanAllocateCCB(dcb, fcb);
    if (ccb == NULL) {
      DDbgPrint("    Was not able to allocate CCB\n");
      status = STATUS_INSUFFICIENT_RESOURCES;
      __leave;
    }

    if (irpSp->Parameters.Create.Options & FILE_OPEN_FOR_BACKUP_INTENT) {
      DDbgPrint("FILE_OPEN_FOR_BACKUP_INTENT\n");
    }

    fileObject->FsContext = &fcb->AdvancedFCBHeader;
    fileObject->FsContext2 = ccb;
    fileObject->PrivateCacheMap = NULL;
    fileObject->SectionObjectPointer = &fcb->SectionObjectPointers;
    // fileObject->Flags |= FILE_NO_INTERMEDIATE_BUFFERING;

    alignedEventContextSize = PointerAlignSize(sizeof(EVENT_CONTEXT));

    if (irpSp->Parameters.Create.SecurityContext->AccessState) {

      if (irpSp->Parameters.Create.SecurityContext->AccessState
              ->SecurityDescriptor) {
        // (CreateOptions & FILE_DIRECTORY_FILE) == FILE_DIRECTORY_FILE
        if (SeAssignSecurity(
                NULL, // we don't keep track of parents, this will have to be
                      // handled in user mode
                irpSp->Parameters.Create.SecurityContext->AccessState
                    ->SecurityDescriptor,
                &newFileSecurityDescriptor,
                (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) ||
                    (irpSp->Flags & SL_OPEN_TARGET_DIRECTORY),
                &irpSp->Parameters.Create.SecurityContext->AccessState
                     ->SubjectSecurityContext,
                IoGetFileObjectGenericMapping(), PagedPool) == STATUS_SUCCESS) {

          securityDescriptorSize = PointerAlignSize(
              RtlLengthSecurityDescriptor(newFileSecurityDescriptor));
        } else {
          newFileSecurityDescriptor = NULL;
        }
      }

      if (irpSp->Parameters.Create.SecurityContext->AccessState->ObjectName
              .Length > 0) {
        // add 1 WCHAR for NULL
        alignedObjectNameSize =
            PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) +
                             irpSp->Parameters.Create.SecurityContext
                                 ->AccessState->ObjectName.Length +
                             sizeof(WCHAR));
      }
      // else alignedObjectNameSize =
      // PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE)) SEE
      // DECLARATION

      if (irpSp->Parameters.Create.SecurityContext->AccessState->ObjectTypeName
              .Length > 0) {
        // add 1 WCHAR for NULL
        alignedObjectTypeNameSize =
            PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE) +
                             irpSp->Parameters.Create.SecurityContext
                                 ->AccessState->ObjectTypeName.Length +
                             sizeof(WCHAR));
      }
      // else alignedObjectTypeNameSize =
      // PointerAlignSize(sizeof(DOKAN_UNICODE_STRING_INTERMEDIATE)) SEE
      // DECLARATION
    }

    eventLength = alignedEventContextSize + securityDescriptorSize;
    eventLength += alignedObjectNameSize;
    eventLength += alignedObjectTypeNameSize;
    eventLength += (parentDir ? fileNameLength : fcb->FileName.Length) +
                   sizeof(WCHAR); // add WCHAR for NULL

    eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

    if (eventContext == NULL) {
      DDbgPrint("    Was not able to allocate eventContext\n");
      status = STATUS_INSUFFICIENT_RESOURCES;
      __leave;
    }

    RtlZeroMemory((char *)eventContext + alignedEventContextSize,
                  eventLength - alignedEventContextSize);

    if (irpSp->Parameters.Create.SecurityContext->AccessState) {
      // Copy security context
      eventContext->Operation.Create.SecurityContext.AccessState
          .SecurityEvaluated = irpSp->Parameters.Create.SecurityContext
                                   ->AccessState->SecurityEvaluated;
      eventContext->Operation.Create.SecurityContext.AccessState.GenerateAudit =
          irpSp->Parameters.Create.SecurityContext->AccessState->GenerateAudit;
      eventContext->Operation.Create.SecurityContext.AccessState
          .GenerateOnClose = irpSp->Parameters.Create.SecurityContext
                                 ->AccessState->GenerateOnClose;
      eventContext->Operation.Create.SecurityContext.AccessState
          .AuditPrivileges = irpSp->Parameters.Create.SecurityContext
                                 ->AccessState->AuditPrivileges;
      eventContext->Operation.Create.SecurityContext.AccessState.Flags =
          irpSp->Parameters.Create.SecurityContext->AccessState->Flags;
      eventContext->Operation.Create.SecurityContext.AccessState
          .RemainingDesiredAccess = irpSp->Parameters.Create.SecurityContext
                                        ->AccessState->RemainingDesiredAccess;
      eventContext->Operation.Create.SecurityContext.AccessState
          .PreviouslyGrantedAccess = irpSp->Parameters.Create.SecurityContext
                                         ->AccessState->PreviouslyGrantedAccess;
      eventContext->Operation.Create.SecurityContext.AccessState
          .OriginalDesiredAccess = irpSp->Parameters.Create.SecurityContext
                                       ->AccessState->OriginalDesiredAccess;

      // NOTE: AccessState offsets are relative to the start address of
      // AccessState

      if (securityDescriptorSize > 0) {
        eventContext->Operation.Create.SecurityContext.AccessState
            .SecurityDescriptorOffset =
            (ULONG)(((char *)eventContext + alignedEventContextSize) -
                    (char *)&eventContext->Operation.Create.SecurityContext
                        .AccessState);
      }

      eventContext->Operation.Create.SecurityContext.AccessState
          .UnicodeStringObjectNameOffset = (ULONG)(
          ((char *)eventContext + alignedEventContextSize +
           securityDescriptorSize) -
          (char *)&eventContext->Operation.Create.SecurityContext.AccessState);
      eventContext->Operation.Create.SecurityContext.AccessState
          .UnicodeStringObjectTypeOffset =
          eventContext->Operation.Create.SecurityContext.AccessState
              .UnicodeStringObjectNameOffset +
          alignedObjectNameSize;
    }

    // Other SecurityContext attributes
    eventContext->Operation.Create.SecurityContext.DesiredAccess =
        irpSp->Parameters.Create.SecurityContext->DesiredAccess;

    // Other Create attributes
    eventContext->Operation.Create.FileAttributes =
        irpSp->Parameters.Create.FileAttributes;
    eventContext->Operation.Create.CreateOptions =
        irpSp->Parameters.Create.Options;
    if (IS_DEVICE_READ_ONLY(DeviceObject) && // do not reorder eval as
        disposition == FILE_OPEN_IF) {
      // Substitute FILE_OPEN for FILE_OPEN_IF
      // We check on return from userland in DokanCompleteCreate below
      // and if file didn't exist, return write-protected status
      eventContext->Operation.Create.CreateOptions &=
          ((FILE_OPEN << 24) | 0x00FFFFFF);
    }
    eventContext->Operation.Create.ShareAccess =
        irpSp->Parameters.Create.ShareAccess;
    eventContext->Operation.Create.FileNameLength =
        parentDir ? fileNameLength : fcb->FileName.Length;
    eventContext->Operation.Create.FileNameOffset =
        (ULONG)(((char *)eventContext + alignedEventContextSize +
                 securityDescriptorSize + alignedObjectNameSize +
                 alignedObjectTypeNameSize) -
                (char *)&eventContext->Operation.Create);

    if (newFileSecurityDescriptor != NULL) {
      // Copy security descriptor
      RtlCopyMemory((char *)eventContext + alignedEventContextSize,
                    newFileSecurityDescriptor,
                    RtlLengthSecurityDescriptor(newFileSecurityDescriptor));
      SeDeassignSecurity(&newFileSecurityDescriptor);
      newFileSecurityDescriptor = NULL;
    }

    if (irpSp->Parameters.Create.SecurityContext->AccessState) {
      // Object name
      intermediateUnicodeStr = (PDOKAN_UNICODE_STRING_INTERMEDIATE)(
          (char *)&eventContext->Operation.Create.SecurityContext.AccessState +
          eventContext->Operation.Create.SecurityContext.AccessState
              .UnicodeStringObjectNameOffset);
      intermediateUnicodeStr->Length = irpSp->Parameters.Create.SecurityContext
                                           ->AccessState->ObjectName.Length;
      intermediateUnicodeStr->MaximumLength = (USHORT)alignedObjectNameSize;

      if (irpSp->Parameters.Create.SecurityContext->AccessState->ObjectName
              .Length > 0) {

        RtlCopyMemory(intermediateUnicodeStr->Buffer,
                      irpSp->Parameters.Create.SecurityContext->AccessState
                          ->ObjectName.Buffer,
                      irpSp->Parameters.Create.SecurityContext->AccessState
                          ->ObjectName.Length);

        *(WCHAR *)((char *)intermediateUnicodeStr->Buffer +
                   intermediateUnicodeStr->Length) = 0;
      } else {
        intermediateUnicodeStr->Buffer[0] = 0;
      }

      // Object type name
      intermediateUnicodeStr = (PDOKAN_UNICODE_STRING_INTERMEDIATE)(
          (char *)intermediateUnicodeStr + alignedObjectNameSize);
      intermediateUnicodeStr->Length = irpSp->Parameters.Create.SecurityContext
                                           ->AccessState->ObjectTypeName.Length;
      intermediateUnicodeStr->MaximumLength = (USHORT)alignedObjectTypeNameSize;

      if (irpSp->Parameters.Create.SecurityContext->AccessState->ObjectTypeName
              .Length > 0) {

        RtlCopyMemory(intermediateUnicodeStr->Buffer,
                      irpSp->Parameters.Create.SecurityContext->AccessState
                          ->ObjectTypeName.Buffer,
                      irpSp->Parameters.Create.SecurityContext->AccessState
                          ->ObjectTypeName.Length);

        *(WCHAR *)((char *)intermediateUnicodeStr->Buffer +
                   intermediateUnicodeStr->Length) = 0;
      } else {
        intermediateUnicodeStr->Buffer[0] = 0;
      }
    }

    // other context info
    eventContext->Context = 0;
    eventContext->FileFlags |= DokanFCBFlagsGet(fcb);

    // copy the file name

    RtlCopyMemory(((char *)&eventContext->Operation.Create +
                   eventContext->Operation.Create.FileNameOffset),
                  parentDir ? fileName : fcb->FileName.Buffer,
                  parentDir ? fileNameLength : fcb->FileName.Length);
    *(PWCHAR)((char *)&eventContext->Operation.Create +
              eventContext->Operation.Create.FileNameOffset +
              (parentDir ? fileNameLength : fcb->FileName.Length)) = 0;

    DokanFCBUnlock(fcb);
    fcbLocked = FALSE;
    //
    // Oplock
    //

#if (NTDDI_VERSION >= NTDDI_WIN7)
    OpenRequiringOplock = BooleanFlagOn(irpSp->Parameters.Create.Options,
                                        FILE_OPEN_REQUIRING_OPLOCK);
#else
    OpenRequiringOplock = FALSE;
#endif

    // Share access support

    if (fcb->FileCount > 1) {

      //
      //  Check if the Fcb has the proper share access.  This routine will
      //  also
      //  check for writable user sections if the user did not allow write
      //  sharing.
      //

      // DokanCheckShareAccess() will update the share access in the fcb if
      // the operation is allowed to go forward

      status = DokanCheckShareAccess(
          fileObject, fcb,
          eventContext->Operation.Create.SecurityContext.DesiredAccess,
          eventContext->Operation.Create.ShareAccess);

      if (!NT_SUCCESS(status)) {

        DDbgPrint("   DokanCheckShareAccess failed with 0x%x\n", status);

#if (NTDDI_VERSION >= NTDDI_WIN7)

        NTSTATUS OplockBreakStatus = STATUS_SUCCESS;

        //
        //  If we got a sharing violation try to break outstanding
        //  handle
        //  oplocks and retry the sharing check.  If the caller
        //  specified
        //  FILE_COMPLETE_IF_OPLOCKED we don't bother breaking the
        //  oplock;
        //  we just return the sharing violation.
        //
        if ((status == STATUS_SHARING_VIOLATION) &&
            !FlagOn(irpSp->Parameters.Create.Options,
                    FILE_COMPLETE_IF_OPLOCKED)) {

          POPLOCK oplock = DokanGetFcbOplock(fcb);
          // This may enter a wait state!
          OplockBreakStatus = FsRtlOplockBreakH(
              oplock, Irp, 0, eventContext,
              NULL /* DokanOplockComplete */, // block instead of callback
              DokanPrePostIrp);

          //
          //  If FsRtlOplockBreakH returned STATUS_PENDING,
          //  then the IRP
          //  has been posted and we need to stop working.
          //
          if (OplockBreakStatus == STATUS_PENDING) { // shouldn't happen now
            DDbgPrint("   FsRtlOplockBreakH returned STATUS_PENDING\n");
            status = STATUS_PENDING;
            __leave;
            //
            //  If FsRtlOplockBreakH returned an error
            //  we want to return that now.
            //
          } else if (!NT_SUCCESS(OplockBreakStatus)) {
            DDbgPrint("   FsRtlOplockBreakH returned 0x%x\n",
                      OplockBreakStatus);
            status = OplockBreakStatus;
            __leave;

            //
            //  Otherwise FsRtlOplockBreakH returned
            //  STATUS_SUCCESS, indicating
            //  that there is no oplock to be broken.
            //  The sharing violation is
            //  returned in that case.
            //
            //  We actually now pass null for the callback to
            //  FsRtlOplockBreakH so it will block until
            //  the oplock break is sent to holder of the oplock.
            //  This doesn't necessarily mean that the
            //  resourc was freed (file was closed) yet,
            //  but we check share access again in case it was
            //  to see if we can proceed normally or if we
            //  still have to reutrn a sharing violation.
          } else {
            status = DokanCheckShareAccess(
                fileObject, fcb,
                eventContext->Operation.Create.SecurityContext.DesiredAccess,
                eventContext->Operation.Create.ShareAccess);
            DDbgPrint("    checked share access again, status = 0x%08x\n",
                      status);
            NT_ASSERT(OplockBreakStatus == STATUS_SUCCESS);
            if (status != STATUS_SUCCESS)
              __leave;
          }

          //
          //  The initial sharing check failed with something
          //  other than sharing
          //  violation (which should never happen, but let's
          //  be future-proof),
          //  or we *did* get a sharing violation and the
          //  caller specified
          //  FILE_COMPLETE_IF_OPLOCKED.  Either way this
          //  create is over.
          //
          // We can't really handle FILE_COMPLETE_IF_OPLOCKED correctly because
          // we don't have a way of creating a useable file handle
          // without actually creating the file in user mode, which
          // won't work unless the oplock gets broken before the
          // user mode create happens.
          // It is believed that FILE_COMPLETE_IF_OPLOCKED is extremely
          // rare and may never happened during normal operation.
        } else {

          if (status == STATUS_SHARING_VIOLATION &&
              FlagOn(irpSp->Parameters.Create.Options,
                     FILE_COMPLETE_IF_OPLOCKED)) {
            DDbgPrint("failing a create with FILE_COMPLETE_IF_OPLOCKED because "
                      "of sharing violation\n");
          }

          DDbgPrint("create: sharing/oplock failed, status = 0x%08x\n", status);
          __leave;
        }

#else
        return status;
#endif
      }
      IoUpdateShareAccess(fileObject, &fcb->ShareAccess);
    } else {
      IoSetShareAccess(
          eventContext->Operation.Create.SecurityContext.DesiredAccess,
          eventContext->Operation.Create.ShareAccess, fileObject,
          &fcb->ShareAccess);
    }

    UnwindShareAccess = TRUE;

    //  Now check that we can continue based on the oplock state of the
    //  file.  If there are no open handles yet in addition to this new one
    //  we don't need to do this check; oplocks can only exist when there are
    //  handles.
    //
    //  It is important that we modified the DesiredAccess in place so
    //  that the Oplock check proceeds against any added access we had
    //  to give the caller.
    //
    // This may block and enter wait state!
    if (fcb->FileCount > 1) {
      status =
          FsRtlCheckOplock(DokanGetFcbOplock(fcb), Irp, eventContext,
                           NULL /* DokanOplockComplete */, DokanPrePostIrp);

      //
      //  if FsRtlCheckOplock returns STATUS_PENDING the IRP has been posted
      //  to service an oplock break and we need to leave now.
      //
      if (status == STATUS_PENDING) {
        DDbgPrint("   FsRtlCheckOplock returned STATUS_PENDING, fcb = "
                  "%p, fileCount = %lu\n",
                  fcb, fcb->FileCount);
        __leave;
      }
    }

    //
    //  Let's make sure that if the caller provided an oplock key that it
    //  gets stored in the file object.
    //
    // OPLOCK_FLAG_OPLOCK_KEY_CHECK_ONLY means that no blocking.

    status =
        FsRtlCheckOplockEx(DokanGetFcbOplock(fcb), Irp,
                           OPLOCK_FLAG_OPLOCK_KEY_CHECK_ONLY, NULL, NULL, NULL);

    if (!NT_SUCCESS(status)) {
      DDbgPrint("   FsRtlCheckOplockEx return status = 0x%08x\n", status);
      __leave;
    }

    if (OpenRequiringOplock) {
      DDbgPrint("   OpenRequiringOplock\n");
      //
      //  If the caller wants atomic create-with-oplock semantics, tell
      //  the oplock package.
      if ((status == STATUS_SUCCESS)) {
        status = FsRtlOplockFsctrl(DokanGetFcbOplock(fcb), Irp, fcb->FileCount);
      }

      //
      //  If we've encountered a failure we need to leave.  FsRtlCheckOplock
      //  will have returned STATUS_OPLOCK_BREAK_IN_PROGRESS if it initiated
      //  and oplock break and the caller specified FILE_COMPLETE_IF_OPLOCKED
      //  on the create call.  That's an NT_SUCCESS code, so we need to keep
      //  going.
      //
      if ((status != STATUS_SUCCESS) &&
          (status != STATUS_OPLOCK_BREAK_IN_PROGRESS)) {
        DDbgPrint("   FsRtlOplockFsctrl failed with 0x%x, fcb = %p, "
                  "fileCount = %lu\n",
                  status, fcb, fcb->FileCount);

        __leave;
      } else if (status == STATUS_OPLOCK_BREAK_IN_PROGRESS) {
        DDbgPrint("create: STATUS_OPLOCK_BREAK_IN_PROGRESS\n");
      }
      // if we fail after this point, the oplock will need to be backed out
      // if the oplock was granted (status == STATUS_SUCCESS)
      if (status == STATUS_SUCCESS) {
        BackoutOplock = TRUE;
      }
    }

    // register this IRP to waiting IPR list
    status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

    EventContextConsumed = TRUE;
  } __finally {

    DDbgPrint("  Create: FileName:%wZ, status = 0x%08x\n",
              &fileObject->FileName, status);

    // Getting here by __leave isn't always a failure,
    // so we shouldn't necessarily clean up only because
    // AbnormalTermination() returns true

#if (NTDDI_VERSION >= NTDDI_WIN7)
    //
    //  If we're not getting out with success, and if the caller wanted
    //  atomic create-with-oplock semantics make sure we back out any
    //  oplock that may have been granted.
    //
    //  Also unwind any share access that was added to the fcb

    if (!NT_SUCCESS(status)) {
      if (BackoutOplock) {
        FsRtlCheckOplockEx(DokanGetFcbOplock(fcb), Irp,
                           OPLOCK_FLAG_BACK_OUT_ATOMIC_OPLOCK, NULL, NULL,
                           NULL);
      }

      if (UnwindShareAccess) {
        IoRemoveShareAccess(fileObject, &fcb->ShareAccess);
      }
    }
#endif
    if (fcbLocked)
      DokanFCBUnlock(fcb);

    if (relatedFileName) {
      ExFreePool(relatedFileName->Buffer);
      ExFreePool(relatedFileName);
    }

    if (!NT_SUCCESS(status)) {

      // DokanRegisterPendingIrp consumes event context

      if (!EventContextConsumed && eventContext) {
        DokanFreeEventContext(eventContext);
      }
      if (ccb) {
        DokanFreeCCB(ccb);
      }
      if (fcb) {
        DokanFreeFCB(fcb);
      }
    }

    if (parentDir      // SL_OPEN_TARGET_DIRECTORY
        && fileName) { // fcb owns parentDir, not fileName
      ExFreePool(fileName);
    }

    DokanCompleteIrpRequest(Irp, status, info);

    DDbgPrint("<== DokanCreate\n");
  }

  return status;
}

NTSTATUS DokanCompleteCreate(__in PIRP_ENTRY IrpEntry,
                             __in PEVENT_INFORMATION EventInfo,
                             __in BOOLEAN Wait) {
  PIRP irp;
  PIO_STACK_LOCATION irpSp;
  NTSTATUS status;
  ULONG info;
  PDokanCCB ccb;
  PDokanFCB fcb;
  BOOLEAN FCBAcquired = FALSE;

  irp = IrpEntry->Irp;
  irpSp = IrpEntry->IrpSp;

  DDbgPrint("==> DokanCompleteCreate\n");

  ccb = IrpEntry->FileObject->FsContext2;
  ASSERT(ccb != NULL);

  fcb = ccb->Fcb;
  ASSERT(fcb != NULL);

  if (FALSE == Wait) {
    DokanFCBTryLockRW(fcb, FCBAcquired);
    if (FALSE == FCBAcquired) {
      return STATUS_PENDING;
    }
  } else {
    DokanFCBLockRW(fcb);
  }

  DDbgPrint("  FileName:%wZ\n", &fcb->FileName);

  ccb->UserContext = EventInfo->Context;
  // DDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

  status = EventInfo->Status;

  info = EventInfo->Operation.Create.Information;

  switch (info) {
  case FILE_OPENED:
    DDbgPrint("  FILE_OPENED\n");
    break;
  case FILE_CREATED:
    DDbgPrint("  FILE_CREATED\n");
    break;
  case FILE_OVERWRITTEN:
    DDbgPrint("  FILE_OVERWRITTEN\n");
    break;
  case FILE_DOES_NOT_EXIST:
    DDbgPrint("  FILE_DOES_NOT_EXIST\n");
    break;
  case FILE_EXISTS:
    DDbgPrint("  FILE_EXISTS\n");
    break;
  case FILE_SUPERSEDED:
    DDbgPrint("  FILE_SUPERSEDED\n");
    break;
  default:
    DDbgPrint("  info = %d\n", info);
    break;
  }

  DokanPrintNTStatus(status);

  // If volume is write-protected, we subbed FILE_OPEN for FILE_OPEN_IF
  // before call to userland in DokanDispatchCreate.
  // In this case, a not found error should return write protected status.
  if ((info == FILE_DOES_NOT_EXIST) &&
      (IS_DEVICE_READ_ONLY(irpSp->DeviceObject))) {

    DWORD disposition = (irpSp->Parameters.Create.Options >> 24) & 0x000000ff;
    if (disposition == FILE_OPEN_IF) {
      DDbgPrint("  Media is write protected\n");
      status = STATUS_MEDIA_WRITE_PROTECTED;
    }
  }

  if (NT_SUCCESS(status) &&
      (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE ||
       EventInfo->Operation.Create.Flags & DOKAN_FILE_DIRECTORY)) {
    if (irpSp->Parameters.Create.Options & FILE_DIRECTORY_FILE) {
      DDbgPrint("  FILE_DIRECTORY_FILE %p\n", fcb);
    } else {
      DDbgPrint("  DOKAN_FILE_DIRECTORY %p\n", fcb);
    }
    DokanFCBFlagsSetBit(fcb, DOKAN_FILE_DIRECTORY);
  }

  if (NT_SUCCESS(status)) {
    DokanCCBFlagsSetBit(ccb, DOKAN_FILE_OPENED);
  }

  // remember FILE_DELETE_ON_CLOSE so than the file can be deleted in close
  // for windows 8
  if (NT_SUCCESS(status) &&
      irpSp->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) {
    DokanFCBFlagsSetBit(fcb, DOKAN_DELETE_ON_CLOSE);
    DokanCCBFlagsSetBit(ccb, DOKAN_DELETE_ON_CLOSE);
    DDbgPrint(
        "  FILE_DELETE_ON_CLOSE is set so remember for delete in cleanup\n");
  }

  if (NT_SUCCESS(status)) {
    if (info == FILE_CREATED) {
      if (DokanFCBFlagsIsSet(fcb, DOKAN_FILE_DIRECTORY)) {
        DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_DIR_NAME,
                                FILE_ACTION_ADDED);
      } else {
        DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME,
                                FILE_ACTION_ADDED);
      }
    }
  } else {
    DDbgPrint("   IRP_MJ_CREATE failed. Free CCB:%p. Status 0x%x\n", ccb,
              status);
    DokanFreeCCB(ccb);
    IoRemoveShareAccess(irpSp->FileObject, &fcb->ShareAccess);
    DokanFCBUnlock(fcb);
    DokanFreeFCB(fcb);
    fcb = NULL;
    IrpEntry->FileObject->FsContext2 = NULL;
  }

  if (fcb)
    DokanFCBUnlock(fcb);

  DokanCompleteIrpRequest(irp, status, info);

  DDbgPrint("<== DokanCompleteCreate\n");

  return STATUS_SUCCESS;
}
