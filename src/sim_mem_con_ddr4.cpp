///////////////////////////////////////////////////////////////////////////////
// DDR-4 DRAM Memory Controller 
//
// Memory Controller implementing open page policy without access scheduling.
// Reads memory references from a file and stores them in a queue. After that
// it services them one by one in order. All the DRAM commands issued by the 
// memory controller are ouput to a text file. Once, a memory request is 
// serviced completely it is evicted from the queue.
//
// Authors: Mehul Shah, Shivani Palkar, Ramaa Potnis, Ritvik Tiwari
////////////////////////////////////////////////////////////////////////////////

#include<iostream>
#include<fstream>
#include<iomanip>
#define Q_SIZE 16
#define MAX 999999

// Timing Parameters in CPU cycles
#define RC 152 //Row-cycle time
#define RAS 104 // Row active time
#define RRD_L 12// Row to row delay_long
#define RRD_S 16// Row to row delay_short
#define RP 48// Row precharge time
#define RFC 1120// Row refresh cycle time
#define CWD 40// delay between col-write and data valid on bus
#define CAS 48// column address strobe
#define RCD 48// Row-column delay
#define WR  40// Write to read delay
#define RTP 48//read to precharge
#define CCD_L 16//column to column delay_long
#define CCD_S 8// column to column delay_short
#define BURST 8// Burst cycle
#define WTR_L 24 // Write to read_long
#define WTR_S 8 // Write to read_short
#define REFI 24960 //refresh interval

using namespace std;

//declaring global variables
long int add;
int first_flag = 1;
int typ;
long int m_time;
long int byte_order;
long int low_column;
long int bank_gp;
long int bank;
long int high_column;
long int row;

int i=0;
int debug_md;
long long int clk_tick;
int file_cnt=0;
int size_q=0;

int pd_req=0;
string op;

// Queue of struct type which maintains all the memory requests
struct queue
{
    long int age;	
    int type;
    long int address;
};
queue q[16];

//Bank Status Register
struct BSR
{
	bool bank_access;
    long int row_open;
    int cmd;
    int array_of_time_commands[4];
};
BSR status[4][4] = {0,0,0,{1,0,0,0}};

// Final DRAM Command Output
void final_display(ofstream &outFile, int b, int bg, int r, int lc, int hc, int command)
{   unsigned int c;
    c = (hc << 3) | lc;
    if (command == 0)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "PRE";
        std::cout << std::hex << left << setw(8) << bg << left << setw(8) << b << std::dec << endl; 
        outFile << clk_tick << "\t" << "PRE" << "\t" << std::hex << bg << "\t" << b << "\t" << std::dec;
    }
    
    else if (command == 1)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "ACT";
        std::cout << std::hex << left << setw(8) << bg << left << setw(8) << b << left << setw(10) << r << std::dec << endl; 
        outFile << clk_tick << "\t" << "ACT" << "\t" << std::hex << bg << "\t" << b << "\t" << r << std::dec;
    }
    
    else if (command == 2)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "RD";
        std::cout << std::hex << left << setw(8) << bg << left << setw(8) << b << left << setw(10) << c << std::dec << endl; 
        outFile << clk_tick << "\t" << "RD" << "\t" << std::hex << bg << "\t" << b << "\t" << c << std::dec;
    }
    
    else if (command == 3)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "WR";
        std::cout << std::hex << left << setw(8) << bg << left << setw(8) << b << left << setw(10) << c << std::dec << endl; 
        outFile << clk_tick << "\t" << "WR" << "\t" << std::hex << bg << "\t" << b << "\t" << c << std::dec;
    }

    outFile << std::endl;
}

// Address breakdown to obtain bank, bank group etc.
void add_bd(long int add_1)
{
	for (int n = 0; n < i; n++)
	{
		byte_order = add_1 & 0x07;
		
		low_column = add_1 & 0x38;
		low_column = low_column >> 3;
		
		bank_gp = add_1 & 0xC0;
		bank_gp = bank_gp >> 6;

		bank = add_1 & 0x300;
		bank = bank >> 8;

		high_column = add_1 & 0x3FC00;
		high_column = high_column >> 10;

		row = add_1 & 0x1FFFC0000;
		row = row >> 18;
	}

}

// Evict function
void evict()
{	    
		    switch(q[0].type) 
		    {
   				case 0 :
      				op = "data read";
      				break; 
   				case 1 :
      				op = "data write";
      				break; 
				case 2 :
      				op = "instruction fetch";
      				break; 
   				default : 
      				op = "nop";
			}
		    
			for(int k=1; k <size_q; k++)
			{
				q[k-1].age = q[k].age;
				q[k-1].type = q[k].type;
				q[k-1].address = q[k].address;
				q[size_q].age = 0;
				q[size_q].type = 0;
				q[size_q].address = 0;
            }
            size_q--;
}

// BSR Update Function
void bank_register_update(int bg, int b, long int r, int command)
{
    status[bg][b].cmd = command;
}

// BSR Counters
void bsr_counter_fn(int bg, int b, int command)
{
    if(status[bg][b].array_of_time_commands[command] != RC)
    {
        status[bg][b].array_of_time_commands[command]++;
    }
}

// BSR Reset Counter
void reset_bsr_counters(int bg, int b)
{
    for(int u = 0; u < 4; u++)
    {
        status[bg][b].array_of_time_commands[u] = 0;
    }
}


// DRAM command generator
void gen(ofstream &outFile)
{  
    if(size_q != 0)
    {
        add_bd(q[0].address);
        
        // Bank has been accessed before. i.e some row is open 
        if (status[bank_gp][bank].bank_access == 1)
        {
            if((status[bank_gp][bank].row_open == row && q[0].type == 0) || (status[bank_gp][bank].row_open == row && q[0].type == 2) )
            {
                bank_register_update(bank_gp,bank,row,2);
                if(status[bank_gp][bank].array_of_time_commands[2] == 0)
                    {final_display(outFile, bank, bank_gp, row, low_column, high_column, 2);}
                
                if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST))
                    bsr_counter_fn(bank_gp,bank,2);
                else 
                {
                    evict();
                    reset_bsr_counters(bank_gp,bank);
                }
            }

            else if((status[bank_gp][bank].row_open == row && q[0].type == 1))
            {
                bank_register_update(bank_gp,bank,row,3);
                if(status[bank_gp][bank].array_of_time_commands[3] == 0)
                    {final_display(outFile, bank, bank_gp, row, low_column, high_column, 3);}

                if(status[bank_gp][bank].array_of_time_commands[3] != (CWD + BURST))
                    bsr_counter_fn(bank_gp,bank,3);
                else 
                {
                    evict();
                    reset_bsr_counters(bank_gp,bank);
                }
            }

            else if(status[bank_gp][bank].row_open != row)
            {
                bank_register_update(bank_gp,bank,row,0);
                if(status[bank_gp][bank].array_of_time_commands[0] == 0)
                    {final_display(outFile, bank, bank_gp, row, low_column, high_column, 0);}
                
                if(status[bank_gp][bank].array_of_time_commands[0] != (RP))
                    bsr_counter_fn(bank_gp,bank,0);
                else 
                {
                    bank_register_update(bank_gp,bank,row,1);
                    if(status[bank_gp][bank].array_of_time_commands[1] == 0)
                        {final_display(outFile, bank, bank_gp, row, low_column, high_column, 1);}
                    
                    if(status[bank_gp][bank].array_of_time_commands[1] != (RCD))
                        bsr_counter_fn(bank_gp,bank,1);
                    else 
                    {   
                        if(q[0].type == 0 || q[0].type == 2)
                        {
                            bank_register_update(bank_gp,bank,row,2);
                            if(status[bank_gp][bank].array_of_time_commands[2] == 0)
                                {final_display(outFile, bank, bank_gp, row, low_column, high_column, 2);}
                            
                            if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST))
                                bsr_counter_fn(bank_gp,bank,2);
                            else 
                            {
                                evict();
                                status[bank_gp][bank].row_open = row;
                                reset_bsr_counters(bank_gp,bank);
                            }
                        }

                        else if(q[0].type == 1)
                        {
                            bank_register_update(bank_gp,bank,row,3);
                            if(status[bank_gp][bank].array_of_time_commands[3] == 0)
                                {final_display(outFile, bank, bank_gp, row, low_column, high_column, 3);}
                            
                            if(status[bank_gp][bank].array_of_time_commands[3] != (CWD + BURST))
                                bsr_counter_fn(bank_gp,bank,3);
                            else 
                            {
                                evict();
                                status[bank_gp][bank].row_open = row;
                                reset_bsr_counters(bank_gp,bank);
                            }
                        }
                    }
                }

            }

        }

        // First access to the bank. bank is precharged
        else if(status[bank_gp][bank].bank_access == 0)
        {
            bank_register_update(bank_gp,bank,row,1);
            if(status[bank_gp][bank].array_of_time_commands[1] == 0)
                {final_display(outFile, bank, bank_gp, row, low_column, high_column, 1);}
                
            status[bank_gp][bank].row_open = row;
            if(status[bank_gp][bank].array_of_time_commands[1] != (RCD))
            {
                bsr_counter_fn(bank_gp,bank,1);
            }
            else
            {
                if(q[0].type == 0 || q[0].type == 2)
                {
                    bank_register_update(bank_gp,bank,row,2);
                    if(status[bank_gp][bank].array_of_time_commands[2] == 0)
                        {final_display(outFile, bank, bank_gp, row, low_column, high_column, 2);}
                    
                    if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST))
                        bsr_counter_fn(bank_gp,bank,2);
                    else 
                    {
                        evict();
                        reset_bsr_counters(bank_gp,bank);
                        status[bank_gp][bank].bank_access = 1;
                    }  
                }
                else if (q[0].type == 1)
                {    
                    bank_register_update(bank_gp,bank,row,3);
                    if(status[bank_gp][bank].array_of_time_commands[3] == 0)
                        {final_display(outFile, bank, bank_gp, row, low_column, high_column, 3);}
                    
                    if(status[bank_gp][bank].array_of_time_commands[3] != (CWD + BURST))
                        bsr_counter_fn(bank_gp,bank,3);
                    else 
                    {
                        evict();
                        reset_bsr_counters(bank_gp,bank);
                        status[bank_gp][bank].bank_access = 1;
                    }
                }
            }
        }
    }   
}

// File Read Function
void file_read (ifstream& inFile)
{
	//Variable Declarations
	std::string line;
	string t;
            if (inFile.eof()){
                inFile.close();
                first_flag = 0;
            }    
			inFile >> m_time >> typ >> std::hex >> add >> std::dec;
			
				switch(typ) {
   					case 0 :
      					t = "data read";
      					break; 
   					case 1 :
      					t = "data write";
      					break; 
					case 2 :
      					t = "instruction fetch";
      					break; 
   					default : 
      					t = "nop";
				}
				
				if (debug_md == 1) {
					cout << left<< setw(20) << m_time << left << setw(20) << t;
				    std::cout << std::hex << left << setw(10) << add << std::dec << endl; 
				}
				i++;
}

// Time advance
void timeadvancing()
{
    if(!size_q && m_time != 0)
    {
        clk_tick = m_time;
            if (debug_md == 1) 
            {
                cout << endl << "Advancing cpu cycles to " << clk_tick << endl << endl;
            }   
    }
		
}

//Push to the queue
void push(ifstream& inFile)
{       if (clk_tick==0)
        {file_read(inFile);}
        
        if((clk_tick==m_time && size_q != 16) || (pd_req == 1 && size_q!=16 && inFile.is_open()))
        {
            q[size_q].age= clk_tick;
            m_time=clk_tick;
            q[size_q].address= add;
            q[size_q].type= typ;
            
            switch(q[size_q].type) 
		    {
   				case 0 :
      				op = "data read";
      				break; 
   				case 1 :
      				op = "data write";
      				break; 
				case 2 :
      				op = "instruction fetch";
      				break; 
   				default : 
      				op = "nop";
			}
            
            size_q++;
            file_cnt++;
            file_read(inFile);
            pd_req=0;
        }
        else if(size_q == 16)
		{   pd_req = 1;
		}
}

// Main Function
int main()
{   string file_nm;	
    string out_nm;
	//User input for filename and debug mode	
	cout <<"Enter the name of the trace file: ";
	cin >> file_nm;
	
	cout <<"Do you want to debug (1/0): ";
	cin >> debug_md;
	
    // Output File name
	cout <<"Enter the name of the output file: ";
	cin >> out_nm;
	
	//File read
	ifstream inFile(file_nm);
	ofstream outFile;
    outFile.open(out_nm, std::ios::out);

		//Check file availability
	if(!inFile)
	{
			cout << "No such file";
			exit(1);
	}
		//Check whether file is empty
	bool isEmpty = inFile.peek() == EOF;
	if(isEmpty)
	{
		cout << boolalpha << "File is empty";	
		exit(1);
	}	
	
	while(1)
    {
        timeadvancing();
        push(inFile);
        gen(outFile);
	    
	    clk_tick++;
		if (first_flag == 0 && (size_q == 0)) {
            outFile.close();
		    exit (0);
		}
    }	
    outFile.close();
	return 0;
}
