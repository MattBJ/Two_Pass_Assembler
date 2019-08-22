//
#include <iomanip>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// for converting string to upper case
// #include <cctype>
#include <cstring>
// #include <istream>

/* useful c-string functions: 	strlen returns number of characters NOT INCLUDING NULL
								strcat concatenates two c-strings (stringvar[25] = "Paul"; strcat(stringVar, " McCartney"))
								strncat(strA,strB, number_of_char's_to_concatenate)

								getline(cin, line); //  doesn't read in '\n' char. 
									-getline(cin,line,'*'); // * is delimiter
										- Ex: input: abc 123 * def 456
												output: abc 123
								cin.ignore(n, delim);
									- ignores n characters UNTIL delimiter character is given

								string can = string, no need for strcpy (only needed for char arrays to receive copied info)
*/

using namespace std;

#define MSB_WORD		0xFF00	// MSB of a word
#define LSB_WORD    	0x00FF 	// LSB of a word
#define OFFSET_BITS		0x03FF	// LAST 10 BITS OF A WORD
#define BYTE_MASK 		0xFF

struct Line {
	int line_number, key, instr_type; // ROM will
	long long int ROM;
	int ROMsource, ROMdest;

	// take care of the instruction word:
	int BW, Ad, As;
	int64_t SOint, DOint;
	int OpCode; // single operands default to source!!
	int PCoffset; // used only in jump
	// SOint = SourceOp int
	// DOint = Dest Op int
	// Ad is 1-bit, As is 2-bit
	bool has_label, empty_rom; // initialized to false and true
	string letters, information, label, instruction;
	// information = line without comments
	string ROInstruction, Source_Op, Dest_Op;
	// everything to the right of the instructions
	
	// pointer to first memory address of a line
	int first_address; // NULL

	// memory size used to determine next line's address
	int Memory_Size; // relative to line start address aka instruction address
	//^ initialized to 0, can be incremented to find addresses of specific bytes in line
	// memory size knows how big the instruction data is in terms of bytes
};



// sorted chronologically LINE ORDER (byte order7)
struct ROM {
	int address; // address data
	int data;
	bool NONE; // don't output ROM column for corresponding line
	struct Line *Instr_Line; // NULL always unless the beginning of a block of memory
};

// can worry about sorting LAST
struct Symbol {
	int64_t address;
	string label, add_str;
};

struct ERROR {
	unsigned int err_line, prev_label_line;
	// bool duplicate_label; // will eventually be a bitfield for errors
	// =================
	// ERROR BITFIELD
	// 3			2			1			0
	// Label 		END dir 	undefined	duplicate
	// Syntax 		Missing		opcode 		label
	// =================
	unsigned int bitfield: 4; // 4-bit bitfield
};

struct eof {
	bool found;
	int line_number;
};

// ==============================================================
// 					FUNCTION 		DECLARATIONS
// ==============================================================

int64_t DecHexToInt(string data, vector<Symbol> &Symbol_Link, struct Line Cur_Line);

int64_t AsciiToInt(string data,vector<Symbol> &Symbol_Link, struct Line Cur_Line);

int64_t StrToIntS(string data, vector<Symbol> &Symbol_Link, struct Line Cur_Line);

void FillLineLinks(string filename, vector<Line> &Line_Link);

void FilterComments(vector<Line> &Line_Link);

void LabelSearch(vector<Line> &Line_Link);

string MakeUpper(string anycase, int size);

void InstructionSearch(vector<Line> &Line_Link);

void KeyFill(vector<Line> &Line_Link, vector<ERROR> &ERROR_link, struct eof &END_info);

void InstructionType(vector<Line> &Line_Link);

void OperandFill(vector<Line> &Line_Link);

void OperandStrFill(vector<Line> &Line_Link, struct eof END_info);

string IntToAddress(long long int x, int memory_size);

void FirstPass(vector<Line> &Line_Link,vector<Symbol> &Symbol_Link);

void SymbolOrder(vector<Symbol> &Symbol_Link);

void DisplayLineLink(vector<Line> &Line_Link);

void DisplaySymbolTable(vector<Symbol> &Symbol_Link);

void ROMOrder(vector<Line> &Line_Link);

void PrintToFile(vector<Line> &Line_Link, vector<Symbol> &Symbol_Link, vector<ERROR> &ERROR_link, string filename);

void DuplicateLabel(vector<Line> &Line_Link, vector<Symbol> &Symbol_Link, vector<ERROR> &ERROR_link);

void LineDisplay(struct Line x);

void SymbolDisplay(struct Symbol x);

int FindSymbolData(string QueryLabel, vector<Symbol> &Symbol_Link, struct Symbol &Temp_Symb);

int ERRORSearch(unsigned int line_number, vector<ERROR> &ERROR_link, unsigned int &error_iterator);

void ERRORSort(vector<ERROR> &ERROR_link);

void SecondPass(vector<Line> &Line_Link,vector<Symbol> &Symbol_Link);

int DBSizeCheck(struct Line &Line_Link);

void ObjectFileCreate(vector<Line> Line_Link, string filename);

void LabelSyntax(vector<Line> Line_Link, vector<ERROR> &ERROR_link, vector<Symbol> &Symbol_Link);

int SymbolSearch(vector<Symbol> Symbol_Link, string label);

// ==============================================================
// 			END 	OF 		FUNCTION 		DECLARATIONS
// ==============================================================

int main(void) {
	vector<Line> Line_Link;
	vector<ROM> ROM_Link; // can append addresses Address_Link[i]
	vector<Symbol> Symbol_Link; // 
	vector<ERROR> ERROR_link;
	eof END_info;
	END_info = {false,9999};

	string filename_base = "TestSourceCode";
	string A_file = filename_base + "A_E";
	string B = filename_base + "B";
	string C = filename_base + "C";
	string D = filename_base + "D";
	string E = filename_base + "E";
	string out_dir = "C:/Users/matth/Desktop/TTU/Summer_2019/Advanced_Microcontroller_Software/Assembler/Phase_1/Cpp/OutputFolder/";
	string file_in = A_file + ".s43";
	string file_list = out_dir + E + ".lst";
	string file_obj = out_dir + E + ".txt";


	// int name_size = (sizeof(filename));

	// cout << MakeUpper(filename,name_size) << endl;

	FillLineLinks(file_in, Line_Link);
	FilterComments(Line_Link); // information element has all comments removed
	LabelSearch(Line_Link);
	InstructionSearch(Line_Link);
	KeyFill(Line_Link, ERROR_link, END_info); // gets EOF
	if(!(END_info.found)){
		ERROR_link.push_back({(Line_Link.size()),0,(1<<2)}); // The last Line number, no previous label issue, 1 shifted left twice, 100
		cout << "\n\n\n\n\n\n\n\nCreated END directive error.\nLine number = " << ERROR_link.back().err_line << "\nBitfield: " << ERROR_link.back().bitfield << "\n\n\n\n\n\n\n";
	}
	InstructionType(Line_Link);
	OperandStrFill(Line_Link, END_info);
	FirstPass(Line_Link,Symbol_Link); // creates symbol table
	// DUPLICATE LABEL ERROR CHECKING
	DuplicateLabel(Line_Link,Symbol_Link,ERROR_link);
	// end
	// ======================================

	// not enough time to do syntax stuff

	// ======================================
	// Label syntax error checking
	// LabelSyntax(Line_Link,ERROR_link,Symbol_Link);
	// end
	SecondPass(Line_Link,Symbol_Link);
	SymbolOrder(Symbol_Link);
	DisplaySymbolTable(Symbol_Link);
	// ROMOrder(Line_Link);
	DisplayLineLink(Line_Link);
	// cout << endl << "\nSYMBOL TABLE" << endl;
	// DisplaySymbolTable(Symbol_Link);
	ERRORSort(ERROR_link);
	ROMOrder(Line_Link);
	PrintToFile(Line_Link,Symbol_Link,ERROR_link,file_list);
	cout << endl;
	LineDisplay(Line_Link[114]);
	SymbolDisplay(Symbol_Link[12]); // RESET
	cout << endl;
	struct Symbol Temp_Symb;
	int fake;
	fake = FindSymbolData("WDTCTL",Symbol_Link,Temp_Symb);
	cout << "integer returned from FindSymbolData: " << FindSymbolData("WDTCTL",Symbol_Link,Temp_Symb) << endl;
	if(fake)
		cout << "Error finding Symbol table data for label: WDTCTL" << endl;
	else
		cout << "No error searching for label: WDTCTL" << endl;
	if(ERROR_link.empty())// checks if no error
		ObjectFileCreate(Line_Link,file_obj);
	return 0;
}

// ==============================================================
// 		BEGINNING 	OF 		FUNCTION 		DEFINITIONS
// ==============================================================

void FillLineLinks(string filename, vector<Line> &Line_Link){
	ifstream inFile;

	inFile.open(filename);
	if(inFile.fail()){
		cerr << "It's fooked" << endl;
		exit(1);
	}
	int number = 0;

	while(!inFile.eof()){
		number++;
		string input;
		// string empty = "\0";
		getline(inFile,input);
		
		// vector.push_back appends a vector to the end
		// {line#,has_label,letters,information,label,instruction,key,address} contains all information which needs to be initialized in structure, in correct order
		Line_Link.push_back({number,0,0,0,0,0,0,0,0,0,0,0,0,false,true,input,"\0","\0","\0","\0","\0","\0",0,0});
	}

	// prints out our file
	for(int i = 0; i <Line_Link.size(); i++){
		int size = Line_Link[i].letters.length();
		// string str = Line_Link[i].letters;
		// int size = str.length();

		// do word processing
		// string information = getline(Line_Link[i].letters,size,';'); // reads entire string in if no comments
		// information has no comments
		// Line_Link[i].label = getline(Line_Link[i].letters,size,' ');
		cout << Line_Link[i].letters << endl; // endl adds a '\n'
	}

	inFile.close();
}

void FilterComments(vector<Line> &Line_Link){
	cout << "\n\nFILTERING COMMENTS\n\n";

	for(int i = 0; i < Line_Link.size(); i++){
		stringstream ss(Line_Link[i].letters);
		string information;
		getline(ss,information,';'); // reads entire string until comment or end
		Line_Link[i].information = information;
		cout << Line_Link[i].line_number << " " << Line_Link[i].information << endl;
	}
}

void LabelSearch(vector<Line> &Line_Link){
	cout << "\n\nSTRING INFORMATION PARSING \n\n";

	for(int i = 0; i <Line_Link.size(); i++){
		string label;
		stringstream ss(Line_Link[i].information);
		getline(ss,label,' '); // returns label
		//getline(ss1,)
		// cout << label << endl; // endl adds a '\n'
		if(label != "\0")
			Line_Link[i].has_label = true;
		// label.erase(remove_if(label.begin(),label.end(), ':'),Line_Link[i].Dest_Op.end());
		Line_Link[i].label = label;
	}

	for(int i = 0; i < Line_Link.size(); i++){
		if(Line_Link[i].has_label)
			cout << Line_Link[i].line_number << " " << Line_Link[i].label << endl;
	}
}

string MakeUpper(string anycase, int size){ // requires the string and its size (NOTE: size doesn't include null character)
	int i = 0;
	// char str[size] = anycase;
	char str[size + 1]; // for null character
	strcpy(str,anycase.c_str());
	while(str[i]){
		str[i] = toupper(str[i]);
		i++;
	}
	str[i] = '\0';
	return str;
} // works as planned

void InstructionSearch(vector<Line> &Line_Link){
	cout << "\n\nINSTRUCTION PARSING\n\n";

	// use Line_Link[i].information for strings without comments
	for(int i = 0; i < Line_Link.size(); i++){
		// cout << "Loop: " << i << endl;
		if(Line_Link[i].information.compare("\0")){
			stringstream ss(Line_Link[i].information);
			string instruction;

			if(Line_Link[i].has_label){ // use .information but skip label
			string waste;
			getline(ss,waste,' '); // returns label to waste
			}
			// keep getline with ' ' delimiter until get string != ' '
			while(ss.peek() == ' '){ // looks at next CHARACTER
				ss.get(); // reads in the character and increments, doesn't return anything
			}
			getline(ss,instruction,' '); // reads instruction until next space
			// cout << "Line: " << Line_Link[i].line_number << " " <<"Instruction string: " << instruction << endl;
			//stringstream ssNew(instruction); // check for byte or word
			int size = instruction.size(); // without null

			char instr_base[size + 1]; // add NULL
			char instr_cha[size + 1];
			strcpy(instr_cha,instruction.c_str());
			// cout << "Instruction character array: " << instr_cha << endl;
			stringstream ssNew(instr_cha);

			int j = 0;

			// peek() != -1
			// ssNew.eof() 
			while((ssNew.peek() != '.') && (!ssNew.eof())) { // end of line
				ssNew.get(instr_base[j]);
				j++;
				// cout << "Number of loops: " << j << "ssNew.peek = "<< ssNew.peek() <<endl;
			}
			// cout << "Leaving 'ssNew.peek() != '.'" << endl;
			instr_base[j] = '\0'; // Base instruction, no .b or .w

			Line_Link[i].instruction = instr_base; // instr_base might have 2 more elements after NULL

			while(ssNew.peek() == '.'){ // only loops one time, and checks if byte or word
				string byte = ".B";
				string tmp;
				getline(ssNew,tmp);
				tmp = MakeUpper(tmp,tmp.size());
				Line_Link[i].BW =	(tmp.compare(byte))? 0 : 1; // if byte, is 1
			}
			cout << "Line: " << Line_Link[i].line_number << "\tByte/Word: " << Line_Link[i].BW << endl;

			// string str_instr_base = instr_base;
			// getline(ssNew,instr_base,'.');

			while(ss.peek() == ' '){ // checks next char
				ss.get();
			}
			getline(ss,Line_Link[i].ROInstruction); // no delimiter, would've been ' ', but don't forget "$ - BufferTable"!!

			// .instruction still has .b in it
			cout << Line_Link[i].instruction << endl;
			cout << "ROInstruction: " << Line_Link[i].ROInstruction << endl;
			// getline(ss,instruction, ' ');
		} // end of for loop
	}
}


// right now doesn't also fill a B/W element --> byte and word operations of same mnemonic have different keys....
void KeyFill(vector<Line> &Line_Link, vector<ERROR> &ERROR_link, struct eof &END_info){
	cout << "\n\nINITIALIZING KEYS\n\n";

	// KEYS AND MNEMONICS SUBJECT TO CHANGE
	// CONSIDER HAVING BYTE/WORD ELEMENT IN VECTOR
	string mnemonics[] = {"EQU","ORG","DB","DW","DS","MOV","INC","DEC","JNZ","JZ","ADD","AND","BIC","BIS","BIT","CALL","CMP","RET","END"};
	int key_arr[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19}; // 7 and 7 = mov vs mov.w --> mnem's default as word ops

	int mnem_size = ((sizeof(mnemonics))/(sizeof(mnemonics[0]))); // works regardless of individual string size differences!

	for(int i = 0; i < Line_Link.size(); i++){
		stringstream ss(Line_Link[i].instruction);
		// in case I change the function to find word and byte operations as well!

		int instr_size = Line_Link[i].instruction.size(); // .size() doesn't include null character
		string upper_instr = MakeUpper(Line_Link[i].instruction,instr_size);
		// upper instruction is all in uppercase now
		
		for(int j = 0; j < mnem_size; j++){ // iterates thru entire string array
			if(upper_instr.compare(mnemonics[j]) == 0){
				Line_Link[i].key = key_arr[j];
			}
		} // added
		if(!(END_info.found)){ // not end of line
			switch(Line_Link[i].key){ // fill in opcode
				case 1: // EQU
				case 2: // ORG
				case 3: // DB
				case 4: // DW
				case 5: // DS
				case 19:// END
					Line_Link[i].OpCode = 0;
					if(Line_Link[i].key == 19){
						END_info.found = true;
						END_info.line_number = (i + 1);
					}
					break;
				case 18: // ret --> emulated
				case 6: // move
					Line_Link[i].OpCode = 0x4;
					break;
				case 7: // inc --> emulated
				case 11: // add
					Line_Link[i].OpCode = 0x5;
					break;
				case 8: // dec --> sub
					Line_Link[i].OpCode = 0x8;
					break;
				// jump opcode --> THREE BITS ONLY!!!! 0-7
				// opcode: 3 bits
				// 'C': 3 bits
				// jump 'opcode and C' is 6 most significan bits
				case 9: // jnz
					Line_Link[i].OpCode = (0<<5) + (0<<4) + (1<<3) + (0<<2) + (0<<1) + 0;
					break;
				case 10: // jz
					Line_Link[i].OpCode = (0<<5) + (0<<4) + (1<<3) + (0<<2) + (0<<1) + 1;
					break;
				// leaves room for 10-bit PC offset
				case 12: // and
					Line_Link[i].OpCode = 0xF;
					break;
				case 13: // bic
					Line_Link[i].OpCode = 0xC;
					break;
				case 14: // bis
					Line_Link[i].OpCode = 0xD;
					break;
				case 15: // bit
					Line_Link[i].OpCode = 0xB;
					break;
				case 16: // call
					Line_Link[i].OpCode = (1<<5) + (1<<2) + (1<<0);
					break;
				case 17: // cmp
					Line_Link[i].OpCode = 0x9;
					break;
				default:{
					Line_Link[i].OpCode = 0;
					if(upper_instr.compare("\0")){ // INVALID OPCODE ERROR
						cout << "Entered Error condition\n";
						cout << "Line_Link[i].key: " << Line_Link[i].key << endl;
						unsigned int error_iterator = 0;
						// 'i' = the line link iterator, + 1 is the line number we're checking for
						int line_check = ERRORSearch(((unsigned int)i + 1), ERROR_link, error_iterator);
						if(line_check){
							ERROR_link[error_iterator].bitfield |= (0b10); // the 2nd bit is opcode error
						} else {
							ERROR_link.push_back({((unsigned int)i + 1),0,(0b10)}); // 0 because no previously duplicated label line to point at
						}
						cout << "New Error created (bitfield = " << ERROR_link[error_iterator].bitfield << ")\n";
					}
				}
			}
		} else // reached end of file, so no more analysis
			Line_Link[i].OpCode = 0;
			/* str.compare() 
				- parameters:	1) str2 (always required)
								1-3) position str1, length str1, str2 (optional)
								1-5) pos str1,length str1, str2, pos str2, length str2 (optional)
								REMEMBER, position starts at 0

				- returns:		0	compare equal
								<0	Either: value of character in str1 is lower than str2 
									OR:		str1 is shorter
								>0	Either: value of character in str2 is lower than str1
									OR:		str2 is shorter											*/
		// }
		cout << "Line: " << Line_Link[i].line_number << "\tMnematic: " << Line_Link[i].instruction <<"\tKey: " << Line_Link[i].key << endl; // they keys of corresponding lines
	}
}

// single operand, double operand, jump instruction, assembler instruction
void InstructionType(vector<Line> &Line_Link){
	cout << "====================================";
	cout << "\n\nLABELING INSTRUCTION TYPES\n\n";
	cout << "====================================" << endl;

	// will probably use constants, since there won't be any extra looping
	// so this array just lists what integer data = what instr_type
	int instr_type[] = {1,2,3,4}; // assembler instr, single op, double op, jmp op
	// instr_type initialized to 0 --> no instruction

	// checks keys and assigns a value for instr_type
	for(int i = 0; i < Line_Link.size(); i++){
		// conditionally assigns instr_type
		Line_Link[i].instr_type =		((Line_Link[i].key > 0 ) && (Line_Link[i].key < 6 ) )? 1 : // EQU-END from KeyFill function
										((Line_Link[i].key == 7 ) || (Line_Link[i].key == 8 ))? 2 : // from our file, only have INC and DEC 
										((Line_Link[i].key == 6 ))? 3 : // from our file, only have mov instruction for double
										((Line_Link[i].key > 8 ))? 4 : 0; // from our file, only use JNZ and JZ
		cout << "Line: " << Line_Link[i].line_number;
		if(Line_Link[i].line_number < 10)
			cout << "\t";
		cout <<  "\tMnematic: " << Line_Link[i].instruction <<"\tKey: " << Line_Link[i].key; // they keys of corresponding lines
		cout << "\tInstuction type integer: " << Line_Link[i].instr_type << endl;
		cout << "\t";
		if(Line_Link[i].instr_type == 1){
			cout << "\tInstruction Type: ASSEMBLER" << endl;
		} else if(Line_Link[i].instr_type == 2) {
			cout << "\tInstruction Type: SINGLE OPERAND" << endl;
		} else if(Line_Link[i].instr_type == 3) {
			cout << "\tInstruction Type: DOUBLE OPERAND" << endl;
		} else if(Line_Link[i].instr_type == 4) {
			cout << "\tInstruction Type: JUMP" << endl;
		} else
			cout << "\tInstruction Type: NONE" << endl;
		cout << endl;
	}
} // now can filter based on instruction types for addressing

// fills the line_link[i].SourceOp and DestOp string values
void OperandFill(vector<Line> &Line_Link, struct eof &END_info){
	// cout << "\n\nFILLING OPERAND DATA\n\n";
	cout << "\n\nEverything to the right of instructions:\n\n";

	for(int i = 0; i < Line_Link.size(); i++){
		// SEE WHAT JUMP INSTRUCTIONS DO!!!
		if(i <= END_info.line_number){
			cout << "Line: " << Line_Link[i].line_number << "\t" << Line_Link[i].ROInstruction << endl;
			stringstream ss(Line_Link[i].ROInstruction);

			// could also use conditional for instruction types -> know how many operands therefor know the delimiter

			getline(ss,Line_Link[i].Source_Op,',');
		
			// can't check for the comma, just check to see if the line is finished or not
			if(!ss.eof()){ // find destination operand
				cout << "\nLine " << Line_Link[i].line_number << " Has detected a comma and destination op" << endl;
				ss.get(); // removes comma
				while(ss.peek() == ' '){ // remove any white space after the comma
					ss.get();
				}
				// getline starting at next non whitespace character
				getline(ss,Line_Link[i].Dest_Op,' '); // potential BUGG
			
				// check the rest of the line for information after spaces
				while(!ss.eof()){ // until end of line
					if(ss.peek() != ' '){
						string temp, dest_cat;
						dest_cat = Line_Link[i].Dest_Op;
						getline(ss,temp,' '); // input string into temp until either space or end of file
						Line_Link[i].Dest_Op = dest_cat + temp;
						// concat the two strings
					}
					if(ss.peek() == ' '){
						ss.get(); // remove whitespace...
					}
				}
			} else { // only source operand
				// cout << "\nLine " << Line_Link[i].line_number << " only has a source" << endl;
				stringstream ss(Line_Link[i].ROInstruction);
				getline(ss,Line_Link[i].Source_Op,' '); // potential BUGGGG
				// $ 
				while(!ss.eof()){ // until end of line
					if(ss.peek() != ' '){
						string temp, source_cat;
						source_cat = Line_Link[i].Source_Op;
						getline(ss,temp,' '); // input string into temp until either space or end of file
						Line_Link[i].Source_Op = source_cat + temp;
						// concat the two strings
					} // don't forget, getline skips a character
					if(ss.peek() == ' '){
						ss.get(); // remove whitespace...
					}
				}
			}

			cout << "Line: " << Line_Link[i].line_number << " " << endl;
			cout << "\tSource Op: " << Line_Link[i].Source_Op << " " << "\tDest Op: " << Line_Link[i].Dest_Op << endl;
		}
	} // end of for loop
}

// non assembler directives have addressing modes
// 1) checks operands for R0-R15 (or SP) --> initializes the integers with 0-F
//		- if found in string, checks for number(Rx) ie 0(Rx) or @Rx (or @Rx+) --> changes addressing mode
//			- only 0-FFFF(Rx) adds something to the ROM size
// 2) Check for an addressing mode prefix (#,&)
//		- # addressing mode immediate for source ONLY
//		- & addressing mode is for source and operand
//		- if none of these, addressing mode is going to be ADDR
// 3)
// 4)

// ===========================================================================
// 				BEGINNING OF NEW STR TO INT CONVERSION FUNCTIONS
// ===========================================================================
// symbol table used in strstr to int (secure) --> Checks for label addresses
int64_t DecHexToInt(string data, vector<Symbol> &Symbol_Link, struct Line Cur_Line){
	// first determine what base it is
	stringstream ssC(data);
	string negative_str;

	// check for base case
	// CHECK SIGN
	bool negative = false;
	if(ssC.peek() == '-'){ // first character peek
		cout << "First character is a negative" << endl;
		negative = true;
		ssC.get(); // discard the -
		getline(ssC,negative_str); // "-4-4" --> "4-4"
		int size = negative_str.size();
		for(int i = 0; i < size; i++){
			ssC.unget();
		}
	}
	// special cases where an entire character is a + or -
	if(!data.compare("'-'")){
		return '-';
	} else if(!data.compare("'+'")){
		return '+';
	}
	while(ssC.peek() != '+' && ssC.peek() != '-'){
		if(ssC.eof()){ // reached end of line
			// base case
			// cout << "ssC is at end of line: " << ssC.peek() << endl;
			int size = data.size();
			string final;
			if(negative){
				size = negative_str.size();
				final = MakeUpper(negative_str,size);
			} else{
				final = MakeUpper(data,size);
			}
			stringstream ssA(final);

			// returns decimal
			while(ssA.peek() != 'X'){ // hexadecimal
				ssA.get();
				if(ssA.eof()){
					// cout << "End of line: " << ssA.peek() << endl;
					if(negative){
						cout << "Returning a negative number from string: " << negative_str << endl;
						int x = stoll(negative_str,NULL,10);
						cout << "-stoll(data,NULL,10) " << x << endl;
						return -x;
					}
					else{
						return stoll(data,NULL,10);
					}
				}
			}
			// returns hex
			if(negative){
				cout << "NEGATIVE HEX from: " << negative_str << endl;
				int64_t x = stoll(negative_str,NULL,16);
				cout << "negative_str * -1 = " << -x << endl;
				// x *=-1;
				return -x;
			}
			else
				return stoll(data,NULL,16);
		}
		char tmp;
		ssC.get(tmp);
		cout << "Next character seen: " << tmp << endl;
	}
	// not in base case
	string left,right;
	int Lint,Rint;
	// stringstream ssP;
	if(negative){ // Lint = -Lint
		cout << "Recursion from negative:: "<< endl;
		stringstream ssP(negative_str);
		if(ssC.peek() == '+'){
			cout << "Recursion +" << endl;
			getline(ssP,left,'+');
			Lint = StrToIntS(left,Symbol_Link,Cur_Line);
			Lint *= -1; // negative bool
			// ssP.unget();
			cout << "Lint: " << Lint << endl;
			getline(ssP,right);
			Rint = StrToIntS(right,Symbol_Link,Cur_Line);
			cout << "Rint: " << Rint << endl;
			return Lint + Rint;
			// don't know how many more +s or -s there are
		} else if(ssC.peek() == '-'){
			cout << "Recursion -" << endl;
			getline(ssP,left,'-');
			Lint = StrToIntS(left,Symbol_Link,Cur_Line);
			Lint *=-1;
			ssP.unget(); // get the - sign back
			cout << "Lint: " << Lint << endl;
			getline(ssP,right);
			Rint = StrToIntS(right,Symbol_Link,Cur_Line);
			cout << "Rint: " << Rint << endl;
			return Lint + Rint; // keeping signs
			// don't know how many more +s or -s there are
		}
	} else{
		stringstream ssP(data);
		if(ssC.peek() == '+'){
			cout << "Recursion +" << endl;
			getline(ssP,left,'+');
			Lint = StrToIntS(left,Symbol_Link,Cur_Line);
			// ssP.unget();
			cout << "Lint: " << Lint << endl;
			getline(ssP,right);
			Rint = StrToIntS(right,Symbol_Link,Cur_Line);
			cout << "Rint: " << Rint << endl;
			return Lint + Rint;
			// don't know how many more +s or -s there are
		} else if(ssC.peek() == '-'){
			cout << "Recursion -" << endl;
			getline(ssP,left,'-');
			Lint = StrToIntS(left,Symbol_Link,Cur_Line);
			ssP.unget();
			cout << "Lint: " << Lint << endl;
			getline(ssP,right);
			Rint = StrToIntS(right,Symbol_Link,Cur_Line);
			cout << "Rint: " << Rint << endl;
			return Lint + Rint; // keeping signs
			// don't know how many more +s or -s there are
		}
	}

	return 8888;
} // recursion works

// for characters and strings ('v' or "EXAMPLES")
int64_t AsciiToInt(string data,vector<Symbol> &Symbol_Link, struct Line Cur_Line){ 
	cout << "Trying AsciiToInt..." << endl;
	cout << "String looking at: " << data << endl;
	stringstream ss(data);
	bool negative = false;
	if(ss.peek() == '-'){
		if(Cur_Line.line_number == 102)
			cout << "LINE 102 IS CONVERTING TO NEGATIVE\n";
		negative = true;
		ss.get();
	}
	if(ss.peek() == '$'){ // always first character in a 'operand'
		cout << "\n\n\n\n\n\n\n FOUND A $" << endl;
		cout << Cur_Line.line_number << " address: " << Cur_Line.first_address << endl;
		if(negative)
			return -Cur_Line.first_address;
		else
			return Cur_Line.first_address;
	}
	string label_check;
	getline(ss,label_check);
	int label_size = label_check.size();
	for(int i = 0; i < label_size; i++){
		ss.unget();
	}
	for(int i = 0; i < Symbol_Link.size(); i++){
		if(!label_check.compare(Symbol_Link[i].label)){
			if(negative){
				if(i == 5)
					cout << "BufferTable address (Symbol_Link[i].address): " << Symbol_Link[i].address << endl;
				return -Symbol_Link[i].address;
			}
			else
				return Symbol_Link[i].address;
		}
	}
	for(int i = 0; i < Symbol_Link.size(); i++){
		string tmp = label_check + ":";
		// tmp.erase(remove_if(tmp.begin(),tmp.end(), ::isspace),tmp.end());
		if(!tmp.compare(Symbol_Link[i].label)){
			if(negative){
				if(i == 5)
					cout << "BufferTable address (Symbol_Link[i].address): " << Symbol_Link[i].address << endl;
				return -Symbol_Link[i].address;
			}
			else
				return Symbol_Link[i].address;
		}
	}
	

	if(ss.peek() == '\''){
		ss.get(); // ignore '
		if(Cur_Line.line_number == 102){
			cout << "LOOKING AT THE '-' CHARACTER FROM LINE 102\n";
			char tmp;
			ss.unget();
			ss.unget();
			ss.get(tmp);
			cout << "ss.peek() = " << tmp << endl;
		}
		if(negative)
			return -ss.get();
		else
			return ss.get(); // error checking for the next '
	} else if(ss.peek() == '"'){
		ss.get(); // ignore "
		string tmp;
		getline(ss,tmp,'"');
		int count = tmp.size();
		for(int i = 0; i < count + 1; i++){
			ss.unget();
		}
		// cout << ss.get() << endl;
		// ss.unget();
		long long int x,w;
		x = 0;
		char y;
		while(ss.peek() != '"'){
			// cout << x << endl;
			count--;
			w = (ss.get());
			cout << "Count: " << count << " 'w': " << w << endl;
			// cout << w << " ";
			cout << "Byte shifting: << count*8 --> << (" << count << ")*8 = " << count*8 << endl;
			x += w << count*8; // "EXAMPLES" returning a negative number
		} // can put error checking for reaching end of line before '"'
		cout << endl;
		if(negative)
			return -x;
		else
			return x;
	}

	return 9999;
}

int64_t StrToIntS(string data, vector<Symbol> &Symbol_Link, struct Line Cur_Line){
	try{
		return DecHexToInt(data,Symbol_Link,Cur_Line);
	} catch (const invalid_argument &e) {
			cout << "First catch loop" << endl;
			return AsciiToInt(data,Symbol_Link,Cur_Line);
	}
}
// ===========================================================================
//	 				END OF NEW STR TO INT CONVERSION FUNCTIONS
// ===========================================================================


void OperandStrFill(vector<Line> &Line_Link, struct eof END_info){
	cout << "\n\nFILLING OPERAND STRINGS\n\n";
	for(int i = 0; i < Line_Link.size(); i++){
		// if(Line_Link[i].key != 0){
		if(i <= END_info.line_number){
			cout << Line_Link[i].line_number << " ROInstr: " << Line_Link[i].ROInstruction << endl;
			bool destination = false;
			stringstream ss(Line_Link[i].ROInstruction);
			getline(ss,Line_Link[i].Source_Op,',');
			Line_Link[i].Source_Op.erase(remove(Line_Link[i].Source_Op.begin(),Line_Link[i].Source_Op.end(),' '),Line_Link[i].Source_Op.end());
			while(!ss.eof()){ // if in this loop, there's a destination
				getline(ss,Line_Link[i].Dest_Op);
				Line_Link[i].Dest_Op.erase(remove_if(Line_Link[i].Dest_Op.begin(),Line_Link[i].Dest_Op.end(), ::isspace),Line_Link[i].Dest_Op.end());
			}
			cout << Line_Link[i].line_number << " Source: " << Line_Link[i].Source_Op << "\tDest: " << Line_Link[i].Dest_Op << endl;
		}
	}
}

// ===========================================================================
//		OPERAND INTEGER FILL:
//			-Source/Destination integer (inside instruction)
//			-Source/Destination addressing (inside instruction)
//			-Source/Destination additional ROM (outside instruction)
//			-Memory size for the instruction and operands
// ===========================================================================
void OperandIntFill(string input_operand,struct Line &Cur_Line, bool destination, vector<Symbol> &Symbol_Link){ // deep operand analysis, effects the addressing modes and the operand integer
	// start with source operand
	string Regs[] = {"R0","R1","R2","R3","R4","R5","R6","R7","R8","R9","R10","R11","R12","R13","R14","R15","SP","PC"}; // all registers
	int RegsInt[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,0}; // corresponding integers for each Register
	int RegsSize = ((sizeof(Regs))/(sizeof(Regs[0])));
	bool RMode = false;
	int addressing_mode = 0;
	int operand_int = 0;
	long long int operand_ROM = 0;
	for(int i = 0; i < RegsSize; i++){
		if(!(input_operand.compare(Regs[i]))){
			RMode = true;
			operand_int = RegsInt[i];
			addressing_mode = 0; // Register mode
			operand_ROM = 0; // no additional ROM for this operand
		}
	} // determines if in register mode
	stringstream ss(input_operand); // Source Op string
	// with more complex addressing, might run into labels --> Check symbol table
	if(!RMode){
		operand_int = 0; // unless using @Rn or @Rn+
		switch(ss.peek()){
			case '&': {// &ADDR --> ADDR stored in next word
				Cur_Line.Memory_Size += 2;
				addressing_mode = 1; // addressing mode
				ss.get(); // discards '&'
				// loops thru all of symbol table and string compares for label
				string cmp;
				getline(ss,cmp);
				operand_int = 0; // ??? doesn't work with &P1OUT stuff...
				operand_ROM = StrToIntS(cmp,Symbol_Link,Cur_Line);
				// ===========================================================================
				//		CONVERSION OF STRING TO AN INTEGER
				// 		set: operand_rom,	operand_int
				// ===========================================================================
				break;
			}
			case '#':{ // should just be immediate, check for labels
				Cur_Line.Memory_Size += 2;
				addressing_mode = 3;
				ss.get(); // ignore '#'
				// loops thru all of symbol table and string compares for label
				string cmp;
				getline(ss,cmp);
				operand_int = 0;
				operand_ROM = StrToIntS(cmp,Symbol_Link,Cur_Line);
				break;
			}
			case '@': {// check for @Rn or @Rn+
				ss.get(); // discards '@'
				ss.ignore(4,'+'); //
				stringstream ssReg(input_operand);
				ssReg.get(); // discard @
				string cmp;
				if(!ss.eof()){
					// if not the end of file, next character is '\0'
					// not end of file? then confirmed + detected
					addressing_mode = 3;
					getline(ssReg,cmp,'+');
				} else {
					addressing_mode = 2;
					getline(ssReg,cmp);
				}
				for(int i = 0; i < RegsSize; i++){
					if(!(cmp.compare(Regs[i]))){
						operand_int = RegsInt[i];
					}
				}
				operand_ROM = 0; // NO ADDITIONAL ROM ALLOCATED
				break;
			}
			default: {// ADDR and X(Rn) modes checked
				Cur_Line.Memory_Size += 2;
				addressing_mode = 1;
				bool IsIndexMode = false;
				int PrefixSize = 0;
				// check for X(Rn) first
				while(!ss.eof()){
					if(ss.peek() == '('){
						IsIndexMode = true;
					}
					ss.get();
					PrefixSize++;
				}
				for(int i = 0; i < PrefixSize; i++){
					ss.unget(); // resets the buffer
				}
				if(IsIndexMode){
					// indexed mode
					// NOTE: SOint will take up source integer
					// if SOint AND As/
					string cmp;
					getline(ss,cmp,'('); // going to discard this
					getline(ss,cmp,')');
					for(int i = 0; i < RegsSize; i++){
						if(!(cmp.compare(Regs[i]))){
							operand_int = RegsInt[i];
						}
					} // have the register now
					stringstream ssNew(input_operand);
					getline(ssNew,cmp,'('); // know this should be a number
					operand_ROM = StrToIntS(cmp,Symbol_Link,Cur_Line);
				} else { // ADDR
					operand_int = 0;
					string cmp;
					getline(ss,cmp);
					int tmp = StrToIntS(cmp,Symbol_Link,Cur_Line);
					if(Cur_Line.Memory_Size == 6 && destination){ // if memory size is max AND we are looking at the Dest Operand
						operand_ROM = tmp - (Cur_Line.first_address + 4); // the 3rd word
					} else {
						operand_ROM = tmp - (Cur_Line.first_address + 2); // the 2nd word
					}
					Cur_Line.ROMsource = tmp - (Cur_Line.first_address + 2); // first address + space for the instruction word --> pionts at this address
				}
				break;
			}
		}
	} // !RMode

	if(destination){
		Cur_Line.Ad = addressing_mode;
		Cur_Line.DOint = operand_int;
		Cur_Line.ROMdest = operand_ROM;
	} else {
		Cur_Line.As = addressing_mode;
		Cur_Line.SOint = operand_int;
		Cur_Line.ROMsource = operand_ROM;
	}

	// debugging
	// cout << "Finished OperandIntFill operation" << endl;
	// cout << "SOint: " << Cur_Line.SOint << "\tROMsource: " << Cur_Line.ROMsource << endl;
	// cout << "DOint: " << Cur_Line.DOint << "\tROMdest: " << Cur_Line.ROMdest << endl;
}

// still having issue with RESET


string IntToAddress(long long int x, int memory_size){
	// each integer of memory size is one byte
	// count how many bytes of 0 in MSB side
	stringstream ss;
	ss << hex << x;
	string y = "0x";
	/*
	int iterator = 0, tmp = x;
	bool check = false;
	while(!check){
		tmp = (x & (BYTE_MASK << (memory_size-iterator)) );
		if(tmp)
			check = true; //
		iterator++;
	}
	for(int i = 0; i < iterator; i++){
		y+="0";
	}
	*/
	return (y + ss.str());
	// return ss.str();
}


// =====================================================================================================
// 		TWO 	PASS 	ASSEMBLER
//
//		FIRST PASS
// 			-Get memory size, and create symbol table linked list
//
//		SECOND PASS
//			-assign ROM, which can now be properly done with filled link list (with proper symbol table)
// =====================================================================================================
// Line by Line analysis for first pass
void FirstPass(vector<Line> &Line_Link,vector<Symbol> &Symbol_Link){
	for(int i = 0; i < Line_Link.size(); i++){
		if(i == 0 && Line_Link[i].key == 0)
			Line_Link[i].first_address = 0;
		else if(Line_Link[i].key != 0){
			Line_Link[i].empty_rom = false;
			switch(Line_Link[i].key){
				case 1: {// EQU
					Line_Link[i].Memory_Size = 0;
					cout << "FirstPass: EQU -- " << Line_Link[i].label;
					Line_Link[i].first_address = Line_Link[i-1].first_address;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]); // turn source op string into integer
					cout << "\tOperand: " << Line_Link[i].SOint << "\tTo String: " << IntToAddress(Line_Link[i].SOint,Line_Link[i].Memory_Size);
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].SOint,Line_Link[i].label,IntToAddress(Line_Link[i].SOint,Line_Link[i].Memory_Size)});
					break;
				}
				case 2: {// ORG
					Line_Link[i].Memory_Size = 0;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]); // turn source op string into integer
					Line_Link[i].first_address = Line_Link[i].SOint;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].SOint,Line_Link[i].label,IntToAddress(Line_Link[i].SOint,Line_Link[i].Memory_Size)});
					break;
				}
				case 3: {// DB
					int DBcheck = DBSizeCheck(Line_Link[i]); // if works properly, mem size should have been changed
					if(DBcheck == 2){
						// ====================================================================================
						// ************************************************************************************
						// add error for improper concatonation of string ie "examples
						// ====================================================================================
						Line_Link[i].Memory_Size = 0; // no ROM could be made
					} else if(DBcheck == 0){ // returns a 0
						Line_Link[i].Memory_Size = 1; // ERROR: DB 	"EXAMPLES"
					}
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = Line_Link[i].SOint; // ERROR: DB 	"EXAMPLES"
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 4: {// DW
					Line_Link[i].Memory_Size = 2;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = Line_Link[i].SOint;
					cout << "DW rom from line: " << i << " ROM = SOint = " << IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size) << ", " << IntToAddress(Line_Link[i].SOint,Line_Link[i].Memory_Size) << endl;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 5: {// DS
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].Memory_Size = Line_Link[i].SOint; // turn the source op string into integer
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					// empty ROM
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 6: {// mov
					// source_op, destination = false
					Line_Link[i].Memory_Size = 2; // for the instruction
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					// dest_op, destination = true
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first

					// periodic word shifting
					if(Line_Link[i].Memory_Size >= 4){ // at least 2 words
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){ // 3 words
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}

					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 7: {// inc -- add
					Line_Link[i].Memory_Size = 2;
					
					// manually fill in source information
					Line_Link[i].SOint = 3; // Constant generator 2 used
					Line_Link[i].As = 1; // apparently
					// no source ROM
					// no additional memory allocated ^

					// NOTE --> 'source op' && destination = true? --> emulated instruction, really two operand but only has one
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],true,Symbol_Link);
					// only instruction ROM
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (3 << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As))<< 4) + (Line_Link[i].DOint); // 3 = CG2 or SR
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 8: {// dec -- sub
					Line_Link[i].Memory_Size = 2; // instruction allocation
					// mannually fill in source information
					Line_Link[i].SOint = 3;
					Line_Link[i].As = 1;

					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],true,Symbol_Link);
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (3 << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As))<< 4) + (Line_Link[i].DOint); // 3 = CG2 or SR for source op
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				// ====================================================================================================
				//	Don't use operandIntFIll for Jumps
				//		- PC offset = (PCnew - 2)/2
				// 	MAKE SURE TO WRITE TO SYMBOL TABLE BEFORE ANYTHING--> end of code jumps to itself ie: (Done 	jz 	done)
				// ====================================================================================================
				case 9: {//jnz
					Line_Link[i].Memory_Size = 2; // instruction allocation


					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					int PCnew;
					for(int a = 0; a < Symbol_Link.size(); a++){
						if(!(Symbol_Link[a].label.compare(Line_Link[i].Source_Op)))
							PCnew = Symbol_Link[a].address;
					}
					Line_Link[i].PCoffset = ((PCnew - 2)/2);
					Line_Link[i].PCoffset &= OFFSET_BITS;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 10) + (Line_Link[i].PCoffset);
					break;
				}
				case 10: {// jz
					Line_Link[i].Memory_Size = 2;

					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					int PCnew;
					for(int a = 0; a < Symbol_Link.size(); a++){
						if(!(Symbol_Link[a].label.compare(Line_Link[i].Source_Op)))
							PCnew = Symbol_Link[a].address;
					}
					Line_Link[i].PCoffset = ((PCnew - 2)/2);
					Line_Link[i].PCoffset &= OFFSET_BITS;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 10) + (Line_Link[i].PCoffset);
					break;
				}
				case 11: {// ADD
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 12: {// AND
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 13: {// BIC
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 14: {// BIS
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 15: {// BIT
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 16: {// CALL, ROM needs to be redone after first pass
					Line_Link[i].Memory_Size = 4; // instruction + operand's location
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 7) + (Line_Link[i].BW << 6) + (Line_Link[i].Ad << 4) + Line_Link[i].SOint;
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 17: {// CMP
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 18: {// RET --> emulated: MOV 	@SP+, PC
					Line_Link[i].Memory_Size = 2;
					Line_Link[i].SOint = 1; // SP
					Line_Link[i].DOint = 0; // PC
					Line_Link[i].As = 3;
					Line_Link[i].Ad = 0;
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (Line_Link[i].As << 4) + (Line_Link[i].Ad << 3) + (Line_Link[i].BW << 2) + (Line_Link[i].DOint);
					if(Line_Link[i].label.compare("\0"))
						Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
					break;
				}
				case 19: {// END
					
					break;
				}
			} // end of switch case 
		} // if(Line_Link[i].key != 0) finished
		else if(Line_Link[i].key == 0){
			Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
			if(Line_Link[i].label.compare("\0"))
				Symbol_Link.push_back({Line_Link[i].first_address,Line_Link[i].label,IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size)});
		}
	}
}

void SecondPass(vector<Line> &Line_Link,vector<Symbol> &Symbol_Link){
	for(int i = 0; i < Line_Link.size(); i++){
		if(i == 0 && Line_Link[i].key == 0)
			Line_Link[i].first_address = 0;
		else if(Line_Link[i].key != 0){
			Line_Link[i].empty_rom = false;
			switch(Line_Link[i].key){
				case 1: {// EQU
					Line_Link[i].Memory_Size = 0;
					Line_Link[i].first_address = Line_Link[i-1].first_address;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]); // turn source op string into integer
					break;
				}
				case 2: {// ORG
					Line_Link[i].Memory_Size = 0;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]); // turn source op string into integer
					Line_Link[i].first_address = Line_Link[i].SOint;
					break;
				}
				case 3: {// DB
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = Line_Link[i].SOint; // ERROR: DB 	"EXAMPLES"
					break;
				}
				case 4: {// DW
					Line_Link[i].Memory_Size = 2;
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = Line_Link[i].SOint;
					cout << "DW rom from line: " << i << " ROM = SOint = " << IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size) << ", " << IntToAddress(Line_Link[i].SOint,Line_Link[i].Memory_Size) << endl;
					break;
				}
				case 5: {// DS
					Line_Link[i].SOint = StrToIntS(Line_Link[i].Source_Op,Symbol_Link,Line_Link[i]);
					Line_Link[i].Memory_Size = Line_Link[i].SOint; // turn the source op string into integer
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					break;
				}
				case 6: {// mov
					// source_op, destination = false
					Line_Link[i].Memory_Size = 2; // for the instruction
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					// dest_op, destination = true
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first

					// periodic word shifting
					if(Line_Link[i].Memory_Size >= 4){ // at least 2 words
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){ // 3 words
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 7: {// inc -- add
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],true,Symbol_Link);
					// only instruction ROM
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (3 << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As))<< 4) + (Line_Link[i].DOint); // 3 = CG2 or SR
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					break;
				}
				case 8: {// dec -- sub
					Line_Link[i].Memory_Size = 2; // instruction allocation

					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],true,Symbol_Link);
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (3 << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As))<< 4) + (Line_Link[i].DOint); // 3 = CG2 or SR for source op
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					break;
				}
				case 9: {//jnz
					Line_Link[i].Memory_Size = 2; // instruction allocation


					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					int PCnew;
					for(int a = 0; a < Symbol_Link.size(); a++){
						if(!(Symbol_Link[a].label.compare(Line_Link[i].Source_Op)))
							PCnew = Symbol_Link[a].address;
					}
					Line_Link[i].PCoffset = ((PCnew - 2)/2);
					Line_Link[i].PCoffset &= OFFSET_BITS;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 10) + (Line_Link[i].PCoffset);
					break;
				}
				case 10: {// jz
					Line_Link[i].Memory_Size = 2;

					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					int PCnew;
					for(int a = 0; a < Symbol_Link.size(); a++){
						if(!(Symbol_Link[a].label.compare(Line_Link[i].Source_Op)))
							PCnew = Symbol_Link[a].address;
					}
					Line_Link[i].PCoffset = ((PCnew - 2)/2);
					Line_Link[i].PCoffset &= OFFSET_BITS;

					Line_Link[i].ROM = (Line_Link[i].OpCode << 10) + (Line_Link[i].PCoffset);
					break;
				}
				case 11: {// ADD
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 12: {// AND
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 13: {// BIC
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 14: {// BIS
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 15: {// BIT
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 16: {// CALL, ROM needs to be redone after first pass
					Line_Link[i].Memory_Size = 4; // instruction + operand's location
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 7) + (Line_Link[i].BW << 6) + (Line_Link[i].Ad << 4) + Line_Link[i].SOint;
					break;
				}
				case 17: {// CMP
					Line_Link[i].Memory_Size = 2;
					OperandIntFill(Line_Link[i].Source_Op,Line_Link[i],false,Symbol_Link);
					OperandIntFill(Line_Link[i].Dest_Op,Line_Link[i],true,Symbol_Link);
					
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (((Line_Link[i].Ad << 3)+(Line_Link[i].BW << 2)+(Line_Link[i].As)) << 4) + (Line_Link[i].DOint);// take care of instruction first
					if(Line_Link[i].Memory_Size >= 4){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMsource);
					}
					if(Line_Link[i].Memory_Size == 6){
						Line_Link[i].ROM = (Line_Link[i].ROM << 16) + (Line_Link[i].ROMdest);
					}
					break;
				}
				case 18: {// RET
					Line_Link[i].Memory_Size = 2;
					Line_Link[i].SOint = 1; // SP
					Line_Link[i].DOint = 0; // PC
					Line_Link[i].As = 3;
					Line_Link[i].Ad = 0;
					Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
					Line_Link[i].ROM = (Line_Link[i].OpCode << 12) + (Line_Link[i].SOint << 8) + (Line_Link[i].As << 4) + (Line_Link[i].Ad << 3) + (Line_Link[i].BW << 2) + (Line_Link[i].DOint);
				}
				case 19: {// END
					
					break;
				}
			} // end of switch case 
		} // if(Line_Link[i].key != 0) finished
		else if(Line_Link[i].key == 0){
			Line_Link[i].first_address = Line_Link[i-1].first_address + Line_Link[i-1].Memory_Size;
		}
	}
}

void SymbolOrder(vector<Symbol> &Symbol_Link){
	for(int j = 0; j < Symbol_Link.size(); j++){
		for(int i = 0; i < Symbol_Link.size(); i++){
			if((Symbol_Link[j].label.compare(Symbol_Link[i].label)) < 0){
				struct Symbol temp = Symbol_Link[j];
				Symbol_Link[j] = Symbol_Link[i];
				Symbol_Link[i] = temp;
			}
		}
	}
}

void DisplayLineLink(vector<Line> &Line_Link){
	cout << "\n\nDISPLAYING\tOPERANDS\tIN\tLINES\n\n";
	for(int i = 0; i < Line_Link.size(); i++){
		cout << Line_Link[i].line_number << "  " << IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size) << " " << IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size) << "\t" << Line_Link[i].letters << endl;
	}
}

void DisplaySymbolTable(vector<Symbol> &Symbol_Link){
	cout << "\n\nDISPLAYING\tSYMBOL\tTABLE\n\n";
	for(int i = 0; i < Symbol_Link.size(); i++){
		cout << Symbol_Link[i].label << " " << Symbol_Link[i].add_str << endl;
	}
}

// each line's rom needs to be ordered in Little Endian
//		- every even byte gets replaced with the previous odd byte
//		- Example:
//				Original = 0x4E0F --> Mov.w 	R14, R15
//				Little E = 0x0F4E --> MSB of word flips with LSB of word
//
//		- Example of a multiple word instruction-operand ROM blosck
//
//				
void ROMOrder(vector<Line> &Line_Link){
	// get the ROM data and the memory size
	cout << "Rorganizing the rom" << endl;
	for(int i = 0; i < Line_Link.size(); i++){
		uint64_t x = Line_Link[i].ROM;
		cout << "Line: " << Line_Link[i].line_number << endl;
		cout << "\tMemory_Size = " << Line_Link[i].Memory_Size << endl;
		cout << "\tMemory_Size/2 = " << Line_Link[i].Memory_Size / 2 << endl;
		if(Line_Link[i].Memory_Size / 2){ // if at least one word
			Line_Link[i].ROM = (x & 0x00FF000000000000) << 8 | (x & 0xFF00000000000000) >> 8 | (x & 0x000000FF00000000) << 8 | (x & 0x0000FF0000000000) >> 8 | (x & 0x0000000000FF0000) << 8 | (x & 0x00000000FF000000) >> 8 | (x & 0x00000000000000FF) << 8 | (x & 0x000000000000FF00) >> 8;
		} else { // at most one byte
			Line_Link[i].ROM = x;
		}
	}
}
// ==================================================================
// 		PRINT 		TO 		FILE
//
//	Finished:
//		-Line numbers
//		-ROM data
//		-Errors
//
// 	Need:
//		-Print EQU values at the 'line address' portion
//		-
// ==================================================================
void PrintToFile(vector<Line> &Line_Link, vector<Symbol> &Symbol_Link, vector<ERROR> &ERROR_link, string filename){
	ofstream outfile(filename);

	// error link list iterator
	int err_it = 0;

	// print source code with addresses and ROM included
	for(int i = 0; i < Line_Link.size(); i++){
		// EQU key = 1
		if(Line_Link[i].key == 1){ // replace line address with EQU constant value
			struct Symbol Temp_Symb;
			int test = FindSymbolData(Line_Link[i].label,Symbol_Link,Temp_Symb);
			if(test) // FindSymbolData returns -1 if error
				cout << "ERROR FINDING SYMBOL DATA FOR LABEL: " << Line_Link[i].label << endl;
			outfile << Line_Link[i].line_number << " " << Temp_Symb.add_str << " " << IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size) << " " << Line_Link[i].letters << endl;
		}
		else{ // print normal line addressing
			outfile << Line_Link[i].line_number << " " << IntToAddress(Line_Link[i].first_address,Line_Link[i].Memory_Size) << " " << IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size) << " " << Line_Link[i].letters << endl;
		}

		// only for duplicate label error check
		// use case statement eventually to have different error outputs
		if(Line_Link[i].line_number == ERROR_link[err_it].err_line){ // error found at this line
			outfile << "^\n\nError(s):\n";
			if(ERROR_link[err_it].bitfield & (1<<0))
				outfile << "Duplicate label: "<< Line_Link[i].label << " Found previously at line: " << ERROR_link[err_it].prev_label_line << endl;
			if(ERROR_link[err_it].bitfield & (1<<1))
				outfile << "Undefined instruction: " << Line_Link[i].instruction << endl;
			if(ERROR_link[err_it].bitfield & (1<<2))
				outfile << "Missing END assembler directive - ROM data has not been terminated properly" << endl;
			if(ERROR_link[err_it].bitfield & (1<<3))
				outfile << "Syntax Error reported for the label: " << Line_Link[i].label << endl;
			outfile << endl;
			err_it++;
		}
	}

	outfile << endl;

	outfile << "SYMBOL TABLE" << endl;

	for(int i = 0; i < Symbol_Link.size(); i++){
		outfile << Symbol_Link[i].label << " " << Symbol_Link[i].add_str << endl;
	}

	if(!ERROR_link.empty()){
		outfile << "\nERROR LIST" << endl;
		for(int i = 0; i < ERROR_link.size(); i++){
			outfile << "Error number: "<< i << "\tFound at line: "<< ERROR_link[i].err_line << "\tBitfield code: " << ERROR_link[i].bitfield << endl;
		}
		outfile << "\nBitfield key:\n2\t1\t0\nSyntax\tEND err\tDuplicate Label\n";
	}

	outfile.close();
}


// ==================================================================
//	FIND 		SYMBOL 		DATA
//
//	Goal:		Return a symbol table structure by searching thru
//					symbol linked list for the corresponding label
// ==================================================================
int FindSymbolData(string QueryLabel, vector<Symbol> &Symbol_Link, struct Symbol &Temp_Symb){
	// cout << "\n\nFinding Symbol Table entry using the query label: " << QueryLabel << endl << endl;
	for(int i = 0; i < Symbol_Link.size(); i++){
		// cout << "Comparing with label: " << Symbol_Link[i].label << " Return of .compare function: " << Symbol_Link[i].label.compare(QueryLabel) << endl;
		if(!(Symbol_Link[i].label.compare(QueryLabel))){
			// cout << "Found label match (Original = " << QueryLabel << "), (Symbol table label = " << Symbol_Link[i].label << ")" << endl;
			Temp_Symb.label = Symbol_Link[i].label;
			Temp_Symb.address = Symbol_Link[i].address;
			Temp_Symb.add_str = Symbol_Link[i].add_str;
			return 0;
		}
	}
	Temp_Symb = {0, "\0","\0"}; // didn't find a matching symbol table
	return 1;

}

// associates the label from a symbol table to label from a line, then grabs the address
// usually want to AddressFromSymb(Line_Link[i].label,Symbol_Link.begin()) // from beginning


// this function is called before the ordering of the symbol table --> Delete the 'newest' copy of label

void DuplicateLabel(vector<Line> &Line_Link, vector<Symbol> &Symbol_Link, vector<ERROR> &ERROR_link){
	// look thru symbol table
	unsigned int original, copy;

	// find errors
	int err_it = -1;
	for(original = 0; original < Symbol_Link.size() - 1; original++){
		// size - 1 because the last link in symbol table won't be checked with anything else
		for(copy = original + 1; copy < Symbol_Link.size(); copy ++){
			if(!(Symbol_Link[original].label.compare(Symbol_Link[copy].label))){
				err_it++;
				// cout << "Matched two labels" << endl;
				// cout << "Original: " << original << " " << Symbol_Link[original].label << endl;
				// cout << "Copy: " << copy << " " << Symbol_Link[copy].label << endl;
				
				// create new error struct that links with the coppied label's line number
				unsigned int find_copy = 2, line_iterator = 0, first_label = 0;
				while(find_copy != 0){
					// only checks for lines with labels
					if(Line_Link[line_iterator].label.compare("\0")){
						if(!(Line_Link[line_iterator].label.compare(Symbol_Link[copy].label))){
							if(find_copy == 2)
								first_label = line_iterator;

							find_copy--;
						}
					}
					if(find_copy != 0)
						line_iterator++;
				} // at end, line iterator points to the copy
				// line_iterator = line_number of error
				// first_label = line_number of original label string that is copied
				// ture = the bit field for errors (1-bit right now == duplicate label)

				// err_line, prev_label line, duplication label
				unsigned int error_iterator = 0, line_check = ERRORSearch(line_iterator + 1, ERROR_link, error_iterator);
				if(line_check){
					cout << "Error already exists at line: " << line_iterator + 1 << endl;
					cout << "Previous error: " << ERROR_link[error_iterator].bitfield << endl;
					ERROR_link[error_iterator].bitfield |= (1<<0); // sets the LSbit for an error
					ERROR_link[error_iterator].prev_label_line = first_label + 1;
					cout << "Total error bit field: " << (ERROR_link[error_iterator].bitfield & (1<<1)) << "-" << (ERROR_link[error_iterator].bitfield & (1<<0)) << endl;
				}
				else
					ERROR_link.push_back({(line_iterator + 1),(first_label + 1),(1<<0)}); // + 1 because line numbers start at 1
				cout << "Error number " << err_it << " created, error line: " << line_iterator << " Original label found at line: " << first_label << endl;
				// delete the symbol link
				// decrement the iterator
				// int temp_reference = &Symbol_Link[copy];
				Symbol_Link.erase (Symbol_Link.begin() + copy);
				copy--;
			}
		}
	}


	cout << ERROR_link.size() << " error(s) detected in symbol vector" << endl;
	cout << Symbol_Link.size() << " links in Symbol_Link vector total" << endl;
}

// checks to see if error already exists at a line number
int ERRORSearch(unsigned int line_number, vector<ERROR> &ERROR_link, unsigned int &error_iterator){
	for(int i = 0; i < ERROR_link.size(); i++){
		if(ERROR_link[i].err_line == line_number){
			error_iterator = i;
			return 1;
		}
	}
	return 0;
}

void LineDisplay(struct Line x){
	cout << "Displaying Line number: " << x.line_number << endl;
	cout << "Key: " << x.key << endl;
	cout << "Instruction type "<< x.instr_type << endl;
	// IntToAddress issue only outputes 4 bytes --> issue
	cout << "Rom data: " << IntToAddress(x.ROM,x.Memory_Size) << endl;
	cout << "Rom from source: " << IntToAddress(x.ROMsource,x.Memory_Size) << endl;
	cout << "Rom from dest: " << IntToAddress(x.ROMdest,x.Memory_Size) << endl;
	cout << "Byte/Word bit: "<< x.BW << endl;
	cout << "Dest Addresing mode: "<< x.Ad << endl;
	cout << "Aource addressing mode: "<< x.As<< endl;
	cout << "Source integer: "<< x.SOint << endl;
	cout << "Dest integer: "<< x.DOint<< endl;
	cout << "Opcode: "<< x.OpCode<< endl;
	cout << "PCoffset: "<< x.PCoffset<< endl;
	cout << "Has label boolean: "<< x.has_label << endl;
	cout << "empty rom boolean: "<< x.empty_rom << endl;
	cout << "Line entirety: "<< x.letters<< endl;
	cout << "Line minus ';': "<< x.information<< endl;
	cout << "Line Label: "<< x.label<< endl;
	cout << "Instruction: "<< x.instruction << endl;
	cout << "Right of instruction string: "<< x.ROInstruction<< endl;
	cout << "Source operand: "<< x.Source_Op<< endl;
	cout << "Destination operand: "<< x.Dest_Op<< endl;
	cout << "Line address: "<< IntToAddress(x.first_address,x.Memory_Size)<< endl;
	cout << "ROM memory size in bytes: "<< x.Memory_Size<< endl;
}

void SymbolDisplay(struct Symbol x){
	cout << "Desplaying Symbol label: " << x.label << endl;
	cout << "Address: " << x.add_str << endl;
}

void ERRORSort(vector<ERROR> &ERROR_link){
	for(int i = 0; i < (ERROR_link.size() - 1); i++){
		for(int j = i+1; j < ERROR_link.size(); j++){
			if(ERROR_link[i].err_line > ERROR_link[j].err_line){
				struct ERROR temp = ERROR_link[j];
				ERROR_link[j] = ERROR_link[i];
				ERROR_link[i] = temp;
			}
		}
	}
}

int DBSizeCheck(struct Line &Line_Link){
	stringstream ss(Line_Link.Source_Op);
	int size = 0;
	int ret_val = 0;
	if(ss.peek() == '"'){ // first character is alphabetical
		ret_val = 1;
	} else {
		return ret_val;
	}
	ss.get(); // get rid of the " character 
	while(ss.peek() != '"'){
		if(ss.eof()){
			return 2; // error, no " detected to end string!
		}
		ss.get();
		size++;
	}
	Line_Link.Memory_Size = size;
	return 1;
	// now size equals mem size
		;
}

void ObjectFileCreate(vector<Line> Line_Link, string filename){
	ofstream outfile(filename);
	outfile << "MatthewBailey00";
	// Begins each ORG with "FF00AA55"
	bool InROM = false, end = false;
	int ROM_section = 0, old_ROM_section = 0;
	for(int i = 0; i < Line_Link.size(); i++){
		//string mnemonics[] = {"EQU","ORG","DB","DW","DS","MOV","INC","DEC","JNZ","JZ","ADD","AND","BIC","BIS","BIT","CALL","CMP","RET","END"};
		//int key_arr[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19}; // 7 and 7 = mov vs mov.w --> mnem's default as word ops
		switch(Line_Link[i].key){
			case 1: // EQU
			case 5: // DS
				break;
			case 2: // ORG
				if(!InROM)
					InROM = true;
				outfile << "FF00AA55";
				break;
			case 3: // DB
			case 4: // DW
			case 6: // MOV
			case 7: // INC
			case 8: // DEC
			case 9: // JNZ
			case 10: // JZ
			case 11: // ADD
			case 12: // AND
			case 13: // BIC
			case 14: // BIS
			case 15: // BIT
			case 16: // CALL
			case 17: // CMP
			case 18: // RET
				// ========================================================
				// ISSUE: ANY OPERAND ERROR WILL STILL HAVE A KEY???
				// FIX:
				//		- CHECK FOR ERRORS HERE (HARD)
				//		- RESET KEYS UPON FINDING OPERAND ERROR (EASY)
				// ========================================================
				if(InROM && !end) {// need to be inside a rom section and before the end directive
					string tmp = IntToAddress(Line_Link[i].ROM,Line_Link[i].Memory_Size);
					cout << "Line: " << Line_Link[i].line_number << "\tMemory_Size: " << Line_Link[i].Memory_Size <<"\tROM (decimal): " << Line_Link[i].ROM << endl;
					cout << "Line_Link[i].Memory_Size > 0: " << (Line_Link[i].Memory_Size > 0) << endl;
					if(Line_Link[i].ROM == 0){
						for(int i = 0; i <= Line_Link[i].Memory_Size; i++){
							outfile << "00";
							cout << "ADDING IN BYTE OF 0's" << endl;
						}
					} else {
						tmp.erase(0,2); // remove '0x' ?? 0,1 = x12345678 , still x there?
						outfile << tmp;
					}
				}
				break;
			case 19: // END, should only have read one end key since KeyFill function stopped reading after END
				end = true;
				outfile << "FFAA5500";
				break;
				// don't cout any data if there's no instruction
		}
	}
	outfile.close();
}

void LabelSyntax(vector<Line> Line_Link, vector<ERROR> &ERROR_link, vector<Symbol> &Symbol_Link){
	cout << "\n\n\n\nLABEL SYNTAX STUFF\n\n\n";
	for(int i = 0; i < Line_Link.size(); i++){
		cout << "Line checking: " << i << endl;
		bool check = false;
		int err_it = 0;
		if(Line_Link[i].label.compare("\0")){
			stringstream ss(Line_Link[i].label);
			// error check
			string tmp;
			if(ss.peek() >= '0' || ss.peek() <= '9'){
				cout << "=============================" << endl;
				for(int j = 0; j < ERROR_link.size(); j++){
					if(ERROR_link[j].err_line == Line_Link[i].line_number){
						check = true;
						err_it = j;
					}
				}
				if(check)
					ERROR_link[err_it].bitfield |= (1<<3);
				else
					ERROR_link.push_back({(unsigned int)Line_Link[i].line_number,0,(1<<3)});
				Symbol_Link.erase (Symbol_Link.begin() + (SymbolSearch(Symbol_Link,Line_Link[i].label)));
			}
			while(!ss.eof()){
				// error check
				if(((ss.peek() > '/' && ss.peek() < ';') || (ss.peek() > '@' && ss.peek() < '[' ) || (ss.peek() > '`' && ss.peek() < '{')) && (ss.peek() != '_')){
					cout << "1=============================" << endl;
					for(int j = 0; j < ERROR_link.size(); j++){
						if(Line_Link[i].line_number == ERROR_link[j].err_line){
							check = true;
							err_it = j;
						}
					}
					if(check)
						ERROR_link[err_it].bitfield |= (1<<3);
					else
						ERROR_link.push_back({(unsigned int)Line_Link[i].line_number,0,(1<<3)});
					Symbol_Link.erase (Symbol_Link.begin() + (SymbolSearch(Symbol_Link,Line_Link[i].label)));
				}
				ss.get();
			}
		}
	}
}

int SymbolSearch(vector<Symbol> Symbol_Link, string label){
	for(int i = 0; i < Symbol_Link.size(); i++){
		if(!(Symbol_Link[i].label.compare(label)))
			return i;
	}
	return -1;
}