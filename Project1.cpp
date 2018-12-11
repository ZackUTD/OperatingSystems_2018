/*
 Project 1
 This project simulates a simple operating system consisting of 'memory' and a 'CPU'.
 A file containing a list of instructions is read in as a 'program' that will be executed.
 
 Zack Oldham
 CS 4348.002
 2/24/2018
 */

#include <iostream>
#include <iomanip>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>
#include <time.h>

using namespace std;


void initMem(const char[]); //initialize memory array
int parseLine(string input, int&); //parse each line
bool readMem(int, int&, int); //read from memory
bool writeMem(int, int, int); //write to memory
int validTimer(const char*);

int MEM[2000]; //array to simulate memory. although global, ONLY accessed directly by memory process; CPU can only edit memory through commands (1-30)


int main(int argc, const char *argv[]) // argv[1] == file name   argv[2] == timer value
{


    int n = validTimer(argv[2]); //value at which timer causes interrupt
  
    if(n == -1)
    {
      cout << "Invalid Timer Value\n";
      _exit(-4);
    }
    
    int pfd1[2]; //pipes for communication between CPU and Memory
    int pfd2[2];

    srand(time(NULL)); //initializes time in the program - needed for random number generator
    
    
    if(pipe(pfd1) == -1) //open up pipes for communication between CPU and Memory
    {
        _exit(-1);
    }
    if(pipe(pfd2) == -1)
    {
        _exit(-1);
    }
    
    int pid = fork();
    
    if(pid == 0) //child process -- "CPU"      --child writes to pfd1[1], reads from pfd2[0]
    {   
      int memRequest[4] = {0,0,0,0}; //integer array that holds the memory command and parameters for it
      //memRequest format: index 0: 0/1 user or kernel mode, index 1: -1/0/1 end program/read/write, index 2: addr, index 3: value to write or value read (if necessary)
      int PC = 0; //Program Counter
      int SP = 1000; //Stack Pointer (starts at beginning of user stack) //decrement then push method
      int IR = 0; //Instruction Register
      int AC = 0; //Accumulator Register
      int X = 0; //general use register
      int Y = 0; //general use register
      int temp = 0; //temporary system register (not directly modifiable by user)
      int timer = 0; //timer - causes interrupts every n instructions

      while(IR != 50)
	{
	  //check timer and get next instruction -- if we are already processing an interrupt of any kind, we don't deal with expired timer until we get back to user space
	  if(timer == n && PC < 1000)
	    { //save all registers before proceeding with handler (to save state of user program)
	      memRequest[0] = 1;

	      temp = SP;
	      SP = 2000;
	      memRequest[1] = 1;
	      memRequest[2] = --SP;
	      memRequest[3] = temp;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = PC;
	      
	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = AC;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = X;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = Y;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      
	      PC = 1000;

	      timer = 0;
	    }
	  
	  //fetch instruction
	  memRequest[1] = 0;
	  memRequest[2] = PC;
	  write(pfd1[1], &memRequest, sizeof(memRequest));
	  read(pfd2[0], &IR, sizeof(IR));
	  
	  if(PC < 1000) //update timer only if we are in user space
	    timer++; 
	  
	  PC++; //update PC so that we are already pointing to next instruction (but we don't fetch this one until we execute current one)
	  
	  switch(IR)
	    {
	    case 1: //Load Value
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      PC++;
	      break;

	    case 2: //Load Addr
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      PC++;
	      break;

	    case 3: //LoadIndAddr
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      PC++;
	      break;

	    case 4: //LoadIdxX addr
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      memRequest[2] += X;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      PC++;
	      break;

	    case 5: //LoadIdxY addr
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      memRequest[2] += Y;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      PC++;
	      break;

	    case 6: //LoadSpX
	      memRequest[2] = SP + X;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));
	      break;

	    case 7: //Store addr
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[2], sizeof(memRequest[2]));
	      memRequest[1] = 1;
	      memRequest[3] = AC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      PC++;
	      break;

	    case 8: //Get
	      AC = rand() % 100 + 1;
	      break;

	    case 9: //Put port
	      memRequest[1] = 0;
	      memRequest[2] = PC;
	      
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &memRequest[3], sizeof(memRequest[3]));

	      if(memRequest[3] == 1)
		cout << AC;
	      else
		cout << (char)AC;

	      PC++;
	      break;

	    case 10: //AddX
	      AC += X;
	      break;

	    case 11: //AddY
	      AC += Y;
	      break;

	    case 12: //SubX
	      AC -= X;
	      break;

	    case 13: //SubY
	      AC -= Y;
	      break;

	    case 14: //CopyToX
	      X = AC;
	      break;

	    case 15: //CopyFromX
	      AC = X;
	      break;

	    case 16: //CopyToY
	      Y = AC;
	      break;

	    case 17: //CopyFromY
	      AC = Y;
	      break;

	    case 18: //CopyToSP
	      SP = AC;
	      break;

	    case 19: //CopyFromSP
	      AC = SP;
	      break;

	    case 20: //Jump Addr
	      memRequest[1] = 0;
	      memRequest[2] = PC;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &PC, sizeof(PC));
	      break;

	    case 21: //JumpIfEqual Addr
	      if(AC == 0)
		{
		  memRequest[1] = 0;
		  memRequest[2] = PC;
		  write(pfd1[1], &memRequest, sizeof(memRequest));
		  read(pfd2[0], &PC, sizeof(PC));
		}
	      else
		PC++;

	      break;

	    case 22: //JumpIfNotEqual
	      if(AC != 0)
		{
		  memRequest[1] = 0;
		  memRequest[2] = PC;
		  write(pfd1[1], &memRequest, sizeof(memRequest));
		  read(pfd2[0], &PC, sizeof(PC));
		}
	      else
		PC++;

	      break;

	    case 23: //Call Addr
	      memRequest[1] = 1;
	      memRequest[2] = --SP;
	      memRequest[3] = PC + 1;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      
	      memRequest[1] = 0;
	      memRequest[2] = PC;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &PC, sizeof(PC));

	      break;

	    case 24: //Ret
	      memRequest[1] = 0;
	      memRequest[2] = SP++;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &PC, sizeof(PC));

	      break;

	    case 25: //IncX
	      X++;
	      break;

	    case 26: //DecX
	      X--;
	      break;

	    case 27: //Push
	      memRequest[1] = 1;
	      memRequest[2] = --SP;
	      memRequest[3] = AC;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      break;

	    case 28: //Pop
	      memRequest[1] = 0;
	      memRequest[2] = SP++;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));

	      break;

	    case 29: //Int  -- will save all registers before proceeding with handler; we want to save the state of the user program
	      memRequest[0] = 1;

	      temp = SP;
	      SP = 2000;
	      memRequest[1] = 1;
	      memRequest[2] = --SP;
	      memRequest[3] = temp;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = PC;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = AC;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = X;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      memRequest[2] = --SP;
	      memRequest[3] = Y;

	      write(pfd1[1], &memRequest, sizeof(memRequest));

	      PC = 1500;
	      break;

	    case 30: //IRet
	      memRequest[1] = 0;
	      memRequest[2] = SP++;
	      
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &Y, sizeof(Y));
	     
	      memRequest[2] = SP++;
	      
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &X, sizeof(X));

	      memRequest[2] = SP++;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &AC, sizeof(AC));

	      memRequest[2] = SP++;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &PC, sizeof(PC));

	      memRequest[2] = SP;

	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      read(pfd2[0], &SP, sizeof(SP));
	      
	      memRequest[0] = 0;
	      break;

	    case 50: //End
	      cout << "\nProgram Terminated Successfully\n";
	      memRequest[1] = -1;
	      write(pfd1[1], &memRequest, sizeof(memRequest));
	      break;
	      
	    default: cout << "Invalid Command: Line " << PC - 1 << endl;
	      _exit(-3);
	    }
	  
	}
      
      
    }
    else //parent process -- Memory   --parent writes to pfd2[1], reads from pfd1[0]
    {   
	int request[4]; //holds memCom from CPU
	int valRead = 0; //value to read if reading 
	
        initMem(argv[1]); //initialize MEM array
  
	read(pfd1[0], &request, sizeof(request)); //this WILL wait for something to be written
	
	
	while(request[1] >= 0) //while we haven't been given an end program command execute current request
	  {
	    if(request[1] == 0) //CPU has made read request
	      {
		
		if(!readMem(request[2], valRead, request[0]))
		  {
		    cout << "READ ERROR: Index Out of Bounds: " << request[2] << endl;
		    _exit(-2);
		  }
		else
		  {
		    write(pfd2[1], &valRead, sizeof(valRead));
		  }
	      }
	    else //CPU has made write request
	      {
		if(!writeMem(request[2], request[3], request[0]))
		  {
		    cout << "WRITE ERROR: Index Out of Bounds: " << request[2] << endl;
		    _exit(-2);
		  }
	      }
	    read(pfd1[0], &request, sizeof(request));
	  }		
    }

    return 0;
}




void initMem(const char filename[]) //loads user program into memory
{
    ifstream inFile;
    
    inFile.open(filename); //open the input file given as command line argument that contains the user program
    
    if(!inFile.good()) //if file could not be opened, print appropriate message and end program.
    {
        cout << "That file could not be opened, please try again.\n";
        _exit(-1);
    }
    
    string curInput; //holds current line
    
    int result = 0; //holds return value from parseLine function
    int loadAddr = 0; //where we are currently loading to in memory
    int status = 0;
    
    while(inFile.good()) //while we haven't hit the end of the file
    {
        getline(inFile, curInput);
        status = parseLine(curInput, result);
        
        if(status == 0) //if parseLine returns 0 then we have been given a new loadAddress to load at
        {
            loadAddr = result;
        }
        else if(status == 1) //if parseLine returns 1 we recieved the next integer in the user program, so we put it in memory
        {
            MEM[loadAddr] = result;
            loadAddr++; //go to next load address if we put something in memory
        }
        else //otherwise it was a blank or entirely commented line so we ignore and don't increment loadAddr
        {
            continue;
        }
    }
}


int parseLine(string input, int &result) //determines contents of line
{
    int val = 1; //return value: if 0 then we are updating loadAddr, otherwise we are reading an instruction or argument
    int mark = 0; //tells us where the instruction stops and comments begin
    int begin = 0; //tells us whether we are converting from the beginning of the line or from just after the first char (in period case)
    
    if(input[0] == '.') //check for period to indicate loadAddr update
    {
        val = 0; //update return value
        mark = 1; //move mark up once to avoid re-checking the first index
        begin = 1; //move begin up once to avoid trying to convert the period to int on accident
    }
    
    while(mark < input.length()) //go through string incrementing mark for length of string or until we hit non-digit (comment)
    {
        
        if(!isdigit(input[mark]))
        {
            break;
        }
        
        mark++;
    }
    
    if(mark == 0 && begin == 0) //if either the line was blank or was all comments (first char wasn't anything good) then we don't care
    {
        return -1;
    }
    
    result = stoi(input.substr(begin, mark)); //convert identified good chunk to int and store into result (passed by ref)
    
    return val;
}



bool readMem(int location, int &value, int mode)   //read from memory
{
  if(location < 0 || location > 2000) // If location is outside of bounds of array, return error code
    {
      return 0;
    }
 
  if(mode == 0 && location > 999) //If CPU in user mode and is trying to access kernel space, return error code
    {
      return 0;
    }

  value = MEM[location]; //otherwise read command was valid, set value to corresponding data
  return 1; //return success code
}



bool writeMem(int location, int value, int mode)   //write to memory
{
  if(location < 0 || location > 1999) //If location is outside of bounds of array, return error code
    {
      return 0;
    }

  if(mode == 0 && location > 999) //If CPU is in user mode and is trying to write to kernel space, return error code
    {
      return 0;
    }

  MEM[location] = value; //otherwise write command was valid, so set memory location to given value
  return 1; //return success code
}



int validTimer(const char *input) //verifies that input for timer is a positive integer greater than one
{
  int i = 0;
  while(input[i] != '\0') //go through char* and check if anything not number
    {
      if(!isdigit(input[i]))
	{
	  return -1;
	}
      i++;
    }

  int result  = stoi(input); //if all good then convert to int

  if(result < 1) //if negative or zero we have a problem, return error code
    {
      return -1;
    }

  return result;
}







