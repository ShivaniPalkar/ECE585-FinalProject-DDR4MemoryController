#include<iostream>
#include<fstream>
#include<iomanip>
#define Q_SIZE 16
#define MAX 999999
	
int first_flag = 1;
//Time_paras.h
//Timing parameters in CPU clock cycles (DRAM cycles/2)
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
    long int low_column;
    long int bank_gp;
    long int bank;
    long int high_column;
    long int row;
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

void final_display(int b, int bg, int r, int c, int command)
{
    if (command == 0)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "PRE";
        std::cout << std::hex << left << setw(15) << bg << left << setw(15) << b << std::dec << endl; 
    }
    
    else if (command == 1)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "ACT";
        std::cout << std::hex << left << setw(15) << bg << left << setw(15) << b << left << setw(15) << r << std::dec << endl; 
    }
    
    else if (command == 2)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "READ";
        std::cout << std::hex << left << setw(15) << bg << left << setw(15) << b << left << setw(15) << c << std::dec << endl; 
    }
    
    else if (command == 3)
    {
        cout << left<< setw(10) << clk_tick << left << setw(10) << "WRITE";
        std::cout << std::hex << left << setw(15) << bg << left << setw(15) << b << left << setw(15) << c << std::dec << endl; 
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
		    
			//cout << left<< setw(20) << clk_tick << left << setw(20) << op;
			//std::cout << std::hex << left << setw(15) << q[size].low_column << left << setw(15) << q[size].bank_gp << left << setw(15) << q[size].bank << left << setw(15) << q[size].high_column << left << setw(15) << q[size].row << std::dec << "Evict" << endl; 
			for(int k=1; k <size; k++)
			{
				q[k-1].age = q[k].age;
				q[k-1].type = q[k].type;
				q[k-1].row = q[k].row;
				q[k-1].bank_gp = q[k].bank_gp;
				q[k-1].bank = q[k].bank;
				q[k-1].low_column = q[k].low_column;
				q[k-1].high_column = q[k].high_column;
				q[size].age = 0;
				q[size].type = 0;
				q[size].row = 0;
				q[size].bank = 0;
				q[size].bank_gp = 0;
				q[size].high_column = 0;
				q[size].low_column = 0;
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
    if(status[bg][b].array_of_time_commands[command] != RAS)
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
    for(int u = 0; u < size; u++)
    {
        if (status[q[u].bank_gp][q[u].bank].bank_access == 1)
        {
            if(status[q[u].bank_gp][q[u].bank].row_open == q[u].row && q[u].type == 0)
            {
                bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,2);
                final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 2);
                
                if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[2] != (CAS + BURST - 1))
                {   bsr_counter_fn(q[u].bank_gp,q[u].bank,2);
                    goto ende;
                }
                else 
                {
                    evict();
                    reset_bsr_counters(q[u].bank_gp,q[u].bank);
                }
            }
                
            else if(status[q[u].bank_gp][q[u].bank].row_open == q[u].row && q[u].type == 1)
            {
                bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,3);
                final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 3);
                if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[3] != (CWD + BURST - 1))
                {
                    bsr_counter_fn(q[u].bank_gp,q[u].bank,3);
                    goto ende;
                }
                else 
                {
                    evict();
                    reset_bsr_counters(q[u].bank_gp,q[u].bank);
                }
            }

            else if(status[q[u].bank_gp][q[u].bank].row_open != q[u].row)
            {
                bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,0);
                final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 0);
                if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[0] != (RP - 1))
                {    
                    bsr_counter_fn(q[u].bank_gp,q[u].bank,0);
                    goto ende;
                }
                else 
                {
                    bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,1);
                    final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 1);
                    
                    if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[1] != (RCD))
                    {
                        bsr_counter_fn(q[u].bank_gp,q[u].bank,1);
                        goto ende;
                    }
                    else 
                    {   
                        if(q[u].type == 0)
                        {
                            bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,2);
                            final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 2);
                            if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[2] != (CAS + BURST))
                            {    
                                bsr_counter_fn(q[u].bank_gp,q[u].bank,2);
                                goto ende;
                            }    
                            else 
                            {
                                evict();
                                status[q[u].bank_gp][q[u].bank].row_open = q[u].row;
                                reset_bsr_counters(q[u].bank_gp,q[u].bank);
                            }
                        }

                        else if(q[u].type == 1)
                        {
                            bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,3);
                            final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 3);
                            if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[3] != (CWD + BURST))
                            {    
                                bsr_counter_fn(q[u].bank_gp,q[u].bank,3);
                                goto ende;
                            }
                            else 
                            {
                                evict();
                                status[q[u].bank_gp][q[u].bank].row_open = q[u].row;
                                reset_bsr_counters(q[u].bank_gp,q[u].bank);
                            }
                        }
                    }
                }
            }
        }

        else if(status[q[u].bank_gp][q[u].bank].bank_access == 0)
        {   if(file_cnt == 1)
                status[q[u].bank_gp][q[u].bank].array_of_time_commands[1] = 1;
                
            bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,1);
            final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 1);
            status[q[u].bank_gp][q[u].bank].row_open = q[u].row;
            if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[1] != (RCD - 1))
            {
                bsr_counter_fn(q[u].bank_gp,q[u].bank,1);
                goto ende;
            }
            else
            {
                if(q[u].type == 0)
                {
                    bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,2);
                    final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 2);
                    if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[2] != (CAS + BURST))
                    {
                        bsr_counter_fn(q[u].bank_gp,q[u].bank,2);
                        goto ende;
                    }    
                    else 
                    {
                        evict();
                        reset_bsr_counters(q[u].bank_gp,q[u].bank);
                        status[q[u].bank_gp][q[u].bank].bank_access = 1;
                    }  
                }
                else if (q[u].type == 1)
                {    
                    bank_register_update(q[u].bank_gp,q[u].bank,q[u].row,3);
                    final_display(q[u].bank, q[u].bank_gp, q[u].row, q[u].high_column, 3);
                    if(status[q[u].bank_gp][q[u].bank].array_of_time_commands[3] != (CWD + BURST))
                    {
                        bsr_counter_fn(q[u].bank_gp,q[u].bank,3);
                        goto ende;
                    }    
                    else 
                    {
                        evict();
                        reset_bsr_counters(q[u].bank_gp,q[u].bank);
                        status[q[u].bank_gp][q[u].bank].bank_access = 1;
                    }
                }
            }
        }
    }
    
    }
    
    ende: return;
}

void file_read (ifstream& inFile)
{
	//Variable Declarations
	std::string line;
	string t;
			if (inFile.eof()) {
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
            q[size].low_column = add & 0x38;
            q[size].low_column = q[size].low_column >> 3;
            q[size].bank_gp = add & 0xC0;
            q[size].bank_gp = q[size].bank_gp >> 6;
            q[size].bank = add & 0x300;
            q[size].bank = q[size].bank >> 8;
            q[size].high_column & 0x3FC00;
            q[size].high_column = q[size].high_column >> 10;
            q[size].row & 0x1FFFC0000;
            q[size].row = q[size].row >> 18;
            q[size].type = typ;
            
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
            
            //cout << left<< setw(20) << clk_tick << left << setw(20) << op;
			//std::cout << std::hex << left << setw(15) << q[size].low_column << left << setw(15) << q[size].bank_gp << left << setw(15) << q[size].bank << left << setw(15) << q[size].high_column << left << setw(15) << q[size].row << std::dec << "Push" << endl; 

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
		if (first_flag == 0 && (size == 0)) {
		  
            inFile.close();    
		    exit (0);
		}
	}
	return 0;
}