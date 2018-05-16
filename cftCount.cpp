#include <pin.H>
#include <iostream>
#include <fstream>

using namespace std;

UINT64 directCFT = 0;
UINT64 indirectCFT = 0;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "cftCount.out", "specify profile file name");
static ofstream out;

void Instruction(INS ins, void *v) {
	if (INS_IsDirectBranchOrCall(ins)) {
		out << "Direct Instruction : " << INS_Mnemonic(ins) << endl;
		directCFT++;
	} else if (INS_IsIndirectBranchOrCall(ins)) {
		out << "InDirect Instruction : " << INS_Mnemonic(ins) << endl;
		indirectCFT++;
	}
}

void Finalize(INT32 code, void *v) {
	cout << "Number of Direct CFT's : " << directCFT << endl;
	cout << "Number of InDirect CFT's : " << indirectCFT << endl;
	out << "Number of Direct CFT's : " << directCFT << endl;
	out << "Number of InDirect CFT's : " << indirectCFT << endl;
	out.close();
}

INT32 Usage() {
	cerr << "This tool is used to count the number of direct and indirect control flow transers executed by the program.";
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}

int main(int argc, char *argv[]) {
	if (PIN_Init(argc, argv)) {
		return Usage();
	}

	out.open(KnobOutputFile.Value().c_str());

	INS_AddInstrumentFunction(Instruction, 0);
	PIN_AddFiniFunction(Finalize, 0);
	PIN_StartProgram();
	return 0;
}
