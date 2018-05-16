#include <iostream>
#include <fstream>
#include "pin.H"

using namespace std;

UINT64 blkCount = 0;
PIN_LOCK lock;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "countBBL.out", "specify profile file name");

static ofstream out;

void PIN_FAST_ANALYSIS_CALL count_blocks() {
	blkCount++;
}

void countBBL(TRACE trace, void *v) {
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)) {
		BBL_InsertCall(bbl, IPOINT_ANYWHERE, (AFUNPTR)count_blocks, IARG_FAST_ANALYSIS_CALL, IARG_END);
	}
}

void Fini(INT32 code, void *v) {
	cout << "Count : " << blkCount << endl;
	out << "Count : " << blkCount << endl;
	out.close();
}

void ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, void *v) {
	PIN_GetLock(&lock, tid + 1);
	out << "Entering thread with id : " << tid << endl;
	blkCount++;
	PIN_ReleaseLock(&lock);
}

void ThreadFini(THREADID tid, const CONTEXT *ctxt, INT32 flags, void *v) {
	PIN_GetLock(&lock, tid + 1);
	out << "Exiting thread with id : " << tid << endl;
	blkCount++;
	PIN_ReleaseLock(&lock);
}

INT32 Usage() {
	cerr << "This tool is used to count the number of basic blocks executed by the program.";
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char *argv[]) {
	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	PIN_InitLock(&lock);

	out.open(KnobOutputFile.Value().c_str());

	PIN_AddThreadStartFunction(ThreadStart, 0);
	PIN_AddThreadFiniFunction(ThreadFini, 0);

	TRACE_AddInstrumentFunction(countBBL, 0);

	PIN_AddFiniFunction(Fini, 0);

	PIN_StartProgram();
	return 0;
}
