#include<iostream>
#include<fstream>
#include<iomanip>
#define Q_SIZE 16
#define MAX 999999

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

//declaring a global variable which can be later used to add with the counter
long int add;
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
long int clk_tick;
int file_cnt=0;
int size=0;
long int evict_time=0;
int pd_req=0;
string op;

struct queue
{
    long int age;	
    int type;
    long int address;
};
queue q[16];

struct BSR
{
	bool bank_access;
    long int row_open;
    int cmd;
    int array_of_time_commands[4];
};
BSR status[4][4] = {0,0,0,{0,0,0,0}};

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
	
		//std::cout << std::hex << left << setw(35) << byte_order << left << setw(35) << low_column << left << setw(35) << bank_gp << left << setw(35) << bank << left << setw(35) << high_column << left << setw(35) << row << std::dec << endl; 
	}

}

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
		    
			cout << left<< setw(20) << clk_tick << left << setw(20) << op;
			std::cout << std::hex << left << setw(10) << q[0].address << std::dec << left << setw(10) << "Evict" << endl; 
			for(int k=1; k <size; k++)
			{
				q[k-1].age = q[k].age;
				q[k-1].type = q[k].type;
				q[k-1].address = q[k].address;
				q[size].age = 0;
				q[size].type = 0;
				q[size].address = 0;
            }
            size--;
            evict_time = clk_tick;
}


void bank_register_update(int bg, int b, long int r, int command)
{
    //status[bank][bg].bank_access = 1;
    //status[bg][b].row_open = r;
    status[bg][b].cmd = command;
}

void bsr_counter_fn(int bg, int b, int command)
{
    if(status[bg][b].array_of_time_commands[command] != 38)
    {
        status[bg][b].array_of_time_commands[command]++;
    }
}

void reset_bsr_counters(int bg, int b)
{
    for(int u = 0; u < 4; u++)
    {
        status[bg][b].array_of_time_commands[u] = 0;
    }
}



void gen()
{  
    if(size != 0)
    {
        add_bd(q[0].address);

        if (status[bank_gp][bank].bank_access == 1)
        {
            if(status[bank_gp][bank].row_open == row && q[0].type == 1)
            {
                bank_register_update(bank_gp,bank,row,2);
                
                if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST - 1))
                    bsr_counter_fn(bank_gp,bank,2);
                else 
                {
                    evict();
                    reset_bsr_counters(bank_gp,bank);
                }
            }

            // write comand
            else if(status[bank_gp][bank].row_open == row && q[0].type == 2)
            {
                bank_register_update(bank_gp,bank,row,3);

                if(status[bank_gp][bank].array_of_time_commands[3] != (CAS + BURST - 1))
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

                if(status[bank_gp][bank].array_of_time_commands[0] != (RP - 1))
                    bsr_counter_fn(bank_gp,bank,0);
                else 
                {
                    bank_register_update(bank_gp,bank,row,1);
                    
                    if(status[bank_gp][bank].array_of_time_commands[1] != (RCD))
                        bsr_counter_fn(bank_gp,bank,1);
                    else 
                    {   
                        if(q[0].type == 1)
                        {
                            bank_register_update(bank_gp,bank,row,2);
                
                            if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST))
                                bsr_counter_fn(bank_gp,bank,2);
                            else 
                            {
                                evict();
                                status[bank_gp][bank].row_open = row;
                                reset_bsr_counters(bank_gp,bank);
                            }
                        }

                        else if(q[0].type == 2)
                        {
                            bank_register_update(bank_gp,bank,row,3);

                            if(status[bank_gp][bank].array_of_time_commands[3] != (CAS + BURST))
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

        else if(status[bank_gp][bank].bank_access == 0)
        {   if(file_cnt == 1)
                status[bank_gp][bank].array_of_time_commands[1] = 1;
                
            bank_register_update(bank_gp,bank,row,1);
            status[bank_gp][bank].row_open = row;
            if(status[bank_gp][bank].array_of_time_commands[1] != (RCD - 1))
            {
                bsr_counter_fn(bank_gp,bank,1);
            }
            else
            {
                if(q[0].type == 1)
                {
                    bank_register_update(bank_gp,bank,row,2);
                    if(status[bank_gp][bank].array_of_time_commands[2] != (CAS + BURST))
                        bsr_counter_fn(bank_gp,bank,2);
                    else 
                    {
                        evict();
                        reset_bsr_counters(bank_gp,bank);
                        status[bank_gp][bank].bank_access = 1;
                    }  
                }
                else if (q[0].type == 2)
                {    
                    bank_register_update(bank_gp,bank,row,3);
                    
                    if(status[bank_gp][bank].array_of_time_commands[3] != (CAS + BURST))
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

void file_read (ifstream& inFile)
{
	//Variable Declarations
	std::string line;
	string t;
            if (inFile.eof()){
                inFile.close();
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

void timeadvancing()
{
    if(!size && m_time != 0)
    {
        clk_tick = m_time;
            if (debug_md == 1) 
            {
                cout << endl << "Advancing cpu cycles to " << clk_tick << endl << endl;
            }   
    }
		
}

void push(ifstream& inFile)
{       if (clk_tick==0)
        {file_read(inFile);}
        
        if((clk_tick==m_time && size != 16) || (pd_req == 1 && size!=16 && inFile.is_open()))
        {
            q[size].age= clk_tick;
            m_time=clk_tick;
            q[size].address= add;
            q[size].type= typ;
            
            switch(q[size].type) 
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
            
            cout << left<< setw(20) << clk_tick << left << setw(20) << op;
			std::cout << std::hex << left << setw(10) << q[size].address << std::dec << left << setw(10) << "Push" << endl; 
            size++;
            file_cnt++;
            file_read(inFile);
            pd_req=0;
        }
        else if(size == 16)
		{   pd_req = 1;
		}
}

int main()
{   string file_nm;	

	//User input for filename and debug mode	
	cout <<"Enter the name of the trace file: ";
	cin >> file_nm;
	
	cout <<"Do you want to debug (1/0): ";
	cin >> debug_md;
	
	//File read
	ifstream inFile(file_nm);

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
		
		//Data Parse and output	
	
	
	/*if (inFile.is_open()){
		if (debug_md == 1) 
		{
		    cout << endl << "File Output: " << endl;
		    cout<< left<< setw(20)<<"CPU Cycle"<< left<< setw(20)<< "Type"<< left<< setw(10)<< "Address"<< endl;
		}
        
    }*/

	//add_bd();
    
	
        timeadvancing();
        push(inFile);
        gen();
	    
	    clk_tick++;
		if ((clk_tick >= 44) && (size == 0))
		    exit (0);
	}
	return 0;
}