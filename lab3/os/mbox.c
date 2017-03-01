#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "synch.h"
#include "queue.h"
#include "mbox.h"

static mbox mbox_structs[MBOX_NUM_MBOXES];
static mbox_messages mbox_mess_structs[MBOX_NUM_BUFFERS];

//-------------------------------------------------------
//
// void MboxModuleInit();
//
// Initialize all mailboxes.  This process does not need
// to worry about synchronization as it is called at boot
// time.  Only initialize necessary items here: you can
// initialize others in MboxCreate.  In other words, 
// don't waste system resources like locks and semaphores
// on unused mailboxes.
//
//-------------------------------------------------------

void MboxModuleInit() {
	int i, j;
	for(i = 0; i < MBOX_NUM_MBOXES; i++) {
		mbox_structs[i].inuse = 0;
		mbox_structs[i].used = 0;
		mbox_structs[i].count = 0;
		for(j = 0; j < PROCESS_MAX_PROCS; j++) {
			mbox_structs[i].pids[j] = 0;
		}
	}

	for (i = 0; i < MBOX_NUM_BUFFERS; i++) {
		mbox_mess_structs[i].inuse = 0;
	}

}

//-------------------------------------------------------
//
// mbox_t MboxCreate();
//
// Allocate an available mailbox structure for use. 
//
// Returns the mailbox handle on success
// Returns MBOX_FAIL on error.
//
//-------------------------------------------------------
mbox_t MboxCreate() {
	mbox_t available = 0;
	while(mbox_structs[available].inuse = 1) { 
		available++;
		if(available > MBOX_NUM_MBOXES - 1) {
			return MBOX_FAIL;
		}
	}

	mbox_structs[available].inuse = 1;

	if((mbox_structs[available].l = lock_create()) == SYNC_FAIL) {
		Printf("Bad lock_create in"); Printf(argv[0]); Printf("\n");
    	Exit();
	}

	if((mbox_structs[available].moreSpace = cond_create(mbox_structs[available].l)) == SYNC_FAIL) {
		Printf("Bad cond_create in"); Printf(argv[0]); Printf("\n");
		Exit();
	}

	if((mbox_structs[available].moreData = cond_create(mbox_structs[available].l)) == SYNC_FAIL) {
		Printf("Bad cond_create in"); Printf(argv[0]); Printf("\n");
		Exit();
	}

	if(AQueueInit(mbox_structs[available].msg) != QUEUE_SUCCESS) {
		Printf("FATAL ERROR: could not initialize message queue in MboxCreate");
		Exit();
	}
  
	return available;
}

//-------------------------------------------------------
// 
// void MboxOpen(mbox_t);
//
// Open the mailbox for use by the current process.  Note
// that it is assumed that the internal lock/mutex handle 
// of the mailbox and the inuse flag will not be changed 
// during execution.  This allows us to get the a valid 
// lock handle without a need for synchronization.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxOpen(mbox_t handle) {
	if(!handle) return MBOX_FAIL;
	if(handle < 0) return MBOX_FAIL;
	if(handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if(mbox_structs[i].inuse == 0) return MBOX_FAIL;

	if(lock_acquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		Printf("Lock unable to be acquired in MboxOpen in %d \n", GetCurrentPid());
		Exit();
	}

	mbox_structs[handle].used++;
	mbox_structs[handle].pids[getpid()] = 1;

	if(lock_release(mbox_structs[handle].l) != SYNC_SUCCESS) {
		Printf("Lock unable to be released in MboxOpen in %d \n", GetCurrentPid());
		Exit();
	}
	
	return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxClose(mbox_t);
//
// Close the mailbox for use to the current process.
// If the number of processes using the given mailbox
// is zero, then disable the mailbox structure and
// return it to the set of available mboxes.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
int MboxClose(mbox_t handle) {
	if(!handle) return MBOX_FAIL;
	if(handle < 0) return MBOX_FAIL;
	if(handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if(mbox_structs[i].inuse == 0) return MBOX_FAIL;

	if(lock_acquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		Printf("Lock unable to be acquired in MboxOpen in %d \n", GetCurrentPid());
		Exit();
	}

    mbox_structs[handle].used--;
    mbox_structs[handle].pids[getpid()] = 0;

    if (mbox_structs[i].used == 0) {
    	mbox_structs[i].inuse = 0;
    }

    if(lock_release(mbox_structs[handle].l) != SYNC_SUCCESS) {
		Printf("Lock unable to be released in MboxOpen in %d \n", GetCurrentPid());
		Exit();
	}
    return MBOX_SUCCESS;
}

//-------------------------------------------------------
//
// int MboxSend(mbox_t handle,int length, void* message);
//
// Send a message (pointed to by "message") of length
// "length" bytes to the specified mailbox.  Messages of
// length 0 are allowed.  The call 
// blocks when there is not enough space in the mailbox.
// Messages cannot be longer than MBOX_MAX_MESSAGE_LENGTH.
// Note that the calling process must have opened the 
// mailbox via MboxOpen.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//-------------------------------------------------------
int MboxSend(mbox_t handle, int length, void* message) {
	if (length <= 0) return MBOX_FAIL;
	if(!handle) return MBOX_FAIL;
	if(handle < 0) return MBOX_FAIL;
	if(handle > MBOX_NUM_MBOXES) return MBOX_FAIL;
	if(mbox_structs[handle].inuse == 0) return MBOX_FAIL;
	if(length > MBOX_MAX_MESSAGE_LENGTH) return MBOX_FAIL;
	if(mbox_structs[handle].pids[GetCurrentPid()] == 0) return MBOX_FAIL;

	//Check for space in mailbox:
	//Possibly use semaphores instead!!
	if(CondHandleWait(mbox_structs[handle].moreSpace) == SYNC_FAIL) {
		Printf("Bad cond handle wait in mbox send\n");
	}

	//Lock:
	if (lock_acquire(mbox_structs[handle].l) != SYNC_SUCCESS) {
		Printf("Lock unable to be acquired in MboxSend in %d \n", GetCurrentPid());
	}

	int available = 0;
	while(mbox_mess_structs[available].inuse = 1) { 
		available++;
		if(available > MBOX_NUM_BUFFERS - 1) {
			return MBOX_FAIL;
		}
	}
	
	mbox_mess_structs[available].inuse = 1;
	mbox_mess_structs[available].message = (char *) message;
	mbox_mess_structs[available].msize = length;





}

//-------------------------------------------------------
//
// int MboxRecv(mbox_t handle, int maxlength, void* message);
//
// Receive a message from the specified mailbox.  The call 
// blocks when there is no message in the buffer.  Maxlength
// should indicate the maximum number of bytes that can be
// copied from the buffer into the address of "message".  
// An error occurs if the message is larger than maxlength.
// Note that the calling process must have opened the mailbox 
// via MboxOpen.
//   
// Returns MBOX_FAIL on failure.
// Returns number of bytes written into message on success.
//
//-------------------------------------------------------
int MboxRecv(mbox_t handle, int maxlength, void* message) {
  return MBOX_FAIL;
}

//--------------------------------------------------------------------------------
// 
// int MboxCloseAllByPid(int pid);
//
// Scans through all mailboxes and removes this pid from their "open procs" list.
// If this was the only open process, then it makes the mailbox available.  Call
// this function in ProcessFreeResources in process.c.
//
// Returns MBOX_FAIL on failure.
// Returns MBOX_SUCCESS on success.
//
//--------------------------------------------------------------------------------
int MboxCloseAllByPid(int pid) {
  return MBOX_FAIL;
}
