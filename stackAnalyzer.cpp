#include <pin.H>
#include <iostream>
#include <fstream>

using namespace std;

UINT64 minRSP = ~0x0;
UINT64 maxRSP = 0x0;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "stackAnalyzer.out", "specify profile file name");
static ofstream out;

void Before(CONTEXT *ctxt) {
	ADDRINT BeforeRSP = (ADDRINT)PIN_GetContextReg(ctxt, REG_RSP);
	//out << "BeforeRSP : RSP = " << hex << BeforeRSP << dec << endl;
	if (minRSP > BeforeRSP) {
		minRSP = BeforeRSP;
	}
	if (maxRSP < BeforeRSP) {
		maxRSP = BeforeRSP;
	}
}

void After(CONTEXT *ctxt) {
	ADDRINT AfterRSP = (ADDRINT)PIN_GetContextReg(ctxt, REG_RSP);
	//out << "AfterRSP : RSP = " << hex << AfterRSP << dec << endl;
	if (minRSP > AfterRSP) {
		minRSP = AfterRSP;
	}
	if (maxRSP < AfterRSP) {
		maxRSP = AfterRSP;
	}
}

void Taken(const CONTEXT *ctxt) {
	ADDRINT TakenRSP = (ADDRINT)PIN_GetContextReg(ctxt, REG_RSP);
	//out << "TakenRSP : RSP = " << hex << TakenRSP << dec << endl;
	if (minRSP > TakenRSP) {
		minRSP = TakenRSP;
	}
	if (maxRSP < TakenRSP) {
		maxRSP = TakenRSP;
	}
}

void ImageLoad(IMG img, void *v) {
	//cout << "Initial Min RSP : " << hex << minRSP << dec << endl;
	//cout << "Initial Max RSP : " << hex << maxRSP << dec << endl;
	//Comment the next 2 lines if you want for all libraries
	minRSP = ~0x0;
	maxRSP = 0x0;
	for (SEC sec = IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec)) {
		for (RTN rtn = SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)) {
			RTN_Open(rtn);
			RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)After, IARG_CONTEXT, IARG_END);
			for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {
				if (INS_IsRet(ins)) {
					INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)Before, IARG_CONTEXT, IARG_END);
					INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)Taken, IARG_CONTEXT, IARG_END);
				}
			}
			RTN_Close(rtn);
		}
	}
}

void Finalize(INT32 code, void *v) {
	cout << "Minimum Stack Pointer : " << hex << minRSP << dec << endl;
	cout << "Maximum Stack Pointer : " << hex << maxRSP << dec << endl;
	cout << "Net Stack Usage of the program : " << maxRSP - minRSP << endl;
	out << "Minimum Stack Pointer : " << hex << minRSP << dec << endl;
	out << "Maximum Stack Pointer : " << hex << maxRSP << dec << endl;
	out << "Net Stack Usage of the program : " << maxRSP - minRSP << endl;
	out.close();
}

INT32 Usage() {
	cerr << "This tool is used to keep track of the mimimum and maximum stack usage of a program. Currently, it doesn't take into account the libraries. If libraries needs to be included, comment the two lines mentioned in the code at lines 50 and 51.";
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char *argv[]) {
	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	PIN_InitSymbols();

	out.open(KnobOutputFile.Value().c_str());
	out.setf(ios::showbase);

	IMG_AddInstrumentFunction(ImageLoad, 0);
	PIN_AddFiniFunction(Finalize, 0);
	PIN_StartProgram();
	return 0;
}
