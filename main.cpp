/* Name of Program Developer: Brendan Burdick
 * Student ID: 300772602
 * Date Produced: April 1st, 2017
 * Professor: Suban Krishnamoorthy
 * Course: Operating Systems Internals (CSCI 465)
 * Homework #2
 * 
 * Purpose of Program:
 * The purpose of this program is to provide a working virtual simulator of a PC's logistical components.
 * Homework 2 is completed by this code. A working HYPO machine and MTOPS operating system was designed
 * for this project. Hardware components are simulated and used accordingly, and specific operating
 * systems concepts are expressed as well. Among the operating systems covered, some include: process
 * creation, interrupt handling, PCB and stack allocation, process state transitioning, program execution,
 * and context manipulation. This program can simulate running hardware components and operating system
 * resources in a running machine. Homework 2 required the class to create a total of 4 programs in both
 * Assembly Language and Machine Code to be run within this system. All 4 programs will run at the same time,
 * and implement process handling to ensure that all of them execute appropriately.
 * 
 * Further descriptions are provided within the variable definitions and function definitions.
 * */
 
#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
using namespace std;


/* Global variables for memory (of size 10,000), GPR (of size 8), MAR, MBR, Clock, IR, PSR, PC, SP, RQ, OSFreeList, and UserFreeList declared here.
 * Other useful constants including EndOfList, Waiting and Ready State, Stack size checking, Halt and Time Slice expiration checking, etc. are defined here as well.
 * */
const int EndOfMachLangProgram = -1; //Variable to compare to, to determine if the end of the program is reached.
const int EndOfList = -1; //End of list is -1 to mark that the end of OS or User free lists has been encountered.
const long TotalUserProgramArea = 99; //Variable used hold the TOTAL number of memory locations used for user program instructions (the 4 programs we have to write). I do not use more than 100 lines, of machine instructions.
const long SampleDynamicDumpSize = 249; //Variable used to dump the first 250 locations of specified dynamic memory (as opposed to printing out 2000+ memory locations). (Defined in HW2)
const long DefaultPriority = 128; //Default priority is in the middle. (Priority range is from 0-255)
const long ReadyState = 1; //ReadyState is a constant set to 1.
const long WaitingState = 2; //WaitingState is a constant set to 2.
const long StackFullCheck = 9; //Variable used when checking if a stack is full.
const long PCBSize = 25; //Variable to hold the PCBSize, which was given in class. This is used to help store all of a PCB's contents.
const long UserMode = 2; //Variable used to set the system mode to UserMode.
const long OSMode = 1; //Variable used to set the system mode to OSMode.
const long StackSize = 10; //Variable used hold how large the stack is.
const long TimeSlice = 200; //Set the timeslice to 200 clock ticks.
const long HaltInProgramReached = 1; //Used as a return status in the CPU function to indicate a halt was reached, and that's the reason why the CPU was left.
const long TimeSliceExpirationReached = 2; //Used as a return status in the CPU function to indicate a time slice expiration was reached, and that's the reason why the CPU was left.
long RunningPCBPtr = EndOfList; //Variable to hold the current running PCB.
long memory[10000]; //Simulated main memory.
long gpr[8]; //Simulated GPRs.
long MAR, MBR, CLOCK, IR, PSR, PC, SP; //Simulated hardware components.
long RQ = EndOfList; //Ready Queue is set to empty list. (no 'const' data type because we want to change this value later)
long WQ = EndOfList; //Waiting Queue is set to empty list. (no 'const' data type because we want to change this value later)
long OSFreeList = EndOfList;	//OS Free memory list is set to empty list. (no 'const' data type because we want to change this value later)
long UserFreeList = EndOfList; //User free memory list is set to empty list. (no 'const' data type because we want to change this value later)
long ProcessID = 1; //Global variable ProcessID holds the current process ID. Is incremented by 1 every time a new process is created.
bool systemShutdownStatus = false; //Global variable to hold the shutdown status of the HYPO machine.


/* Symbolic constants (memory locations regarding user program area, user dynamic area, and OS dynamic area definition and size) defined here.
 * */
const int StartAddrOfUserProgramArea = 0; //Holds the starting address of the user program area. (given in class)
const int AddressRangeCap = 2499; //Holds the end address of the user program area. (given in class)
const int StartAddrOfUserFreeList = 2500; //Holds the starting address of the user free list. (given in class)
const int EndAddrOfUserFreeList = 4499; //Holds the end address of the user free list. (given in class)
const int StartAddrOfOSFreeList = 4500; //Holds the starting address of the OS free list. (given in class)
const long MaxMemoryAddress = 9999; //Variable to hold the highest possible memory address.
const long StartSizeOfUserFreeList = 2000; //Variable set to the starting free block size of User Free List. Given in class.
const long StartSizeOfOSFreeList = 5500; //Variable set to the starting free block size of OS Free List. Given in class.


/* Symbolic constants (error and OK codes) defined here.
 * */
const int OK = 0;
const int ER_PID = -1; //ERROR - Invalid PID.
const int ER_MEM = -2; //ERROR - No memory available.
const int ER_NMB = -3; //ERROR - Not a memory block.
const int ER_ISC = -4; //ERROR - Invalid system call.
const int ER_PIDNotFound = -5; //ERROR - PID Not Found (This takes the place of error code ER_QFL, since we are not implementing msg system calls.
const int IncorrectSizeValue = -6; //ERROR - Requested memory size was less than zero.
const int RequestedMemoryTooSmall = -7; //ERROR - Requested memory size was less than zero.
const int DivideByZeroError = -8; //ERROR - Divide by zero attempted.
const int ErrorOpeningFile = -9; //ERROR - Error opening file.
const int InvalidMemoryRange = -10; //ERROR - Memory out of range.
const int NoEndOfProgIndicator = -11; //ERROR - End of program not found.
const int InvalidPCValue = -12; //ERROR - Invalid PC value.
const int InvalidMode = -13; //ERROR - Invalid mode.
const int InvalidAddressInGPR = -14; //ERROR - Address in GPR is invalid.
const int InvalidOperandGPR = -15; //ERROR - GPR value is invalid.
const int InvalidOpCode = -16; //ERROR - OpCode is invalid.
const int StackOverflow = -17; //ERROR - Stack overflow error occurred.
const int StackUnderflow = -18; //ERROR - Stack underflow error occurred.
const int UnknownProgramError = -19; //ERROR - Unknown Programming Error has occurred.


/* Symbolic constants (Possible Syscall Values) defined here.
 * */
const int Process_Create = 1; //Symbolic constant used when the 'process create' system call is called.
const int Process_Delete = 2; //Symbolic constant used when the 'process delete' system call is called.
const int Process_Inquiry = 3; //Symbolic constant used when the 'process inquiry' system call is called.
const int Mem_Alloc = 4; //Symbolic constant used when the 'memory allocation' system call is called.
const int Mem_Free = 5; //Symbolic constant used when the 'memory free' system call is called.
const int Msg_Send = 6; //Symbolic constant used when the 'msg send' system call is called.
const int Msg_Receive = 7; //Symbolic constant used when the 'msg receive' system call is called.
const int IO_GETC = 8; //Symbolic constant used when the 'io getc' system call is called.
const int IO_PUTC = 9; //Symbolic constant used when the 'io putc' system call is called.
const int Time_Get = 10; //Symbolic constant used when the 'time get' system call is called.
const int Time_Set = 11; //Symbolic constant used when the 'time set' system call is called.
 
 
/* Symbolic constants (Opcodes) defined here.
 * */
const int HALT = 0; //Symbolic constant used when the 'HALT' opcode is encountered.
const int ADD = 1; //Symbolic constant used when the 'ADD' opcode is encountered.
const int SUBTRACT = 2; //Symbolic constant used when the 'SUBTRACT' opcode is encountered.
const int MULTIPLY = 3; //Symbolic constant used when the 'MULTIPLY' opcode is encountered.
const int DIVIDE = 4; //Symbolic constant used when the 'DIVIDE' opcode is encountered.
const int MOVE = 5; //Symbolic constant used when the 'MOVE' opcode is encountered.
const int BRANCH = 6; //Symbolic constant used when the 'BRANCH' opcode is encountered.
const int BRANCHONMINUS = 7; //Symbolic constant used when the 'BRANCHONMINUS' opcode is encountered.
const int BRANCHONPLUS = 8; //Symbolic constant used when the 'BRANCHONPLUS' opcode is encountered.
const int BRANCHONZERO = 9; //Symbolic constant used when the 'BRANCHONZERO' opcode is encountered.
const int PUSH = 10; //Symbolic constant used when the 'PUSH' opcode is encountered.
const int POP = 11; //Symbolic constant used when the 'POP' opcode is encountered.
const int SYSTEMCALL = 12; //Symbolic constant used when the 'SYSTEMCALL' opcode is encountered.


/* Symbolic constants (Possible Interrupt Codes) defined here. These codes are also used for event IDs.
 * */
const int NoInterrupt = 0; //Symbolic constant used when there is no interrupt.
const int RunProgramInterrupt = 1; //Symbolic constant used when a 'run program' interrupt has occurred.
const int ShutdownSystemInterrupt = 2; //Symbolic constant used when a 'shutdown system' interrupt has occurred.
const int IO_GETCInterrupt = 3; //Symbolic constant used when an 'input operation completed' interrupt has occurred.
const int IO_PUTCInterrupt = 4; //Symbolic constant used when an 'output operation completed' interrupt has occurred.


/* Symbolic constants (PCB Index Accessors) defined here.
 * */
const long NextPointerIndex = 0; //Variable used to help access the Next PCB Pointer of a PCB.
const long PIDIndex = 1; //Variable used to help access the PID of a PCB.
const long StateIndex = 2; //Variable used to help access the State of a PCB.
const long ReasonForWaitingIndex = 3; //Variable used to help access a PCB's reason for waiting.
const long PriorityIndex = 4; //Variable used to help access the Priority of a PCB.
const long StackStartAddrIndex = 5; //Variable used to help access the Stack Starting Address of a PCB.
const long StackSizeIndex = 6; //Variable used to help access the Stack Size of a PCB.
const long GPR0Index = 11; //Variable used to help access the GPR0 value of a PCB.
const long GPR1Index = 12; //Variable used to help access the GPR1 value of a PCB.
const long GPR2Index = 13; //Variable used to help access the GPR2 value of a PCB.
const long GPR3Index = 14; //Variable used to help access the GPR3 value of a PCB.
const long GPR4Index = 15; //Variable used to help access the GPR4 value of a PCB.
const long GPR5Index = 16; //Variable used to help access the GPR5 value of a PCB.
const long GPR6Index = 17; //Variable used to help access the GPR6 value of a PCB.
const long GPR7Index = 18; //Variable used to help access the GPR7 value of a PCB.
const long SPIndex = 19; //Variable used to help access the SPIndex of a PCB.
const long PCIndex = 20; //Variable used to help access the PC of a PCB.
const long PSRIndex = 21; //Variable used to help access the PSR of a PCB.



/* Function prototype definitions located here. 
 * */
void InitializeSystem(); //Function Prototype for InitializeSystem() function.
long AbsoluteLoader(string *filename); //Function Prototype for AbsoluteLoader() function.
long CPU(); //Function Prototype for CPU() function.
long FetchOperand(long  OpMode, long  OpReg, long  *OpAddress, long  *OpValue); //Function Prototype for FetchOperand() function.
void DumpMemory(string str, long StartAddress, long size); //Function Prototype for DumpMemory() function.
long CreateProcess(string *filename, long priority); //Function Prototype for CreateProcess() function.
void InitializePCB(long PCBptr); //Function prototype for InitializePCB() function.
void PrintPCB(long PCBptr); //Function prototype for PrintPCB() function.
long PrintQueue(long Qptr); //Function prototype for PrintQueue() function.
long InsertIntoRQ(long PCBptr); //Function prototype for InsertIntoRQ() function.
long InsertIntoWQ(long PCBptr); //Function prototype for InsertIntoWQ() function.
long SelectProcessFromRQ(); //Function prototype for SelectProcessFromRQ() function.
void SaveContext(long PCBptr); //Function prototype for SaveContext() function.
void Dispatcher(long PCBptr); //Function prototype for Dispatcher() function.
void TerminateProcess(long PCBptr); //Function prototype for TerminateProcess() function.
long AllocateOSMemory (long RequestedSize); //Function prototype for AllocateOSMemory() function.
long FreeOSMemory(long ptr, long size); //Function prototype for FreeOSMemory() function.
long AllocateUserMemory(long size); //Function prototype for AllocateUserMemory() function.
long FreeUserMemory(long ptr, long size); //Function prototype for FreeUserMemory() function.
long CheckAndProcessInterrupt(); //Function prototype for CheckAndProcessInterrupt() function.
void ISRrunProgramInterrupt(); //Function prototype for ISRrunProgramInterrupt() function.
void ISRinputCompletionInterrupt(); //Function prototype for ISRinputCompletionInterrupt() function.
void ISRoutputCompletionInterrupt(); //Function prototype for ISRoutputCompletionInterrupt() function.
void ISRshutdownSystem(); //Function prototype for ISRshutdownSystem() function.
long SearchAndRemovePCBfromWQ(long pid); //Function prototype for SearchAndRemovePCBfromWQ() function.
long SystemCall(long SystemCallID); //Function prototype for SystemCall() function.
long MemAllocSystemCall(); //Function prototype for MemAllocSystemCall() function.
long MemFreeSystemCall(); //Function prototype for MemFreeSystemCall() function.
long io_getcSystemCall(); //Function prototype for io_getcSystemCall() function.
long io_putcSystemCall(); //Function prototype for io_putcSystemCall() function.


/*************************************************************
 * 
 * Function: Main
 * 
 * Task Description:
 * The main method begins with the initialization of all of the hardware components (registers, RAM, etc.) 
 * defined at the top of the program. The OSFreeList and UserFreeList values are configured here as well. A Null
 * process is started and ensured to always be running in the background, even when no other processes are in the
 * RQ and WQ.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * There are twelve possible function return values that may occur:
 * 1.) Error: Unable to open the file. Return code = ErrorOpeningFile
 * 2.) Error: Invalid memory address (out of range). Return code = InvalidMemoryRange
 * 3.) Error: Missing end of program indicator. Return code = NoEndOfProgIndicator
 * 4.) Error: Invalid PC value. Return code = InvalidPCValue
 * 5.) Error: Invalid Operand Mode. Return code = InvalidMode
 * 6.) Error: Invalid Address in GPR. Return code = InvalidAddressInGPR
 * 7.) Error: Invalid operand GPR. Return code = InvalidOperandGPR
 * 8.) Error: Invalid OpCode. Return code = InvalidOpCode
 * 9.) Error: Divide by zero error. Return code = DivideByZeroError
 * 10.) Error: Stack overflow error. Return code = Stackoverflow
 * 11.) Error: Stack underflow error. Return code = Stackunderflow
 * 12.) Success: Execution was successful. Return code = OK
 * 
 * ************************************************************/
int main ()
{
	long status; //Variable to hold the statuses of CheckandProcessInterrupt and returning CPU values.
	
	InitializeSystem(); //Call Initialize System function.
	
	// Run until shutdown.
	while(systemShutdownStatus == false)
	{
		//Check and process interrupt.
		status = CheckAndProcessInterrupt(); //Call Check and Process Interrupt function and store return status.
		if(status == ShutdownSystemInterrupt) //If the interrupt is 'shutdown system', exit main.
		{
			break;
		}
		
		//Dump Ready Queue and Waiting Queue.
		cout << "\n\n\nRQ: BEFORE CPU scheduling..."; //Dump the contents of RQ.
		PrintQueue(RQ);
		
		cout << "\nWQ: BEFORE CPU scheduling..."; //Dump the contents of RQ.//Dump the contents of RQ.
		PrintQueue(WQ);
		
		DumpMemory("\nDynamic Memory Area BEFORE CPU scheduling:", StartAddrOfUserFreeList, SampleDynamicDumpSize); //Dump the content of the user dynamic memory.
		
		//Select the next process from RQ to give the CPU to.
		RunningPCBPtr = SelectProcessFromRQ(); //Remove the PCB at the front of the RQ.

		// Perform 'restore context' using Dispatcher function.
		Dispatcher(RunningPCBPtr); //Call Dispatcher function using the Running PCB ptr as an argument.
		
		cout << "\nRQ: AFTER selecting process from RQ..."; //Dump the contents of RQ after the front PCB has been removed.
		PrintQueue(RQ);
		
		cout << "\n\nDumping the PCB contents of the RUNNING PCB..."; //Dump the contents of the running PCB.
		PrintPCB(RunningPCBPtr); //Dump Running PCB and CPU Context passing Running PCB ptr as an argument.
		
		//Execute instructions of the running process using the CPU.
		cout << "\n*CPU HAS BEGUN OPERATING*" << endl;
		status = CPU();  //Call the CPU function.
		cout << "*CPU HAS FINISHED OPERATING*" << endl;
		
		DumpMemory("\nDynamic Memory Area AFTER executing program:", StartAddrOfUserFreeList, SampleDynamicDumpSize); //Dump the content of the user dynamic memory after executing.
		
		// Check return status – reason for giving up CPU.
		if(status == TimeSliceExpirationReached) //If the reason is 'time slice expiration', then the process is still active, but ran out of time. Save the context and return it into the RQ.
		{
			cout << "\nTIME SLICE HAS EXPIRED, saving context and inserting back into RQ." << endl;
			SaveContext(RunningPCBPtr); //Save CPU Context of running process in its PCB, because the running process is losing control of the CPU.
			InsertIntoRQ(RunningPCBPtr); //Insert running process PCB into RQ.
			RunningPCBPtr = EndOfList; //Set the running PCB ptr to the end of list.
		}
		
		else if(status == HaltInProgramReached || status < 0) //If the reason is 'halt encountered' or 'error occurred', terminate the process.
		{
			cout << "\nHALT REACHED, end of program." << endl;
			TerminateProcess(RunningPCBPtr); //Terminate running Process.
			RunningPCBPtr = EndOfList; //Set the running PCB ptr to the end of list.
		}
		
		else if(status == IO_GETCInterrupt)
		{
			cout << "\nINPUT INTERRUPT DETECTED, please enter interrupt for PID: " << memory[RunningPCBPtr + PIDIndex] << endl;
			SaveContext(RunningPCBPtr); //Save CPU Context of running process in its PCB, because the running process is losing control of the CPU.
			memory[RunningPCBPtr + ReasonForWaitingIndex] = IO_GETCInterrupt; //Set reason for waiting in the running PCB to 'Input Completion Event'.
			InsertIntoWQ(RunningPCBPtr); //Insert running process into WQ.
			RunningPCBPtr = EndOfList; //Set the running PCB ptr to the end of list.
		}
		
		else if(status == IO_PUTCInterrupt)
		{
			cout << "\nOUTPUT INTERRUPT DETECTED, please enter interrupt for PID: " << memory[RunningPCBPtr + PIDIndex] << endl;
			SaveContext(RunningPCBPtr); //Save CPU Context of running process in its PCB, because the running process is losing control of the CPU.
			memory[RunningPCBPtr + ReasonForWaitingIndex] = IO_PUTCInterrupt; //Set reason for waiting in the running PCB to 'Output Completion Event'.
			InsertIntoWQ(RunningPCBPtr); //Insert running process into WQ.
			RunningPCBPtr = EndOfList; //Set the running PCB ptr to the end of list.
		}
		
		else //Unknown programming error encountered.
		{
			cout << "\nUnknown programming error encountered. returning error code -19.\n" << endl;
			return UnknownProgramError;
		}
		
		cout << "------------------------------------------------------------------------------------------------------------------------------" << endl;
	}
	
	cout << "\nSystem is shutting down. Returning code = 0. Goodbye!\n" << endl;
	return status; //Terminate Operating System.

}  // End of main function.



/*************************************************************
 * 
 * Function: InitializeSystem
 * 
 * Task Description:
 * Set all global system hardware components to 0. Set starting points for user and OS free lists.
 * Create the Null process, designed to run at all times.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void InitializeSystem()
{
	cout << "\nInitializing system, setting all hardware components to 0, setting UserFreeList starting address to 2500, and setting OSFreeList starting address to 4500." << endl;
	
	for (int i = 0; i<10000; i++) //Loop used to initialize all memory values to 0.
		memory[i]=0;

	for (int i = 0; i<8; i++) //Loop used to initialize all 8 gpr values to 0.
		gpr[i]=0;
	
	MAR=0, MBR=0, CLOCK=0, IR=0, PSR=0, PC=0, SP=0; //Other hardware components initialized to 0.
	
	UserFreeList = StartAddrOfUserFreeList; //Set UserFreeList to 2500, which is the starting address defined above.
	memory[UserFreeList + NextPointerIndex] = EndOfList; //Set the "next user free block pointer" to EndOfList.
	memory[UserFreeList+1] = StartSizeOfUserFreeList; //Set the second location in the free block to the size of the free block. Range is (2500-4499), or 2000 locations.
	
	OSFreeList = StartAddrOfOSFreeList; //Set OSFreeList to 4500, which is the starting address defined above.
	memory[OSFreeList + NextPointerIndex] = EndOfList; //Set the "next OS free block pointer" to EndOfList.
	memory[OSFreeList+1] = StartSizeOfOSFreeList; //Set the second location in the free block to the size of the free block. Range is (4500-9999), or 5500 locations.
	
	cout << "Initialization complete!\n" <<endl;

	string NullFile = "Null.txt";
	string *NullFilePtr = &NullFile;
	CreateProcess(NullFilePtr, 0); //Call CreateProcess function passing Null Executable File and priority zero as arguments.
	
}  // End of InitializeSystem function.



/********************************************************************
 * 
 * Function: AbsoluteLoader
 * 
 * Task Description:
 * Open the file containing HYPO machine user program and load the 
 * content into HYPO memory. On successful load, return the PC value in 
 * the End of Program line. On failure, display appropriate error message 
 * and return appropriate error code.
 * 
 * Input Parameters:
 * Filename -- The name of the HYPO Machine Executable File.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * There are five possible function return values that may occur:
 * 1.) Error: Unable to open the file. Return code = ErrorOpeningFile
 * 2.) Error: Invalid memory address (out of range). Return code = InvalidMemoryRange
 * 3.) Error: Missing end of program indicator. Return code = NoEndOfProgIndicator
 * 4.) Error: Invalid PC value. Return code = InvalidPCValue
 * 5.) Success: Load was successful. Return value is PC (0 to 2499).
 * 
 * *******************************************************************/
long AbsoluteLoader (string *filename) //Address of filename inputted.
{
	long memLocation, contentOfMemLocation; //Variables are defined.
	ifstream readFile(*filename);
	
	if (readFile.is_open())
	{
		cout << "Running AbsoluteLoader, attempting to load program..." << endl;
		while (!readFile.eof()) //While the file is open, load the program into HYPO memory.
		{
			readFile >> memLocation; //First value on line is the memory location.
			readFile >> contentOfMemLocation; //Second value on line is the memory content.
			
			if (memLocation >= 0 && memLocation <= AddressRangeCap) //Memory location is IN valid range.
				memory[memLocation] = contentOfMemLocation; //Loads memory accordingly.
			
			else if (memLocation == EndOfMachLangProgram) //Memory location is -1 -- reached end of program.
			{ 
				readFile.close();
				
				if (contentOfMemLocation >= 0 && contentOfMemLocation <= AddressRangeCap) //PC value is within range. Return PC value.
					{
						cout << "Program successfully loaded!" << endl;
						return contentOfMemLocation; //NOTE: This is the PC value.
					}
					
				else //PC value is incorrect.
				{
					cout << "\nERROR: Invalid PC value found. Returning error code -12.";
					return InvalidPCValue;// Return ERROR code. -- Invalid PC value.
				}
			}
			
			else if (memLocation > AddressRangeCap || memLocation < -1) //Memory is in an invalid range.
			{
				cout << "\nERROR: Invalid memory range found. Returning error code -10.";
				return InvalidMemoryRange; // Return ERROR code. -- Invalid memory address (out of range).
			}
		}
		cout << "\nERROR: No end of program found. Returning error code -11.";
		return NoEndOfProgIndicator; // Return ERROR code. -- Missing end of program indicator.
	}
	
	else
	{
		cout << "\nERROR: Could not open file. Returning error code -9.";
		return ErrorOpeningFile; //Return ERROR code. -- Unable to open the file.
	}
}  // End of AbsoluteLoader function.



/*************************************************************
 * 
 * Function: CPU
 * 
 * Task Description:
 * The CPU function takes the loaded program and executes it. It does this
 * by taking the first instruction to execute and breaking it up into its'
 * opcode and operand values. From there, depending on the opcode, it performs
 * calculations, manipulates memory, and adjusts global variables as needed.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * There are nine possible function return values that may occur:
 * 1.) Error: Invalid operand GPR. Return code = InvalidOperandGPR
 * 2.) Error: Invalid PC value. Return code = InvalidPCValue
 * 3.) Error: Invalid mode specified. Return code = InvalidMode
 * 4.) Error: Attempted to divide by zero. Return code = DivideByZeroError
 * 5.) Error: Invalid address in GPR (out of range). Return code = InvalidAddressInGPR
 * 6.) Error: Stack overflow error. Return code = Stackoverflow
 * 7.) Error: Stack underflow error. Return code = Stackunderflow
 * 8.) Error: Invalid Opcode. Return code = InvalidOpcode
 * 9.) Success: HALT Reached. Return code: HaltInProgramReached
 * 10.) Success: TimeSlice Expiration. Return code: TimeSliceExpirationReached
 * 11.) Success: IO_GETC Reached. Return code: IO_GETCInterrupt
 * 12.) Success: IO_PUTC Reached. Return code: IO_PUTCInterrupt
 * 
 * ************************************************************/
long CPU()
{
	long opCode, remainder, op1mode, op1gpr, op2mode, op2gpr, status, op1Address, op1Value, op2Address, op2Value, result;
	long TimeLeft = TimeSlice; //Set Timeleft for process to 200 ticks.
	bool haltReached = false;
	
	while(!haltReached && TimeLeft > 0)
	{
		if (PC >= 0 && PC <= AddressRangeCap)
		{
			MAR = PC++; // Set MAR to PC value and advance PC by 1 to point to next word.
			MBR = memory[MAR];
		}
		
		else //PC is not in valid range.
		{
			cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
			return InvalidPCValue; //Returns invalid PC address error.
		}
		
		IR = MBR; // Copy MBR value into instruction register IR.
		
		//Begin to break the instruction down into opCode, op1 Mode and GPR, and op2 mode and GPR.
		opCode = IR / 10000; // Gives you the the opcode.
		remainder = IR % 10000; // Gives you the rest of the instruction (last four digits for op1mode, op1gpr, op2mode, and op2gpr).
		
		op1mode = remainder / 1000; // Gives you the second digit (the op1mode).
		remainder = remainder % 1000;
		
		op1gpr = remainder / 100; // Gives you the third digit (the op1gpr).
		remainder = remainder % 100;
		
		op2mode = remainder / 10; // Gives you the fourth digit (the op2mode).
		remainder = remainder % 10;
		
		op2gpr = remainder; // Gives you the fifth digit (the op2gpr).
		
		if (op1mode < 0 || op1mode > 6 || op2mode < 0 || op2mode > 6) //Invalid operand mode.
		{
			cout << "\nERROR: Invalid mode found. Returning error code. Returning error code -13." << endl;
			return InvalidMode; //Returns invalid mode error.
		}
		
		else if (op1gpr < 0 || op1gpr > 7 || op2gpr < 0 || op2gpr > 7) //Invalid operand gpr.
		{
			cout << "\nERROR: Invalid operand GPR found. Returning error code -15." << endl;
			return InvalidOperandGPR; //Returns invalid operand GPR error.
		}
		
		switch (opCode)
		{
			case HALT:  // HALT command entered. HALT constant and 0 are interchangeable.
				haltReached = true;
				CLOCK = CLOCK + 12; //Update clock.
				TimeLeft = TimeLeft - 12; //Update time left.
				break;
				
			case ADD: // ADD command entered. ADD constant and 1 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				status = FetchOperand(op2mode, op2gpr, &op2Address, &op2Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				result = op1Value + op2Value; //Add the fetched values.
				
				if (op1mode == 1) // Op1Mode is in register mode.
					gpr[op1gpr] = result; //Store result in GPR array at op1gpr;
				
				else if (op1mode == 6) // Op1Mode is in immediate mode. Error.
				{
					cout << "\nERROR: Can not store value in Op1 when Op1 is in immediate mode, invalid mode found. Returning error code -13." << endl;
					return InvalidMode; //Returns invalid mode error.
				}
				
				else
					memory[op1Address] = result;
				
				CLOCK = CLOCK + 3; //Update clock.
				TimeLeft = TimeLeft - 3; //Update time left.
				break;
			
			case SUBTRACT: // SUBTRACT command entered. SUBTRACT constant and 2 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				status = FetchOperand(op2mode, op2gpr, &op2Address, &op2Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				result = op1Value - op2Value; //Subtract the fetched values.
				
				if (op1mode == 1) // Op1Mode is in register mode.
					gpr[op1gpr] = result; //Store result in GPR array at op1gpr;
				
				else if (op1mode == 6) // Op1Mode is in immediate mode. Error.
				{
					cout << "\nERROR: Can not store value in Op1 when Op1 is in immediate mode, invalid mode found. Returning error code -13." << endl;
					return InvalidMode; //Returns invalid mode error.
				}
				
				else
					memory[op1Address] = result;
				
				CLOCK = CLOCK + 3; //Update clock.
				TimeLeft = TimeLeft - 3; //Update time left.
				break;
			
			case MULTIPLY: // MULTIPLY command entered. MULTIPLY constant and 3 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				status = FetchOperand(op2mode, op2gpr, &op2Address, &op2Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				result = op1Value * op2Value; //Multiply the fetched values.
				
				if (op1mode == 1) // Op1Mode is in register mode.
					gpr[op1gpr] = result; //Store result in GPR array at op1gpr;
				
				else if (op1mode == 6) // Op1Mode is in immediate mode. Error.
				{
					cout << "\nERROR: Can not store value in Op1 when Op1 is in immediate mode, invalid mode found. Returning error code -13." << endl;
					return InvalidMode; //Returns invalid mode error.
				}
				
				else
					memory[op1Address] = result;
				
				CLOCK = CLOCK + 6; //Update clock.
				TimeLeft = TimeLeft - 6; //Update time left.
				break;
			
			
			case DIVIDE: // DIVIDE command entered. DIVIDE constant and 4 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				status = FetchOperand(op2mode, op2gpr, &op2Address, &op2Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				if(op2Value == 0)
				{
					cout << "\nERROR: Runtime error found- Attempted to divide by 0. Returning error code -8." << endl;
					return DivideByZeroError; // Returns divide by zero error.
				}
					
				result = op1Value / op2Value; //Divide the fetched values.
				
				if (op1mode == 1) // Op1Mode is in register mode.
					gpr[op1gpr] = result; //Store result in GPR array at op1gpr;
				
				else if (op1mode == 6) // Op1Mode is in immediate mode. Error.
				{
					cout << "\nERROR: Can not store value in Op1 when Op1 is in immediate mode, invalid mode found. Returning error code -13." << endl;
					return InvalidMode; //Returns invalid mode error.
				}
				
				else
					memory[op1Address] = result;
				
				CLOCK = CLOCK + 6; //Update clock.
				TimeLeft = TimeLeft - 6; //Update time left.
				break;
			
			
			case MOVE: // MOVE command entered. MOVE constant and 5 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				status = FetchOperand(op2mode, op2gpr, &op2Address, &op2Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
				
				result = op2Value; //Moves the fetched op2 value into result.
				
				if (op1mode == 1) // Op1Mode is in register mode.
					gpr[op1gpr] = result; //Store result in GPR array at op1gpr;
				
				else if (op1mode == 6) // Op1Mode is in immediate mode. Error.
				{
					cout << "\nERROR: Can not store value in Op1 when Op1 is in immediate mode, invalid mode found. Returning error code -13." << endl;
					return InvalidMode; //Returns invalid mode error.
				}
				
				else
					memory[op1Address] = result;
				
				CLOCK = CLOCK + 2; //Update clock.
				TimeLeft = TimeLeft - 2; //Update time left.
				break;
			
			
			case BRANCH: // BRANCH command enetered. BRANCH constant and 6 are interchangeable.
				if (PC >= 0 && PC <= AddressRangeCap)
					PC = memory[PC]; //Branches to new memory location.
		
				else //PC is not in valid range.
				{
					cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
					return InvalidPCValue; //Returns invalid PC address error.
				}
				CLOCK = CLOCK + 2; //Update clock.
				TimeLeft = TimeLeft - 2; //Update time left.
				break;
			
			
			case BRANCHONMINUS: // BRANCHONMINUS command entered. BRANCHONMINUS constant and 7 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				if (op1Value < 0) // If Op1 < 0, branch to desired PC location.
				{
					if (PC >= 0 && PC <= AddressRangeCap)
						PC = memory[PC];
					
					else //PC is not in valid range.
					{
						cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
						return InvalidPCValue; //Returns invalid PC address error.
					}
				}
				
				else
					PC++;  // Increment PC by 1, skip branch address, and go to next instruction.
				
				CLOCK = CLOCK + 4; //Update clock.
				TimeLeft = TimeLeft - 4; //Update time left.
				break;
			
			
			case BRANCHONPLUS: // BRANCHONPLUS command entered. BRANCHONPLUS constant and 8 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				if (op1Value > 0) // If Op1 > 0, branch to desired PC location.
				{
					if (PC >= 0 && PC <= AddressRangeCap)
						PC = memory[PC];
					
					else //PC is not in valid range.
					{
						cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
						return InvalidPCValue; //Returns invalid PC address error.
					}
				}
				
				else
					PC++;  // Increment PC by 1, skip branch address, and go to next instruction.
				
				CLOCK = CLOCK + 4; //Update clock.
				TimeLeft = TimeLeft - 4; //Update time left.
				break;
			
			
			case BRANCHONZERO: // BRANCHONZERO command entered. BRANCHONZERO constant and 9 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status; // Returns error code found while fetching.
					
				if (op1Value == 0) // If Op1 = 0, branch to desired PC location.
				{
					if (PC >= 0 && PC <= AddressRangeCap)
						PC = memory[PC];
					
					else //PC is not in valid range.
					{
						cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
						return InvalidPCValue; //Returns invalid PC address error.
					}
				}
				
				else
					PC++;  // Increment PC by 1, skip branch address, and go to next instruction.
				
				CLOCK = CLOCK + 4; //Update clock.
				TimeLeft = TimeLeft - 4; //Update time left.
				break;
			
			
			case PUSH: // PUSH command entered. PUSH constant and 10 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status;
					
				//Check if SP is at stack limit and if yes, return stack overflow error code.
				//If SP is at Stackstartvalue + 9, then it is full. Cannot push.
				if (SP == memory[RunningPCBPtr + StackStartAddrIndex] + StackFullCheck)
				{
					cout << "\nERROR: Stack is full, overflow encountered. Returning error code -17.";
					return StackOverflow;
				}
				
				else
				{
					SP++;
					memory[SP] = op1Value;
				}
				
				cout << "Pushing the value: " << memory[SP] << " to the RUNNING PCB's stack." << endl;
				
				CLOCK = CLOCK + 2; //Update clock.
				TimeLeft = TimeLeft - 2; //Update time left.
				break;
			
			
			case POP: // POP command entered. POP constant and 11 are interchangeable.
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status;
			
				// Check if SP is outside stack limit, and return stack underflow error code if it is.
				//If SP < Stackstartvalue, then the stack is empty. Cannot pop. 
				if (SP < memory[RunningPCBPtr + StackStartAddrIndex]) 
				{
					cout << "\nERROR: Stack is empty, underflow encountered. Returning error code -18.";
					return StackUnderflow;
				}
					
				else
				{
					cout << "Popping the value: " << memory[SP] << " from the RUNNING PCB's stack." << endl;
					op1Address = memory[SP]; //Store (pop) top value on stack at Op1Address.
					SP--; // Decrement SP by 1
				}
				
				CLOCK = CLOCK + 2; // Update clock.
				TimeLeft = TimeLeft - 2; //Update time left.
				break;
			
			
			case SYSTEMCALL: // SYSTEMCALL command enetered. SYSTEMCALL constant and 12 are interchangeable.
	
				if(PC < StartAddrOfUserProgramArea || PC > AddressRangeCap) //Check to see if PC is in User Program Area.
				{
					cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
					return InvalidPCValue; //Returns invalid PC address error.
				}
				
				status = FetchOperand(op1mode, op1gpr, &op1Address, &op1Value);
				if (status < 0)
					return status;
				
				status = SystemCall(op1Value); //Call SystemCall function to process the system call.
				
				if(status == IO_GETCInterrupt || status == IO_PUTCInterrupt)
					return status; //If IO_GETC or IO_PUTC is encountered, an interrupt has occurred. Return from CPU to process accordingly.
				
				
				CLOCK = CLOCK + 12; //Update clock.
				TimeLeft = TimeLeft - 12; //Update time left.
				break;
			
			
			default:  // If the opCode doesn't fall within these 13 cases, return an error (Invalid OpCode).
				cout << "\nERROR: Invalid OpCode found. Returning error code -16." << endl;
				return InvalidOpCode; //Returns invalid opCode error.
		}
	}
	
	if(haltReached == true)
	{
		return HaltInProgramReached; //If the reason why the CPU was given up was because of a halt in the program, then return that to main.
	}
	
	else if(TimeLeft <= 0)
	{
		return TimeSliceExpirationReached; //If the reason why the CPU was given up was because of a time slice expiration in the program, then return that to main.
	}
	
	else
	{
		return UnknownProgramError; //Any other reason why CPU is left is due to unknown programming errors.
	}
	
}  // End of CPU function.



/*************************************************************
 * 
 * Function: FetchOperand
 * 
 * Task Description:
 * Take in operand mode, operand register, operand address, and operand
 * value as arguments and set them accordingly. The OpMode determines
 * which case gets executed. For example:
 * 
 * If OpMode = 1, then we use register mode.
 * If OpMode = 2, then we use register deferred mode.
 * If OpMode = 3, then we use autoincrement mode.
 * If OpMode = 4, then we use autodecrement mode.
 * If OpMode = 5, then we use direct mode.
 * If OpMode = 6, then we use immediate mode.
 * 
 * Depending on which mode is specified, the OpAddress and OpValue
 * arguments will be adjusted.
 * 
 * Input Parameters:
 * OpMode -- Operand mode value.
 * OpReg -- Operand GPR value.
 * 
 * Output Parameters:
 * OpAddress -- Address of operand.
 * OpValue -- Operand value when mode and GPR are valid.
 * 
 * Function Return Value:
 * There are four possible function return values that may occur:
 * 1.) Error: Invalid address in GPR (out of range). Return code = InvalidAddressInGPR
 * 2.) Error: Error: Invalid PC value. Return code = InvalidPCValue
 * 3.) Error: Error: Invalid mode specified. Return code = InvalidMode
 * 4.) Success: The fetch was successful. Return code: OK
 * 
 * ************************************************************/
long FetchOperand(long  OpMode, long  OpReg, long  *OpAddress, long  *OpValue)
{
	
	switch (OpMode)
	{
		case 1: // Register mode -- Operand value is in the register (GPR).
			*OpAddress = -2; //Set address to negative address, because operand value is in GPR.
			*OpValue = gpr[OpReg]; //Set value to gpr specified.
			break;
		
		case 2:  // Register deferred mode – Operand address is in GPR & value in memory.
			*OpAddress = gpr[OpReg]; //Operand address is in specified GPR.
			if (*OpAddress <= EndAddrOfUserFreeList && *OpAddress >= StartAddrOfUserFreeList)
				*OpValue = memory[*OpAddress];
			
			else{
				cout << "\nERROR: Invalid address within specified GPR. Returning error code -14." << endl;
				return InvalidAddressInGPR;
			}
			break;
		
		case 3:  // Autoincrement mode – Operand address is in GPR & Operand value is in memory.
			*OpAddress = gpr[OpReg]; //Operand address is in specified GPR.
			if (*OpAddress <= EndAddrOfUserFreeList && *OpAddress >= StartAddrOfUserFreeList)
				*OpValue = memory[*OpAddress];
			
			else{
				cout << "\nERROR: Invalid address within specified GPR. Returning error code -14." << endl;
				return InvalidAddressInGPR;
			}
			gpr[OpReg]++; // Increment register content/value by 1.
			break;
		
		case 4:  // Autodecrement mode – Operand address is in GPR & Operand value is in memory.
			--gpr[OpReg];    // Decrement register content/value by 1.
			*OpAddress = gpr[OpReg]; //Operand address is in specified GPR.
			if (*OpAddress <= EndAddrOfUserFreeList && *OpAddress >= StartAddrOfUserFreeList)
				*OpValue = memory[*OpAddress];
			
			else{
				cout << "\nERROR: Invalid address within specified GPR. Returning error code -14." << endl;
				return InvalidAddressInGPR;
			}
			break;
		
		case 5:  // Direct mode – Operand address is in the instruction. Pointed to by PC.
			*OpAddress = memory[PC++]; // Get variable address (at current PC location), then increments PC by 1.
			if (*OpAddress <= EndAddrOfUserFreeList && *OpAddress >= StartAddrOfUserFreeList)
				*OpValue = memory[*OpAddress]; // Assign the content at OpAddress's memory location to OpValue.
				
			else{
				cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
				return InvalidPCValue; //Returns invalid PC address error.
			}
			break;
		
		case 6:  // Immediate mode – Operand value is in the instruction.
			if (PC <= AddressRangeCap && PC >= StartAddrOfUserProgramArea){
				*OpAddress = -2; //Set address to negtive address, because operand value is in the instruction.
				*OpValue = memory[PC++]; // Get operand value (at current PC location), then increment PC by 1.
			}
			
			else{
				cout << "\nERROR: Invalid PC value found. Returning error code -12." << endl;
				return InvalidPCValue; //Returns invalid PC address error.
			}
			break;
		
		default: //If the mode doesn't fall within these 6 cases, return an error (Invalid Mode).
			cout << "\nERROR: Invalid mode. Returning error code -13." << endl;
			return InvalidMode;
	}
	
	return OK;
}  // End of FetchOperand function.



/*************************************************************
 * 
 * Function: DumpMemory
 * 
 * Task Description:
 * Displays a string passed as one of the input parameters. 
 * Displays content of GPRs, SP, PC, PSR, system Clock and the 
 * content of specified memory locations in a specific format.
 * 
 * Input Parameters:
 * String -- String to be displayed at the top of the dump.
 * StartAddress -- Starting memory address of memory dump.
 * Size -- Number of locations to dump.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void DumpMemory(string str, long StartAddress, long size)
{
	cout << str << endl; //Display the string that was passed as a parameter.
	
	if (StartAddress < 0 || StartAddress > MaxMemoryAddress || size < 1 || StartAddress+size > MaxMemoryAddress) //Checks for invalid starting location, ending location, or size. Checks for valid memory dump range between 0-9999.
		cout << "There is either an invalid start address, end address, or size. Cannot continue.\n" << endl;
		
	else
	{
		cout << setw(12) << "\nGPRs: " << setw(7) << "G0" << setw(7) << "G1" << setw(7) << "G2" << setw(7) << "G3" << setw(7) << "G4" << setw(7) << "G5" << setw(7) << "G6" << setw(7) << "G7" << setw(7) << "SP" << setw(7) << "PC" << endl; //Display GPR header.
		cout << left << setfill(' ') << setw(11) << " " << setw(7) << gpr[0] << setw(7) << gpr[1] << setw(7) << gpr[2] << setw(7) << gpr[3] << setw(7) << gpr[4] << setw(7) << gpr[5] << setw(7) << gpr[6] << setw(7) << gpr[7] << setw(7) << SP << setw(7) << PC << endl; //Display GPR, SP, and PC.
		
		cout << left << setw(12) << "\nAddress: " << setw(7) << "+0" << setw(7) << "+1" << setw(7) << "+2" << setw(7) << "+3" << setw(7) << "+4" << setw(7) << "+5" << setw(7) << "+6" << setw(7) << "+7" << setw(7) << "+8" << setw(7) << "+9" << endl; //Display memory header.
		
		int addr = StartAddress; //Set starting address to dump.
		int endAddress = StartAddress+size; //Set starting address to dump.
		
		while (addr <= endAddress) //While the current address is less than the ending address, keep displaying.
		{
			cout << left << setw(11) << addr; //Print starting memory location in the row.
			
			for (int i = 0 ; i<10 ; i++) //Prints all values at the desired memory location until the end address is reached.
			{
				if (addr <= endAddress)
				{
					cout << setw(7) << memory[addr];
					addr++;
				}
					
				else
					break; //No more values to print, exit for loop.
			}
			cout << "\n";
		}
		
		cout << "Clock: " << CLOCK <<endl; //Clock value displayed.
		cout << "PSR: " << PSR <<endl; //PSR value displayed.
	}
}  // End of DumpMemory function.



/*************************************************************
 * 
 * Function: CreateProcess
 * 
 * Task Description:
 * This function takes a filename and a priority as input paramters and creates an active process for
 * the executing program. The function creates and allocates a PCB for the process, where it will initialize
 * all PCB contents to appropriate values. This function also defines stack space for the program and dumps
 * all of the active user program area memory locations as well.
 * 
 * Input Parameters:
 * filename -- The filename of the machine language file to create a process for.
 * priority -- The priority of the process.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Not enough memory available. Return code = ER_MEM
 * 2.) ERROR: Requested size too small. Return code = RequestedMemoryTooSmall
 * 3.) Error: Unable to open the file. Return code = ErrorOpeningFile
 * 4.) Error: Invalid memory address (out of range). Return code = InvalidMemoryRange
 * 5.) Error: Missing end of program indicator. Return code = NoEndOfProgIndicator
 * 6.) Error: Invalid PC value. Return code = InvalidPCValue
 * 7.) Success: The process creation was successful. Return code: OK
 * 
 * ************************************************************/
long CreateProcess(string *filename, long priority)	// or char * pointer
{
	//Allocate space for PCB.
	long PCBptr = AllocateOSMemory(PCBSize); //Return value contains either start address of PCB or error.
	if(PCBptr < 0)
	{
		return PCBptr; //Return error code pertaining to OSMemory allocation error.
	}
	
	InitializePCB(PCBptr); //Initialize the components of the allocated PCB.
	
	//Load the program.
	long value = AbsoluteLoader(filename);
	if(value < 0)
	{
		return value; //Return error code pertaining to Absolute Loader error.
	}
	
	memory[PCBptr + PCIndex] = value; //Set PC value in PCB.
	
	long ptr = AllocateUserMemory(StackSize); // Allocate stack space from user free list.
	if(ptr < 0)
	{
		FreeOSMemory(PCBptr, PCBSize); //Error has occurred, so you must free the pre-allocated OS memory before you return the error.
		return ptr; //Return error code pertaining to User Memory allocation error.
	}
	
	//Store stack information in the PCB.
	memory[PCBptr + StackStartAddrIndex] = ptr; //Set Starting Stack Address in the PCB.
	memory[PCBptr + SPIndex] = ptr - 1; //Empty stack is low address, full stack is high address. Subtract 1 because empty address is one prior to start address.
	memory[PCBptr + StackSizeIndex] = StackSize; //Set Stack Size in the PCB.
	memory[PCBptr + PriorityIndex] = priority; //Set Priority (passed as argument) in the PCB.
	
	DumpMemory("\nDumping memory addresses in User Program Area pertaining to the four machine language programs written.", StartAddrOfUserProgramArea, TotalUserProgramArea); //Dump program area.
	
	PrintPCB(PCBptr); // Print the contents of the PCB.
	InsertIntoRQ(PCBptr); // Insert PCB into Ready Queue according to the scheduling algorithm.
	
	return OK;
	
}  // End of CreateProcess function.



/*************************************************************
 * 
 * Function: InitializePCB
 * 
 * Task Description:
 * This function takes the memory address of the PCB to initialize, and sets all of it's contents to the appropriate values.
 * The values are defined and explained below. Please note that the PCB positions are based off of the MTOPS pseudocode.
 * 
 * Input Parameters:
 * PCBptr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void InitializePCB(long PCBptr)
{
	for(int PCBindex = 0; PCBindex < PCBSize; PCBindex++) //Loop initializes all 25 locations of the PCB to 0.
		memory[PCBptr + PCBindex] = 0;
	
	memory[PCBptr + NextPointerIndex] = EndOfList; //The first location in the PCB holds the next PCB pointer. Set the next PCBPtr to current EndOfList.
	memory[PCBptr + PIDIndex] = ProcessID++; //The second location in the PCB holds the PID. Set the PID to current unused ProcessID and increment ProcessID by 1.
	memory[PCBptr + StateIndex] = ReadyState; //The third location in the PCB holds the state. Set the state to ReadyState (1).
	memory[PCBptr + PriorityIndex] = DefaultPriority; //The fifth location in the PCB holds the priority. Set the priority to DefaultPriority (128).

}  // End of InitializePCB function.



/*************************************************************
 * 
 * Function: PrintPCB
 * 
 * Task Description:
 * This function takes the memory address of the PCB, and prints it's significant contents, including it's PID, state, priority,
 * GPRs, SP, and PC. Please note that the PCB positions are based off of the MTOPS pseudocode.
 * 
 * Input Parameters:
 * PCBptr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * 
 * ************************************************************/
void PrintPCB(long PCBptr)
{
	cout << "\nCONTENTS of the PCB in memory address " << PCBptr << ":" << endl;
	
	//Prints PCB address, Next Pointer Address, PID, State, Priority, PC, and SP values of the PCB.
	cout << "PCB address = " << PCBptr << ", Next PCB Ptr = " << memory[PCBptr + NextPointerIndex] << ", PID = " << memory[PCBptr + PIDIndex] << ", State = " << memory[PCBptr + StateIndex] << ", Reason for Waiting = " << memory[PCBptr + ReasonForWaitingIndex] << ", PC = " << memory[PCBptr + PCIndex] << ", SP = " << memory[PCBptr + SPIndex] << ", Priority = " << memory[PCBptr + PriorityIndex] << ", STACK INFO: Starting Stack Address " << memory[PCBptr + StackStartAddrIndex] << ", Stack Size = " << memory[PCBptr + StackSizeIndex] << endl;
	
	//Prints the GPR values of the PCB.
	cout << "GPRs:   GPR0: " << memory[PCBptr + GPR0Index] << "   GPR1: " << memory[PCBptr + GPR1Index] << "   GPR2: " << memory[PCBptr + GPR2Index] << "   GPR3: " << memory[PCBptr + GPR3Index]  << "   GPR4: " << memory[PCBptr + GPR4Index] << "   GPR5: " << memory[PCBptr + GPR5Index] << "   GPR6: " << memory[PCBptr + GPR6Index] << "   GPR7: " << memory[PCBptr + GPR7Index] << "\n" << endl;
	
}  // End of PrintPCB function.



/*************************************************************
 * 
 * Function: PrintQueue
 * 
 * Task Description:
 * This function walks through the queue from the given pointer until end of list is encountered.
 * As each PCB is encountered, it's values are printed on screen using the PrintPCB() message.
 * 
 * Input Parameters:
 * QPtr -- The starting location of the desired queue in memory.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) Success: Queue was printed. Return value = OK.
 * 
 * ************************************************************/
long PrintQueue(long Qptr)
{
	long currentPCBPtr = Qptr;

	if(currentPCBPtr == EndOfList) //If the initial address is EndOfList, then the list itself is empty.
	{
		cout << "\nSorry, this list is empty!" << endl;
		return OK;
	}

	// Walk thru the queue.
	while(currentPCBPtr != EndOfList)
	{
		PrintPCB(currentPCBPtr);
		currentPCBPtr = memory[currentPCBPtr + NextPointerIndex];
	}

	return OK;
	
}  // End of PrintQueue function.



/*************************************************************
 * 
 * Function: InsertIntoRQ
 * 
 * Task Description:
 * This function takes a PCB address and inserts it into the ready queue. After looking at its'
 * priority, it determines where the new PCB will be inserted. Allcorresponding 'next pointer 
 * index' values are adjusted accordingly.
 * 
 * 
 * Input Parameters:
 * PCBptr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) Error: Invalid Memory Range. Return code = InvalidMemoryRange
 * 2.) Success: PCB was inserted. Return value = OK
 * 
 * ************************************************************/
long InsertIntoRQ(long PCBptr)
{
	long previousPtr = EndOfList;
	long currentPtr = RQ;
	
	if(PCBptr < 0 || PCBptr > MaxMemoryAddress)
	{
		cout << "\nERROR: Invalid memory range found. Returning error code -10." << endl;
		return InvalidMemoryRange;
	}
	
	memory[PCBptr + StateIndex] = ReadyState; //Set the PCB's state to "ready."
	memory[PCBptr + NextPointerIndex] = EndOfList; //Set the PCB's Next Pointer value to EndOfList.
	
	if(RQ == EndOfList) //If RQ is equal to the value of EndOfList (-1), then RQ is empty.
	{
		RQ = PCBptr;
		return OK;
	}
	
	while (currentPtr != EndOfList)
	{
		if(memory[PCBptr + PriorityIndex] > memory[currentPtr + PriorityIndex]) //If the priority of the PCB we want to insert is higher than the priority of the current PCB...
		{
			if(previousPtr == EndOfList) //If previousPtr is EndOfList, then the priority of the PCB that we want to insert is higher than the highest priority PCB.
			{
				memory[PCBptr + NextPointerIndex] = RQ; //Change the current PCB's next pointer from EOL to the first PCB in the ready queue.
				RQ = PCBptr; //Change the RQ value to the address of the PCB that we want to insert (because it is now the head of the queue).
				return OK;
			}
			
			//If it isn't at the start of the RQ, then we're inserting this PCB into the middle of the list.
			memory[PCBptr + NextPointerIndex] = memory[previousPtr + NextPointerIndex]; //Set PCBptr's next pointer index to the previous pointer's next pointer index, because PCBptr is taking over the previous pointer's slot.
			memory[previousPtr + NextPointerIndex] = PCBptr; //Set the previous pointer's next pointer index to the PCB address, completing the insertion.
			return OK;
		}
		
		else //PCB to be inserted has lower or equal priority as the current PCB in RQ, move on to the next PCB.
		{
			previousPtr = currentPtr;
			currentPtr = memory[currentPtr + NextPointerIndex];
		}
	}
	
	//If it gets to this point in the InsertIntoRQ() function, than the PCB we want to insert has the lowest priority in the RQ. Insert the new PCB into the end of the RQ.
	memory[previousPtr + NextPointerIndex] = PCBptr; //Change the previous pointer's next pointer index from EOL to the new PCB address. Note that the new PCB has a next pointer address of EOL.
	return OK;
	
}  // End of InsertIntoRQ function.



/*************************************************************
 * 
 * Function: InsertIntoWQ
 * 
 * Task Description:
 * This function takes a PCB address and inserts it into the front of the waiting queue. All
 * corresponding 'next pointer index' values are adjusted accordingly.
 * 
 * Input Parameters:
 * PCBptr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) Error: Invalid Memory Range. Return code = InvalidMemoryRange
 * 2.) Success: PCB was inserted. Return value = OK
 * 
 * ************************************************************/
long InsertIntoWQ(long PCBptr)
{
	if(PCBptr < 0 || PCBptr > MaxMemoryAddress)
	{
		cout << "\nERROR: Invalid memory range found. Returning error code -10." << endl;
		return InvalidMemoryRange;
	}
	
	memory[PCBptr + StateIndex] = WaitingState; //Set the PCB's state to "waiting."
	memory[PCBptr + NextPointerIndex] = WQ;
	WQ = PCBptr;
	
	return OK;
	
}  // End of InsertIntoWQ function.



/*************************************************************
 * 
 * Function: SelectProcessFromRQ
 * 
 * Task Description:
 * This function takes the first PCB in the ready queue and returns it. It also
 * removes the first PCB from the first location in the ready queue and moves the
 * other PCBs up one slot.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * PCBptr -- The PCB address of the first PCB in the ready queue.
 * 
 * ************************************************************/
long SelectProcessFromRQ()
{
	long PCBptr = RQ; //Set PCBptr to the first PCB in the ready queue.
	
	if(RQ != EndOfList) //If the ready queue is not empty...
	{
		RQ = memory[RQ + NextPointerIndex]; //Set RQ equal to the next 'PCB pointer value'.
	}
	
	memory[PCBptr + NextPointerIndex] = EndOfList;
	return PCBptr;
	
}  // End of SelectProcessFromRQ function.



/*************************************************************
 * 
 * Function: SaveContext
 * 
 * Task Description:
 * This function is to save all of the current hardware values within the passed PCBPtr. 
 * The running process that is tied to this PCB is going to lose the CPU. Hence, its CPU 
 * context has to be saved in its PCB so that it can be restored when it gets the CPU at 
 * a later time.
 * 
 * Input Parameters:
 * PCBPtr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void SaveContext(long PCBptr)
{
	memory[PCBptr + GPR0Index] = gpr[0];
	memory[PCBptr + GPR1Index] = gpr[1];
	memory[PCBptr + GPR2Index] = gpr[2];
	memory[PCBptr + GPR3Index] = gpr[3];
	memory[PCBptr + GPR4Index] = gpr[4];
	memory[PCBptr + GPR5Index] = gpr[5];
	memory[PCBptr + GPR6Index] = gpr[6];
	memory[PCBptr + GPR7Index] = gpr[7];
	memory[PCBptr + SPIndex] = SP;
	memory[PCBptr + PCIndex] = PC;
	memory[PCBptr + PSRIndex] = PSR;
	
}  // End of SaveContext function.



/*************************************************************
 * 
 * Function: Dispatcher
 * 
 * Task Description:
 * This function takes the PCB that is passed and restores it's context to all of the hardware
 * components. The selected process has been given the CPU to run, hence, restore its CPU context 
 * from the PCB into the CPU registers. The operating system code that performs the restore context 
 * is called the Dispatcher, which is why this function is named 'Dispatcher' instead of restore context.
 * 
 * Input Parameters:
 * PCBPtr -- The memory location of the PCB.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void Dispatcher(long PCBptr)
{
	gpr[0] = memory[PCBptr + GPR0Index];
	gpr[1] = memory[PCBptr + GPR1Index];
	gpr[2] = memory[PCBptr + GPR2Index];
	gpr[3] = memory[PCBptr + GPR3Index];
	gpr[4] = memory[PCBptr + GPR4Index];
	gpr[5] = memory[PCBptr + GPR5Index];
	gpr[6] = memory[PCBptr + GPR6Index];
	gpr[7] = memory[PCBptr + GPR7Index];
	SP = memory[PCBptr + SPIndex];
	PC = memory[PCBptr + PCIndex];
	PSR = UserMode; // Set system mode to User mode.
	
}  // End of Dispatcher function.



/*************************************************************
 * 
 * Function: TerminateProcess
 * 
 * Task Description:
 * This function terminates a process. The function takes a PCB pointer as a parameter, and
 * frees that the stack memory and PCB memory to be used by another process.
 * 
 * Input Parameters:
 * PCBptr -- The address of the PCB to terminate
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void TerminateProcess(long PCBptr)
{
	FreeUserMemory(memory[PCBptr + StackStartAddrIndex], memory[PCBptr + StackSizeIndex]); // Return stack memory using stack start address and stack size in the given PCB.
	
	FreeOSMemory(PCBptr, PCBSize); // Return PCB memory using the PCBptr.
	
}  // End of TerminateProcess function.



/*************************************************************
 * 
 * Function: AllocateOSMemory
 * 
 * Task Description:
 * This function is used to take a block out of OS memory to be used by the user. The
 * input parameter 'RequestedSize' is the size that the OS attempts to free up. If the memory
 * allocation is successful, the memory address of the allocated block is returned. An error
 * message and code is returned otherwise.
 * 
 * Input Parameters:
 * RequestedSize -- The size needed for block allocation.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Not enough memory available. Return code = ER_MEM
 * 2.) ERROR: Requested size too small. Return code = RequestedMemoryTooSmall
 * 3.) SUCCESS: Memory allocation was successful. Return code = currentPtr (address of allocated block)
 * 
 * ************************************************************/
long AllocateOSMemory(long RequestedSize)  // return value contains address or error
{
	if(OSFreeList == EndOfList) //If OSFreeList is equal to EndOfList, then there is no memory available to allocate.
	{
		cout << "\nERROR: The OSFreeList is empty and there is no memory available to allocate. Returning error code -2." << endl;
		return ER_MEM;
	}
	
	if(RequestedSize < 0)
	{
		cout << "\nERROR: The requested memory size is too small. When requesting memory, the request size must be greater than one. Returning error code -7." << endl;
		return RequestedMemoryTooSmall;
	}
	
	if(RequestedSize == 1)
	{
		RequestedSize = 2;  //Minimum allocated memory size allowed is 2 locations.
	}
	
	long currentPtr = OSFreeList; //Set the current pointer to the first free OS block.
	long previousPtr = EndOfList; //Set the previous pointer to EndOfList.
	
	while(currentPtr != EndOfList) //Check each block in the link list until block with requested memory size is found.
	{
		if(memory[currentPtr+1] == RequestedSize) //If the size of the memory block is equal to the requested size, then we have found a block with requested size.
		{
			if(currentPtr == OSFreeList) //First block is the exact size requested.
			{
				OSFreeList = memory[currentPtr]; //Take the 'next OS block pointer' and set it equal to OSFreeList (which is the new start of the OSFreeList).
				memory[currentPtr] = EndOfList; //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr; //Return the starting point of the block with the requested size.
			}
			else //The block found has exactly the size that is requested, but it is not the first block.
			{
				memory[previousPtr] = memory[currentPtr]; //Set the pointer of the previous block to the pointer of the current block, effectively taking the desired block out of the OSFreeList.
				memory[currentPtr] = EndOfList; //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr; //Return the starting point of the block with the requested size.
			}
		}
		
		else if(memory[currentPtr + 1] > RequestedSize) //If the size of the memory block is greater than the size requested, then we can still use this block. Adjust all values to cut out what is needed.
		{
			if(currentPtr == OSFreeList) //First block size is the larger than the requested size.
			{
				memory[currentPtr + RequestedSize] = memory[currentPtr]; //Move next block pointer up a total of 'requestedSize' spaces.
				memory[currentPtr + RequestedSize + 1] = memory[currentPtr + 1] - RequestedSize; //Set the size of this next block to be the size that it was, minus the requestedSize (which was taken out).
				OSFreeList = currentPtr + RequestedSize;  //Set the beginning of the list to the adjusted block value.
				memory[currentPtr] = EndOfList;  //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr;	//Return the starting point of the block with the requested size.
			}
			else //Block size is the larger than the requested size and is not the first block.
			{
				memory[currentPtr + RequestedSize] = memory[currentPtr]; //Move next block pointer up a total of 'requestedSize' spaces.
				memory[currentPtr + RequestedSize + 1] = memory[currentPtr + 1] - RequestedSize; //Set the size of this next block to be the size that it was, minus the requestedSize (which was taken out).
				memory[previousPtr] = currentPtr + RequestedSize;  //Set the previous pointer's next block to the newly designed location.
				memory[currentPtr] = EndOfList;  //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr;	//Return the starting point of the block with the requested size.
			}
		}
		
		else //The current block is too small, move onto the next block.
		{
			previousPtr = currentPtr; //Adjust previous pointer value.
			currentPtr = memory[currentPtr]; //Adjust current pointer value.
		}
	}
	
	//If it makes it to this point, then there is not enough OS memory in a block available.
	cout << "\nERROR: After traversing all OS free blocks, none of them was large enough for the requested size. Returning error code -2." << endl;
	return ER_MEM;
	
}  // End of AllocateOSMemory function.



/*************************************************************
 * 
 * Function: FreeOSMemory
 * 
 * Task Description:
 * This function takes a location in memory and a memory size as parameters and
 * attempts to free them back into the OSFreeList.
 * 
 * Input Parameters:
 * ptr -- Pointer of the block to free memory for.
 * size -- Size of the block.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Memory address that is attempting to be freed is out of range. Return code = ER_NMB
 * 2.) ERROR: Requested size to free is too small. Return code = RequestedMemoryTooSmall
 * 3.) ERROR: Memory requested to be freed is outside OS Dynamic Memory Range. Return code = InvalidMemoryRange
 * 4.) SUCCESS: Memory was successfully freed. Return code = OK
 * 
 * ************************************************************/
long FreeOSMemory(long ptr, long size)
{
	if(ptr < StartAddrOfOSFreeList || ptr > MaxMemoryAddress) //If the pointer given is out of range...
	{
		cout << "\nERROR: The memory address that you're trying to free is outside of the OSFreeList area. Invalid memory address. Returning error code -3." << endl;
		return ER_NMB;
	}
	
	if(size == 1)
	{
		size = 2; //Minimum allocated memory size allowed is 2 locations.
	}
	else if(size < 1) //Size to OS memory to free is too small, return error.
	{
		cout << "\nERROR: The requested memory size is too small. When freeing memory, the request size must be greater than one. Returning error code -7." << endl;
		return RequestedMemoryTooSmall;
	}
	else if((ptr+size) > MaxMemoryAddress) //Trying to free elements in memory that pass its' limit, return error.
	{
		cout << "\nERROR: The memory address that you're trying to free is outside of the OSFreeList area. Invalid memory address. Returning error code -10." << endl;
		return InvalidMemoryRange;
	}
	
	memory[ptr] = OSFreeList; //Set the pointer of the released free block to point to the 'front' of the OSFreeList (the newly released block is taking it's place at the front).
	memory[ptr + 1] = size; //Set the size of this block in OSFreeList to the size given.
	OSFreeList = ptr; //Set the pointer given to be the new front of the OSFreeList.
	return OK;

}  // End of FreeOSMemory function.



/*************************************************************
 * 
 * Function: AllocateUserMemory
 * 
 * Task Description:
 * This function is used to take a block out of User memory to be used by the user. The
 * input parameter 'RequestedSize' is the size that is attempted to be freed up. If the memory
 * allocation is successful, the memory address of the allocated block is returned. An error
 * message and code is returned otherwise.
 * 
 * Input Parameters:
 * RequestedSize -- The size needed for block allocation.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Not enough memory available. Return code = ER_MEM
 * 2.) ERROR: Requested size too small. Return code = RequestedMemoryTooSmall
 * 3.) SUCCESS: Memory allocation was successful. Return code = currentPtr (address of allocated block)
 * 
 * ************************************************************/
long AllocateUserMemory(long RequestedSize)  // return value contains address or error code
{
	if(UserFreeList == EndOfList) //If UserFreeList is equal to EndOfList, then there is no memory available to allocate.
	{
		cout << "\nERROR: The UserFreeList is empty and there is no memory available to allocate. Returning error code -2." << endl;
		return ER_MEM;
	}
	
	if(RequestedSize < 0)
	{
		cout << "\nERROR: The requested memory size is too small. When requesting memory, the request size must be greater than one. Returning error code -7." << endl;
		return RequestedMemoryTooSmall;
	}
	
	if(RequestedSize == 1)
	{
		RequestedSize = 2;  //Minimum allocated memory size allowed is 2 locations.
	}
	
	long currentPtr = UserFreeList; //Set the current pointer to the first free User block.
	long previousPtr = EndOfList; //Set the previous pointer to EndOfList.
	
	while(currentPtr != EndOfList) //Check each block in the link list until block with requested memory size is found.
	{
		if(memory[currentPtr+1] == RequestedSize) //If the size of the memory block is equal to the requested size, then we have found a block with requested size.
		{
			if(currentPtr == UserFreeList) //First block is the exact size requested.
			{
				UserFreeList = memory[currentPtr]; //Take the 'next User block pointer' and set it equal to UserFreeList (which is the new start of the UserFreeList).
				memory[currentPtr] = EndOfList; //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr; //Return the starting point of the block with the requested size.
			}
			else //The block found has exactly the size that is requested, but it is not the first block.
			{
				memory[previousPtr] = memory[currentPtr]; //Set the pointer of the previous block to the pointer of the current block, effectively taking the desired block out of the UserFreeList.
				memory[currentPtr] = EndOfList; //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr; //Return the starting point of the block with the requested size.
			}
		}
		
		else if(memory[currentPtr + 1] > RequestedSize) //If the size of the memory block is greater than the size requested, then we can still use this block. Adjust all values to cut out what is needed.
		{
			if(currentPtr == UserFreeList) //First block size is the larger than the requested size.
			{
				memory[currentPtr + RequestedSize] = memory[currentPtr]; //Move next block pointer up a total of 'requestedSize' spaces.
				memory[currentPtr + RequestedSize + 1] = memory[currentPtr + 1] - RequestedSize; //Set the size of this next block to be the size that it was, minus the requestedSize (which was taken out).
				UserFreeList = currentPtr + RequestedSize;  //Set the beginning of the list to the adjusted block value.
				memory[currentPtr] = EndOfList;  //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr;	//Return the starting point of the block with the requested size.
			}
			else //Block size is the larger than the requested size and is not the first block.
			{
				memory[currentPtr + RequestedSize] = memory[currentPtr]; //Move next block pointer up a total of 'requestedSize' spaces.
				memory[currentPtr + RequestedSize + 1] = memory[currentPtr + 1] - RequestedSize; //Set the size of this next block to be the size that it was, minus the requestedSize (which was taken out).
				memory[previousPtr] = currentPtr + RequestedSize;  //Set the previous pointer's next block to the newly designed location.
				memory[currentPtr] = EndOfList;  //Adjust the 'next pointer' field of the current pointer block to equal EndOfList, since it's being returned.
				return currentPtr;	//Return the starting point of the block with the requested size.
			}
		}
		
		else //The current block is too small, move onto the next block.
		{
			previousPtr = currentPtr; //Adjust previous pointer value.
			currentPtr = memory[currentPtr]; //Adjust current pointer value.
		}
	}
	
	//If it makes it to this point, then there is not enough OS memory in a block available.
	cout << "\nERROR: After traversing all User free blocks, none of them was large enough for the requested size. Returning error code -2." << endl;
	return ER_MEM;
	
}  // End of AllocateUserMemory function.



/*************************************************************
 * 
 * Function: FreeUserMemory
 * 
 * Task Description:
 * This function takes a location in memory and a memory size as parameters and
 * attempts to free them back into the UserFreeList.
 * 
 * Input Parameters:
 * ptr -- Pointer of the block to free memory for.
 * size -- Size of the block.
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Memory address that is attempting to be freed is out of range. Return code = ER_NMB
 * 2.) ERROR: Requested size to free is too small. Return code = RequestedMemoryTooSmall
 * 3.) ERROR: Memory requested to be freed is outside OS Dynamic Memory Range. Return code = InvalidMemoryRange
 * 4.) SUCCESS: Memory was successfully freed. Return code = OK
 * 
 * ************************************************************/
long FreeUserMemory(long ptr, long size)  // return value contains OK or error code
{
	
	if(ptr < StartAddrOfUserFreeList || ptr > EndAddrOfUserFreeList) //If the pointer given is out of range...
	{
		cout << "\nERROR: The memory address that you're trying to free is outside of the UserFreeList area. Invalid memory address. Returning error code -3." << endl;
		return ER_NMB;
	}
	
	if(size == 1)
	{
		size = 2; //Minimum allocated memory size allowed is 2 locations.
	}
	else if(size < 1) //Size to User memory to free is too small, return error.
	{
		cout << "\nERROR: The requested memory size is too small. When freeing memory, the request size must be greater than one. Returning error code -7." << endl;
		return RequestedMemoryTooSmall;
	}
	else if((ptr+size) > MaxMemoryAddress) //Trying to free elements in memory that pass its' limit, return error.
	{
		cout << "\nERROR: The memory address that you're trying to free is outside of the UserFreeList area. Invalid memory address. Returning error code -10." << endl;
		return InvalidMemoryRange;
	}
	
	memory[ptr] = UserFreeList; //Set the pointer of the released free block to point to the 'front' of the UserFreeList (the newly released block is taking it's place at the front).
	memory[ptr + 1] = size; //Set the size of this block in UserFreeList to the size given.
	UserFreeList = ptr; //Set the pointer given to be the new front of the UserFreeList.
	return OK;

}  // End of FreeUserMemory function.



/*************************************************************
 * 
 * Function: CheckAndProcessInterrupt
 * 
 * Task Description:
 * This function is used to check if an interrupt is present. When called, the user
 * is presented with a list of possible interrupts that may have occurred, and the user
 * must choose the corresponding intterupt that matches the occasion. The proper interrupt 
 * handling protocol will be implemented when the user selects an interrupt ID.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) SUCCESS: Returns interrupt ID that was entered.
 * 
 * ************************************************************/
long CheckAndProcessInterrupt()
{
	long InterruptID;
	bool IncorrectInputFlag = false;
	
	while(!IncorrectInputFlag) //Flag used to loop if incorrect interrupt value is encountered (value that is not 0-4).
	{
		cout << "\nInterrupt detected, what type of interrupt has occurred? Types of interrupts:" <<endl;
		cout << "0 – No interrupt." <<endl;
		cout << "1 – Run program." <<endl;
		cout << "2 – Shutdown system." <<endl;
		cout << "3 – Input operation completion (io_getc)." <<endl;
		cout << "4 – Output operation completion (io_putc)." <<endl;
		cout << "Interrupt ID: ";
		cin >> InterruptID;
		cout << "Interrupt that was entered: " << InterruptID << endl;
		
		switch(InterruptID)
		{
			case NoInterrupt: //'No Interrupt' (0) was entered.
			{
				IncorrectInputFlag = true; //Adjust flag value to break loop.
				break;
			}
				
			case RunProgramInterrupt: //'Run Program' (1) was entered.
			{
				ISRrunProgramInterrupt();
				IncorrectInputFlag = true; //Adjust flag value to break loop.
				break;
			}
			
			case ShutdownSystemInterrupt: //'Shutdown System' (2) was entered.
			{
				ISRshutdownSystem();
				systemShutdownStatus = true; //Adjust global shutdown status.
				IncorrectInputFlag = true; //Adjust flag value to break loop.
				break;
			}
			
			case IO_GETCInterrupt: //'Input Operation Completion (io_getc)' (3) was entered.
			{
				ISRinputCompletionInterrupt();
				IncorrectInputFlag = true; //Adjust flag value to break loop.
				break;
			}
			
			case IO_PUTCInterrupt: //'Output Operation Completion (io_putc)' (4) was entered.
			{
				ISRoutputCompletionInterrupt();
				IncorrectInputFlag = true; //Adjust flag value to break loop.
				break;
			}
			
			default: //Invalid interrupt ID.
			{
				cout << "\nInvalid interrupt ID entered. Try again.\n" << endl;
				break;
			}
		}
	}
	
	return InterruptID; //Returns the interrupt ID.
	
}  // End of CheckAndProcessInterrupt function.



/*************************************************************
 * 
 * Function: ISRrunProgramInterrupt
 * 
 * Task Description:
 * This function works as an interrupt service request (ISR). The ISR that this function
 * is serving is a 'run program' request. The function prompts the input device (which is
 * the user in this case) for a program to run, and then that information is used to create 
 * a process for that program.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void ISRrunProgramInterrupt()
{
	string programToRun;
	cout << "\nRun program interrupt has been encountered. Please enter the name of the program to run (name of the machine language program): " << endl;
	cin >> programToRun; //Prompt and read filename.
	cout << endl; //Line spacing for neatness.
	
	string *fptr = &programToRun;
	CreateProcess(fptr, DefaultPriority); //Call Create Process passing filename and Default Priority as arguments.
	
}  // End of ISRrunProgram function.



/*************************************************************
 * 
 * Function: ISRinputCompletionInterrupt
 * 
 * Task Description:
 * This function works as an interrupt service request (ISR). The ISR that this function
 * is serving is a 'input completion' request. The function handles an IO_GETC interrupt by
 * reading a PID, searching the WQ for the PCB that matches, and stores an inputted character
 * in GPR1.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void ISRinputCompletionInterrupt()
{
	long PID;
	char inputCharacter;
	
	cout << "ISR designed for input completion has begun running, please specify the PID of the process that the input is being completed for: ";
	cin >> PID; //Read the PID of the process we're completing input for.
	
	long PCBPtr = SearchAndRemovePCBfromWQ(PID); //Search WQ to find the PCB that has the given PID, return value is stored in PCBPtr.
	if(PCBPtr > 0) //Only performs this section with a valid PCB address.
	{
		cout << "Please enter a character to store: ";
		cin >> inputCharacter; //Read one character from standard input device keyboard.
		memory[PCBPtr + GPR1Index] = (long) inputCharacter; //Store the character in the GPR in the PCB. Use typecasting from char to long data types.
		memory[PCBPtr + StateIndex] = ReadyState; //Set process state to Ready in the PCB.
		cout << "The character " << inputCharacter << " was successfully INPUTTED.";
		InsertIntoRQ(PCBPtr); //Insert PCB into ready queue.
	}
	
}  // End of ISRinputCompletionInterrupt function.



/*************************************************************
 * 
 * Function: ISRoutputCompletionInterrupt
 * 
 * Task Description:
 * This function works as an interrupt service request (ISR). The ISR that this function
 * is serving is a 'output completion' request. The function handles an IO_PUTC interrupt by
 * reading a PID, searching the WQ for the PCB that matches, and printing a character stored
 * in the PCB's GPR1 slot.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void ISRoutputCompletionInterrupt()
{
	long PID;
	char outputcharacter;
	
	cout << "ISR designed for output completion has begun running, please specify the PID of the process that the output is being completed for: ";
	cin >> PID; //Read the PID of the process we're completing input for.
	
	long PCBPtr = SearchAndRemovePCBfromWQ(PID); //Search WQ to find the PCB that has the given PID, return value is stored in PCBPtr.
	if(PCBPtr > 0) //Only performs this section with a valid PCB address.
	{
		outputcharacter = (char) memory[PCBPtr + GPR1Index]; //Typecast the ascii code for the output character back into a character value. Store in output character.
		cout << "\nOUTPUT COMPLETED, CHARACTER DISPLAYED: " << outputcharacter << endl; //Print the character that was in the PCB's GPR1 slot.
		memory[PCBPtr + StateIndex] = ReadyState; //Set process state to Ready in the PCB.
		InsertIntoRQ(PCBPtr); //Insert PCB into ready queue.
	}

}  // End of ISRonputCompletionInterrupt function.



/*************************************************************
 * 
 * Function: ISRshutdownSystem
 * 
 * Task Description:
 * This function works as an interrupt service request (ISR). The ISR that this function
 * is serving is a 'shutdown system' request. The function traverses both the ready queue
 * and the waiting queue and terminates all of the processes in both of the queues one by
 * one. After completing this request, the shutdownsystemstatus flag is adjusted to show
 * that the system has shutdown (this is done in CheckAndProcessInterrupt.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * None
 * 
 * ************************************************************/
void ISRshutdownSystem()
{
	//Terminate all processes in RQ one by one.
	long ptr = RQ; //Set ptr to first PCB pointed by RQ.
	
	while(ptr != EndOfList) //While there are still PCBs in the RQ...
	{
		RQ = memory[ptr + NextPointerIndex]; //Set RQ to equal the next PCB in RQ.
		TerminateProcess(ptr); //Terminate the current process in the list.
		ptr = RQ; //Set ptr to the next PCB in RQ.
	}
	
	//Terminate all processes in WQ one by one.	
	ptr = WQ; //Set ptr to first PCB pointed by WQ.
	
	while(ptr != EndOfList) //While there are still PCBs in the WQ...
	{
		WQ = memory[ptr + NextPointerIndex]; //Set RQ to equal the next PCB in RQ.
		TerminateProcess(ptr); //Terminate the current process in the list.
		ptr = WQ; //Set ptr to the next PCB in WQ.
	}

}  // End of ISRshutdownSystem function.



/*************************************************************
 * 
 * Function: SearchAndRemovePCBfromWQ
 * 
 * Task Description:
 * This function takes a pid as an argument and will attempt to remove the PCB
 * that is tied to that pid from the waiting queue.
 * 
 * Input Parameters:
 * pid -- Process ID of the PCB that will be removed (if successful).
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Invalid PID was specified. Return code = ER_PID
 * 2,) ERROR: PID not found in WQ. Return code = ER_PIDNotFound
 * 3.) SUCCESS: Removal was successful. Return code = PCB Address of PCB with corresponding PID
 * 
 * ************************************************************/
long SearchAndRemovePCBfromWQ (long pid)
{
	
	long currentPCBptr = WQ;
	long previousPCBptr = EndOfList;
	
	if(pid < 1) //PID cannot be zero or less than zero. Check for an incorrect PID.
	{
		cout << "\nInvalid PID used. Returning error code -1.\n" << endl;
		return ER_PID;
	}
	
	//Search WQ for a PCB that has the given pid. If a match is found, remove it from WQ and return the PCB pointer
	while(currentPCBptr != EndOfList)
	{
		if(memory[currentPCBptr + PIDIndex] == pid) //If the current pointer's PID matches the PID we're looking for, then a match is found. Remove that process from WQ.
		{
			if(previousPCBptr == EndOfList) //First PCB in WQ is a match.
			{
				WQ = memory[currentPCBptr + NextPointerIndex]; //Set the starting point of WQ to the second PCB in WQ.
			}
			else //Match is somewhere in the middle of WQ.
			{
				memory[previousPCBptr + NextPointerIndex] = memory[currentPCBptr + NextPointerIndex]; //Adjust the previous PCB's next pointer index to be the next pointer index of the PCB that's being removed from WQ.
			}
			
			memory[currentPCBptr + NextPointerIndex] = EndOfList; //Adjust the returning PCB's next pointer index to be 'EndOfList'.
			return currentPCBptr; //Return matching PCB.
		}
		
		previousPCBptr = currentPCBptr; //Move on to the next PCB if there is no match.
		currentPCBptr = memory[currentPCBptr + NextPointerIndex];
	}
	
	cout << "\nAfter traversing the waiting queue, a process with the specified ID: " << pid << " could not be found. Returning error code -5.\n" << endl;
	return ER_PIDNotFound;

}  // End of SearchAndRemovePCBfromWQ function.



/*************************************************************
 * 
 * Function: SystemCall
 * 
 * Task Description:
 * This function takes in a system call ID and then runs the corresponding command pertaining
 * to the specified system call.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) ERROR: Invalid system call ID encountered. Return code = ER_ISC
 * 2.) ERROR: Incorrect memory size specified. Return code = IncorrectSizeValue
 * 3.) ERROR: Not enough memory available. Return code = ER_MEM
 * 4.) ERROR: Requested size too small. Return code = RequestedMemoryTooSmall
 * 5.) ERROR: Memory address that is attempting to be freed is out of range. Return code = InvalidMemoryRange
 * 6.) SUCCESS: System Call was successfully processed. Return code = status
 * NOTE:) status can mean many things. status can hold memory locations, OK values, IO_GETCInterrupt, and IO_PUTCInterrupt values.
 * 
 * ************************************************************/
long SystemCall(long SystemCallID)
{
	PSR = OSMode; //Set system mode to OS mode.
	
	long status = OK; //Declare long status set to OK.
	
	switch(SystemCallID)
	{
		case Process_Create: //Create Process – User process is creating a child process. NOT implemented in this project.
		{
			cout << "\nCreate process system call not implemented." << endl; //Display 'create process system call not implemented'.
			break;
		}

		case Process_Delete: //Delete Process - NOT implemented in this project.
		{
			cout << "\nDelete process system call not implemented." << endl; //Display 'delete process system call not implemented'.
			break;
		}

		case Process_Inquiry: //Process Inquiry - NOT implemented in this project.
		{
			cout << "\nProcess inquiry system call not implemented." << endl; //Display 'process inquiry system call not implemented'.
			break;
		}

		case Mem_Alloc: //Dynamic memory allocation - Allocate user free memory system call.
		{
			status = MemAllocSystemCall();
			break;
		}

		case Mem_Free: //Free dynamically allocated user memory system call.
		{
			status = MemFreeSystemCall();
			break;
		}
		
		case Msg_Send: //Message Send - NOT implemented in this project.
		{
			cout << "\nMessage send system call not implemented." << endl; //Display 'message send system call not implemented'.
			break;
		}
			
		case Msg_Receive: //Message Receive - NOT implemented in this project.
		{
			cout << "\nMessage receive system call not implemented." << endl; //Display 'message receive system call not implemented'.
			break;
		}

		case IO_GETC: // IO_GETC - Input a single character.
		{
			status = io_getcSystemCall();
			break;
		}

		case IO_PUTC: // IO_PUTC - Output a single character.
		{
			status = io_putcSystemCall();
			break;
		}

		case Time_Get: //Get time - NOT implemented in this project.
		{
			cout << "\nGet time system call not implemented." << endl; //Display 'get time system call not implemented'.
			break;
		}
			
		case Time_Set: //Set time - NOT implemented in this project.
		{
			cout << "\nSet time system call not implemented." << endl; //Display 'set time system call not implemented'.
			break;
		}

		default: // Default switch value set to 'invalid system call ID'.
		{
			cout << "\nThe system call ID is invalid. Returning error code -4." << endl; //Display invalid system call ID error message.
			return ER_ISC; //Return invalid system call.
		}
	}

	PSR = UserMode; // Set system mode to user mode.

	return status;

}  // End of SystemCall function.



/*************************************************************
 * 
 * Function: MemAllocSystemCall
 * 
 * Task Description:
 * This function serves as a system call for the MTOPS operating system. Once a SYSTEMCALL
 * Opcode is found in a machine language program, the CPU calls the SystemCall() function
 * and passes the appropriate system call. In this case, a 4 would need to be passed. When
 * a 4 is passed, then this function (MemAllocSystemCall) is executed. This function uses the
 * value stored in GPR2 as a size value. This value represents how much memory we would like
 * to allocate from user memory.
 * 
 * Input Parameters:
 * GPR2 -- Required size of memory.
 * 
 * Output Parameters:
 * GPR0 -- Return code (holds status, either OK or an error code).
 * GPR1 -- Start address of the allocated memory.
 * 
 * Function Return Value:
 * 1.) ERROR: Incorrect memory size specified. Return code = IncorrectSizeValue
 * 2.) ERROR: Not enough memory available. Return code = ER_MEM
 * 3.) ERROR: Requested size too small. Return code = RequestedMemoryTooSmall
 * 4.) SUCCESS: Memory allocation was successful. Return value stored in = GPR1 (address of allocated block)
 * 
 * ************************************************************/
long MemAllocSystemCall()
{
	long size = gpr[2]; //Declare long 'size' and set it to GPR2 value. GPR[2] is GPR2.
	if (size < 1 || size > StartSizeOfUserFreeList) //Size cannot be negative or 0. Size cannot be greater than 2000.
	{
		cout << "\nThe size of memory that was requested to be freed was out of range (either too big or too small). Returning error code -6.\n" << endl;
		return IncorrectSizeValue;
	}
	
	if (size == 1)
	{
		size = 2; //Minimum allocated memory size allowed is 2 locations.
	}
	
	gpr[1] = AllocateUserMemory(size); //Allocate user memory, passing 'size' as an argument. Store the return value in GPR1.
	
	if(gpr[1] < 0)
	{
		gpr[0] = gpr[1]; //If this condition is hit, then we store the value in GPR1, which is an error message, into GPR0. GPR0 holds the status of the call.
	}
	else
	{
		gpr[0] = OK; //If this condition is hit, then there was no error. The GPR0 value will be okay, and GPR1 will have an accurate starting memory location.
	}
	
	cout << "Mem_Alloc system call encountered. The final values of GPR0, GPR1, and GPR2 are: GPR0 = " << gpr[0] << "  GPR1 = " << gpr[1] << "  GPR2 = " << gpr[2] << endl; //Display Mem_alloc system call, and parameters GPR0, GPR1, GPR2.

	return gpr[0]; //Return the status of the memory allocation.

}  // End of MemAllocaSystemCall function.



/*************************************************************
 * 
 * Function: MemFreeSystemCall
 * 
 * Task Description:
 * This function serves as a system call for the MTOPS operating system. Once a SYSTEMCALL
 * Opcode is found in a machine language program, the CPU calls the SystemCall() function
 * and passes the appropriate system call. In this case, a 5 would need to be passed. When
 * a 5 is passed, then this function (MemFreeSystemCall) is executed. This function uses the
 * value stored in GPR2 as a size value and the value stored in GPR1 as a starting address
 * value. The final status of the memory freeing is stored in GPR0.
 * 
 * Input Parameters:
 * GPR1 -- Pointer to the memory block to be released.
 * GPR2 -- Size of memory released.
 * 
 * Output Parameters:
 * GPR0 -- Return code (holds status, either OK or an error code).
 * 
 * Function Return Value:
 * 1.) ERROR: Incorrect memory size specified. Return code = IncorrectSizeValue
 * 2.) ERROR: Memory address that is attempting to be freed is out of range. Return code = ER_NMB
 * 3.) ERROR: Requested size to free is too small. Return code = RequestedMemoryTooSmall
 * 4.) SUCCESS: Memory was successfully freed. Return code = OK
 * 
 * ************************************************************/
long MemFreeSystemCall()
{
	long size = gpr[2]; //Declare long 'size' setting it to GPR2 value.
	if (size < 1 || size > StartSizeOfUserFreeList) //Size cannot be negative or 0. Size cannot be greater than 2000.
	{
		cout << "\nThe size of memory that was requested to be freed was out of range (either too big or too small). Returning error code -6.\n" << endl;
		return IncorrectSizeValue;
	}
	
	if (size == 1)
	{
		size = 2; //Minimum allocated memory size allowed is 2 locations.
	}
	
	gpr[0] = FreeUserMemory(gpr[1], size); //Free the desired user memory, passing GPR1 (which has the starting address) and 'size' as an argument. Store the return value in GPR0.
	
	cout << "\nMem_Free system call encountered. The final values of GPR0, GPR1, and GPR2 are: GPR0 = " << gpr[0] << "  GPR1 = " << gpr[1] << "  GPR2 = " << gpr[2] << endl; //Display Mem_free system call, and parameters GPR0, GPR1, GPR2.
	
	return gpr[0]; //Return the status of the memory freeing.
	
}  // End of MemAllocaSystemCall function.



/*************************************************************
 * 
 * Function: io_getcSystemCall
 * 
 * Task Description:
 * This function serves as a system call for the MTOPS operating system. Once a SYSTEMCALL
 * Opcode is found in a machine language program, the CPU calls the SystemCall() function
 * and passes the appropriate system call. In this case, an 8 would need to be passed. When
 * an 8 is passed, then this function (io_getcSystemCall) is executed.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) SUCCESS: Input operation started. Return value = IO_GETCInterrupt
 * 
 * ************************************************************/
long io_getcSystemCall()
{

	cout << "\nInput operation required, leaving CPU for input interrupt.\n" << endl;
	return IO_GETCInterrupt;//Return start of input operation event code (3).

}  // End of io_getcSystemCall function.



/*************************************************************
 * 
 * Function: io_putcSystemCall
 * 
 * Task Description:
 * This function serves as a system call for the MTOPS operating system. Once a SYSTEMCALL
 * Opcode is found in a machine language program, the CPU calls the SystemCall() function
 * and passes the appropriate system call. In this case, a 9 would need to be passed. When
 * a 9 is passed, then this function (io_putcSystemCall) is executed.
 * 
 * Input Parameters:
 * None
 * 
 * Output Parameters:
 * None
 * 
 * Function Return Value:
 * 1.) SUCCESS: Input operation started. Return value = IO_PUTCInterrupt
 * 
 * ************************************************************/
long io_putcSystemCall()
{
	
	cout << "\nOutput operation required, leaving CPU for output interrupt.\n" << endl;
	return IO_PUTCInterrupt; //Return start of output operation event code (4).

}  // End of io_putcSystemCall function.