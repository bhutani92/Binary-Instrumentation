#include <pin.H>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

class thread_data_t
{
  public:
    UINT64 oldRSP;
    UINT64 maxRSP;
    bool isStackSafe;
};

UINT64 numThreads = 0;

static  TLS_KEY tls_key;

PIN_LOCK lock;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stackPivoting.out", "specify profile file name");
static ofstream out;

thread_data_t *get_tls(THREADID tid) {
	thread_data_t *tdata = static_cast<thread_data_t *>(PIN_GetThreadData(tls_key, tid));
	return tdata;
}

void Before(CONTEXT *ctxt, int tid) {
	PIN_GetLock(&lock, tid + 1);
	thread_data_t *tdata = get_tls(tid);

	ADDRINT BeforeRSP = (ADDRINT)PIN_GetContextReg(ctxt, REG_RSP);
	//out << "BeforeRSP : RSP = " << hex << BeforeRSP << dec << endl;
	tdata->oldRSP = BeforeRSP;
	if (tdata->maxRSP < BeforeRSP) {
		tdata->maxRSP = BeforeRSP;
	}
	PIN_ReleaseLock(&lock);
}

void ThreadStart(THREADID tid, CONTEXT *ctxt, INT32 flags, void *v) {
	PIN_GetLock(&lock, tid + 1);
	numThreads++;
	PIN_ReleaseLock(&lock);

	thread_data_t *tdata = new thread_data_t;
	tdata->oldRSP = 0x0;
	tdata->maxRSP = 0x0;
	tdata->isStackSafe = true;

	PIN_SetThreadData(tls_key, tdata, tid);
}

void After(CONTEXT *ctxt, int tid) {
	PIN_GetLock(&lock, tid + 1);
	thread_data_t *tdata = get_tls(tid);

	ADDRINT AfterRSP = (ADDRINT)PIN_GetContextReg(ctxt, REG_RSP);
	//out << "AfterRSP : RSP = " << hex << AfterRSP << dec << endl;
	//If tdata->maxRSP = 0; Stack Base not initialized => not our desired section
	if (tdata->maxRSP < AfterRSP) {
		tdata->maxRSP = AfterRSP;
	}
	if (tdata->maxRSP != 0) {
		if (AfterRSP >= tdata->oldRSP && AfterRSP <= tdata->maxRSP) {
			//Legitimate Stack. No Stack Pivoting Detected
			//cout << "Good to go" << endl;
		} else {
			tdata->isStackSafe = false;

			cout << "Someone might be messing up the stack!! Caution!!" << endl;
			out << "Someone might be messing up the stack!! Caution!!" << endl;
			cout << "Thread ID: " << tid << hex << " Previous RSP: " << tdata->oldRSP << " Current RSP: " << AfterRSP << " Maximum Stack: " << tdata->maxRSP << dec << endl;
			out << "Thread ID: " << tid << hex << " Previous RSP: " << tdata->oldRSP << " Current RSP: " << AfterRSP << " Maximum Stack: " << tdata->maxRSP << dec << endl;
		}
	}
	PIN_ReleaseLock(&lock);
}

void ImageLoad(IMG img, void *v) {
	//maxRSP = 0x0; // Stack Base
	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
			RTN_Open(rtn);
			//RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)After, IARG_CONTEXT, IARG_END);
			for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
				string str = INS_Mnemonic(ins);
				string disas = INS_Disassemble(ins);
				if (str.find("MOV") != string::npos) {
					if (disas.find("rsp") != string::npos) {
						if (disas.find("rbp") == string::npos) {
							INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Before, IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
							//cout << "Disassembly : " << disas << endl;
							INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)After, IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
						}
					}
				}
			}
			RTN_Close(rtn);
		}
	}
}

void Finalize(INT32 code, void *v) {
	out << "Total number of threads : " << numThreads << endl;
	for (UINT64 i = 0; i < numThreads; i++) {
		thread_data_t *tdata = get_tls(i);
		if (tdata->isStackSafe) {
			cout << "No stack pivoting detected. This thread with ID " << i << " works as expected." << endl;
			out << "No stack pivoting detected. This thread with ID " << i << " works as expected." << endl;
		} else {
			cout << "Stack Pivoting detected for thread with ID : " << i << endl;
			out << "Stack Pivoting detected for thread with ID : " << i << endl;
		}
	}
	out.close();
}

INT32 Usage() {
	cerr << "This tool is used to detect stack pivoting.";
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char *argv[]) {
	if (PIN_Init(argc, argv)) {
		return Usage();
	}
	PIN_InitLock(&lock);
	PIN_InitSymbols();

	tls_key = PIN_CreateThreadDataKey(0);

	out.open(KnobOutputFile.Value().c_str());
	out.setf(ios::showbase);

	cout.setf(ios::showbase);

	PIN_AddThreadStartFunction(ThreadStart, 0);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	PIN_AddFiniFunction(Finalize, 0);
	PIN_StartProgram();
	return 0;
}
