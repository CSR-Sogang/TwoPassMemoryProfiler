#include <iostream>
#include <fstream>
#include "pin.H"
#include <algorithm>
#include <map>

ofstream OutFile;

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static uint64_t icount = 0;
static uint64_t bbl_count = 0;

struct BBL_record {
    //size_t addr;
    size_t times;
    size_t size;
    
}; // BBL_info[MAX_BBL_SIZE];

//struct cmp {
//    bool operater()( const BBL_record& a, const BBL_record& b) const {
//        return a.times * a.size > b.times * b.size;
//    }
//};
std::map<size_t, BBL_record> BBL_map;

// This function is called before every block
VOID docount(UINT32 size, ADDRINT addr) { 
    BBL_map[(size_t) addr].size = size;
    BBL_map[(size_t) addr].times += 1;
    icount += size;
}

//int cmp(const BBL_record b1,const BBL_record b2) {
//    return b1.times *  b1.size > b2.times * b2.size;
//}


// Pin calls this function every time a new basic block is encountered
// It inserts a call to docount
VOID Trace(TRACE trace, VOID *v)
{
    // Visit every basic block  in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to docount before every bbl, passing the number of instructions
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)docount, 
                       IARG_UINT32, BBL_Size(bbl), 
                       IARG_ADDRINT, BBL_Address(bbl), IARG_END);
        bbl_count ++;
    }
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    //uint64_t cal_count = 0;
    //uint64_t counts = 0;
    OutFile.setf(ios::showbase); 
    //OutFile << "Total:" << bbl_count << endl;
    // Write to a file since cout and cerr maybe closed by the application
    //std::sort(BBL_map.begin(), BBL_map.end(), cmp);
    //printf("Here\n");
    for(std::map<size_t, BBL_record>::iterator it = BBL_map.begin(); 
        it != BBL_map.end(); it++) {
        OutFile << it->second.size * it->second.times << "\t" << it->second.size << "\t" << it->second.times << endl;
    }
    
    //OutFile << "Count " << icount << endl;
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    TRACE_AddInstrumentFunction(Trace, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
