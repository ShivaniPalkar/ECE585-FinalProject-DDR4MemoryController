#include<iostream>
#include<fstream>
#include<iomanip>
#define Q_SIZE 16
#define MAX 999999

using namespace std;

//declaring a global variable which can be later used to add with the counter
long int add[MAX]={0};
int typ[MAX]={0};
long int m_time[MAX]={0};
int i=0;
int debug_md;
long int clk_tick;
int file_cnt=0;
int size_q=0;
string op;

struct queue
{
    long int age;	
    int type;
    long int address;
};
queue q[16];

void file_read ()
{
	//Variable Declarations
	std::string line;
	string t;
	string file_nm;	

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
		
		//Data Parse and output	
		if (inFile.is_open()){
		    if (debug_md == 1) 
		    {
		        cout << endl << "File Output: " << endl;
		        cout<< left<< setw(20)<<"CPU Cycle"<< left<< setw(20)<< "Type"<< left<< setw(10)<< "Address"<< endl;
		    }
		    
			while (inFile >> m_time[i] >> typ[i] >> std::hex >> add[i] >> std::dec)
			{
				switch(typ[i]) {
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
					cout << left<< setw(20) << m_time[i] << left << setw(20) << t;
				    std::cout << std::hex << left << setw(10) << add[i] << std::dec << endl; 
				}
				i++;
			}
		}
	
	inFile.close();
}


void timeadvancing()
{
    if(!size_q && m_time[file_cnt] != 0)
    {
        clk_tick = m_time[file_cnt];
            if (debug_md == 1) 
            {
                cout << endl << "Advancing cpu cycles to " << clk_tick << endl << endl;
            }   
    }
		
}


void evict()
{
    
    for (int j=0; j <size_q; j++)
  	{
		if(clk_tick == 100 + q[j].age)
		{
		    
		    switch(q[j].type) 
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
		    
			cout << left<< setw(20) << clk_tick << left << setw(21) << op;
			std::cout << std::hex << left << setw(14) << q[j].address << std::dec << left << setw(10) << "Evict" << endl; 
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
            j--;
		}
	}
}


void push()
{
    for (int l = 0; l < i; l++)
	{
        if(clk_tick==m_time[l] && size_q != 16)
        {
            q[size_q].age= m_time[l];
            q[size_q].address= add[l];
            q[size_q].type= typ[l];
            
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
            
            cout << left<< setw(20) << clk_tick << left << setw(21) << op;
			std::cout << std::hex << left << setw(14) << q[size_q].address << std::dec << left << setw(10) << "Push" << endl; 
            size_q++;
            file_cnt++;
        }
        else if(clk_tick == m_time[l] && size_q == 16)
		{
		    m_time[l] = q[0].age + 100;
		}
	}
}

int main()
{
	file_read();
    cout<<endl<< "Queue Output: " << endl << left<< setw(20)<< "CPU Cycle"<< left<< setw(21)<< "Type"<< left<< setw(14)<< "Address"<< left<< setw(5)<< "Push/Evict"<< endl;
	while(1)
    {
        timeadvancing();
        evict();
	    push();
	    clk_tick++;
		if ((clk_tick >= (m_time[i - 1] + 100)) && (size_q == 0))
		    exit (0);
	}
	return 0;
}