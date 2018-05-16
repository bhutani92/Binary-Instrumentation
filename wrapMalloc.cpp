#include <iostream>
#include <fstream>
#include "pin.H"

using namespace std;

UINT64 mallocCount = 0;
UINT64 sizeAllocated = 0;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "wrapMalloc.out", "specify profile file name");

static ofstream out;

//typedef void * (*FP_MALLOC)(size_t);

void *MallocWrapper(CONTEXT *ctxt, AFUNPTR pf_malloc, size_t size, THREADID tid) {
	void *res;
	// This routine will be instrumented for all malloc calls for all threads. So, inherently it will get all malloc calls from multi-threaded applications as well.
	//out << "Thread " << tid << " entered malloc" << endl;
	PIN_CallApplicationFunction(ctxt, PIN_ThreadId(), CALLINGSTD_DEFAULT, pf_malloc, NULL, PIN_PARG(void *), &res, PIN_PARG(size_t), size, PIN_PARG_END());
	mallocCount++;
	sizeAllocated += size;
	return res;
}

void ImageLoad(IMG img, void *v) {
	cout << "Loading Image : " << IMG_Name(img) << endl;
	if (strstr(IMG_Name(img).c_str(), "libc.so") || strstr(IMG_Name(img).c_str(), "MSVCR80") || strstr(IMG_Name(img).c_str(), "MSVCR90")) {
		RTN mallocRTN = RTN_FindByName(img, "malloc");
		PROTO protoMalloc = PROTO_Allocate(PIN_PARG(void *), CALLINGSTD_DEFAULT, "malloc", PIN_PARG(size_t), PIN_PARG_END());
		RTN_ReplaceSignature(mallocRTN, AFUNPTR(MallocWrapper), IARG_PROTOTYPE, protoMalloc, IARG_CONTEXT, IARG_ORIG_FUNCPTR, IARG_FUNCARG_ENTRYPOINT_VALUE, 0, IARG_THREAD_ID, IARG_END);
		PROTO_Free(protoMalloc);
	}
}

void ImageUnload(IMG img, void *v) {
	cout << "Unloading Image : " << IMG_Name(img) << endl;
}

void Finalize(INT32 code, void *v) {
	cout << "Number of calls to Malloc : " << mallocCount << endl;
	cout << "Total Size Allocated by Malloc : " << sizeAllocated << endl;
	out << "Number of calls to Malloc : " << mallocCount << endl;
	out << "Total Size Allocated by Malloc : " << sizeAllocated << endl;
	out.close();
}

INT32 Usage() {
	cerr << "This tool is used to wrap all Malloc calls. It keeps track of the number of calls made to malloc and the total amount of memory allocated.";
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char *argv[]) {
	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	PIN_InitSymbols();
	
	out.open(KnobOutputFile.Value().c_str());

	IMG_AddInstrumentFunction(ImageLoad, 0);
	IMG_AddUnloadFunction(ImageUnload, 0);
	PIN_AddFiniFunction(Finalize, 0);
	PIN_StartProgram();
	return 0;
}
