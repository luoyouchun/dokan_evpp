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

NTSTATUS
DokanDispatchCleanup(__in PDEVICE_OBJECT DeviceObject, __in PIRP Irp)

/*++

Routine Description:

        This device control dispatcher handles Cleanup IRP.

Arguments:

        DeviceObject - Context for the activity.
        Irp          - The device control argument block.

Return Value:

        NTSTATUS

--*/
{
  PDokanVCB vcb;
  PIO_STACK_LOCATION irpSp;
  NTSTATUS status = STATUS_INVALID_PARAMETER;
  PFILE_OBJECT fileObject;
  PDokanCCB ccb = NULL;
  PDokanFCB fcb = NULL;
  PEVENT_CONTEXT eventContext;
  ULONG eventLength;

  __try {

    DDbgPrint("==> DokanCleanup\n");

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    fileObject = irpSp->FileObject;

    DDbgPrint("  ProcessId %lu\n", IoGetRequestorProcessId(Irp));
    DokanPrintFileName(fileObject);

    // Cleanup must be success in any case
    if (fileObject == NULL) {
      DDbgPrint("  fileObject == NULL\n");
      status = STATUS_SUCCESS;
      __leave;
    }

    vcb = DeviceObject->DeviceExtension;
    if (vcb == NULL) {
      DDbgPrint("  No device extension\n");
      status = STATUS_SUCCESS;
      __leave;
    }

    if (GetIdentifierType(vcb) != VCB ||
        !DokanCheckCCB(vcb->Dcb, fileObject->FsContext2)) {
      status = STATUS_SUCCESS;
      __leave;
    }

    ccb = fileObject->FsContext2;
    ASSERT(ccb != NULL);

    fcb = ccb->Fcb;
    ASSERT(fcb != NULL);

    FlushFcb(fcb, fileObject);

    DokanFCBLockRW(fcb);

    eventLength = sizeof(EVENT_CONTEXT) + fcb->FileName.Length;
    eventContext = AllocateEventContext(vcb->Dcb, Irp, eventLength, ccb);

    if (eventContext == NULL) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      DokanFCBUnlock(fcb);
      __leave;
    }

    fileObject->Flags |= FO_CLEANUP_COMPLETE;

    eventContext->Context = ccb->UserContext;
    eventContext->FileFlags |= DokanCCBFlagsGet(ccb);
    // DDbgPrint("   get Context %X\n", (ULONG)ccb->UserContext);

    // copy the filename to EventContext from ccb
    eventContext->Operation.Cleanup.FileNameLength = fcb->FileName.Length;
    RtlCopyMemory(eventContext->Operation.Cleanup.FileName,
                  fcb->FileName.Buffer, fcb->FileName.Length);

    // FsRtlCheckOpLock is called with non-NULL completion routine - not blocking.
    status = FsRtlCheckOplock(DokanGetFcbOplock(fcb), Irp, eventContext,
                              DokanOplockComplete, DokanPrePostIrp);
    DokanFCBUnlock(fcb);

    //
    //  if FsRtlCheckOplock returns STATUS_PENDING the IRP has been posted
    //  to service an oplock break and we need to leave now.
    //
    if (status != STATUS_SUCCESS) {
      if (status == STATUS_PENDING) {
        DDbgPrint("   FsRtlCheckOplock returned STATUS_PENDING\n");
      } else {
        DokanFreeEventContext(eventContext);
      }
      __leave;
    }

    // register this IRP to pending IRP list
    status = DokanRegisterPendingIrp(DeviceObject, Irp, eventContext, 0);

  } __finally {

    DokanCompleteIrpRequest(Irp, status, 0);

    DDbgPrint("<== DokanCleanup\n");
  }

  return status;
}

NTSTATUS DokanCompleteCleanup(__in PIRP_ENTRY IrpEntry,
                              __in PEVENT_INFORMATION EventInfo,
                              __in BOOLEAN Wait) {
  PIRP irp;
  PIO_STACK_LOCATION irpSp;
  NTSTATUS status = STATUS_SUCCESS;
  PDokanCCB ccb;
  PDokanFCB fcb;
  PDokanVCB vcb;
  PFILE_OBJECT fileObject;
  BOOLEAN FCBAcquired = FALSE;

  DDbgPrint("==> DokanCompleteCleanup\n");

  irp = IrpEntry->Irp;
  irpSp = IrpEntry->IrpSp;

  fileObject = IrpEntry->FileObject;
  ASSERT(fileObject != NULL);

  ccb = fileObject->FsContext2;
  ASSERT(ccb != NULL);

  ccb->UserContext = EventInfo->Context;
  // DDbgPrint("   set Context %X\n", (ULONG)ccb->UserContext);

  fcb = ccb->Fcb;
  ASSERT(fcb != NULL);

  vcb = fcb->Vcb;

  status = EventInfo->Status;

  if (FALSE == Wait) {
    DokanFCBTryLockRO(fcb, FCBAcquired);
    if (FALSE == FCBAcquired) {
      return STATUS_PENDING;
    }
  } else {
    DokanFCBLockRO(fcb);
  }

  if (DokanFCBFlagsIsSet(fcb, DOKAN_FILE_CHANGE_LAST_WRITE)) {
    DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_LAST_WRITE,
                            FILE_ACTION_MODIFIED);
  }

  if (DokanFCBFlagsIsSet(fcb, DOKAN_DELETE_ON_CLOSE)) {
    if (DokanFCBFlagsIsSet(fcb, DOKAN_FILE_DIRECTORY)) {
      DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_DIR_NAME,
                              FILE_ACTION_REMOVED);
    } else {
      DokanNotifyReportChange(fcb, FILE_NOTIFY_CHANGE_FILE_NAME,
                              FILE_ACTION_REMOVED);
    }
  }
  DokanFCBUnlock(fcb);
  //
  //  Unlock all outstanding file locks.
  //
  (VOID) FsRtlFastUnlockAll(&fcb->FileLock, fileObject,
                            IoGetRequestorProcess(irp), NULL);

  if (DokanFCBFlagsIsSet(fcb, DOKAN_FILE_DIRECTORY)) {
    FsRtlNotifyCleanup(vcb->NotifySync, &vcb->DirNotifyList, ccb);
  }

  IoRemoveShareAccess(irpSp->FileObject, &fcb->ShareAccess);

  DokanCompleteIrpRequest(irp, status, 0);

  DDbgPrint("<== DokanCompleteCleanup\n");

  return STATUS_SUCCESS;
}
