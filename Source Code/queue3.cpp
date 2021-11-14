#include<iostream>
#include<fstream>
#include<iomanip>
using namespace std;

//assumed a tracefile exists
#define Q_SIZE 16
#define MAX 999999

//declaring a global variable which can be later used to add with the counter
long int add[MAX]={0};
int typ[MAX]={0};
int cnt[MAX]={0};
int i=0;

long int clk_tick;

struct queue
{
int count;	
int type;
long int address;
};

void file_read ()
{
	//Variable Declarations
	std::string line;
	string type;
	string file_nm;	
	int debug_md;

	//User input for filename and debug mode	
	cout <<"Enter the name of the trace file: ";
	cin >> file_nm;
	
	cout <<"Do you want to debug the parser (1/0): ";
	cin >> debug_md;
	
	//File read
	ifstream inFile(file_nm);

		//Check file availability
		if(!inFile){
			cout << "No such file";
			exit(1);
		}
		//Check whether file is empty
		bool isEmpty = inFile.peek() == EOF;
		if(isEmpty){
			cout << boolalpha << "File is empty";	
		}	
		
		
		//Data Parse and output	
		if (inFile.is_open()){
			while (inFile >> cnt[i] >> typ[i] >> std::hex >> add[i] >> std::dec)
			{
				
				if (debug_md == 1) {
					//cout << "Time: " << cnt[i] <<"\t";
					//std::cout << "Address: " << std::hex << add[i] <<"\t\t";
					//cout << "Type: " << typ[i] <<endl;	
				}
				i++;
			}
		}
	
	inFile.close();
}

int main()
{
	int file_cnt=0;
	queue q[16];
	int size=0;
	
	file_read();

	while(1)
    {
        /*if(!size)
        {
            clk_tick=cnt[file_cnt];
        }*/
		
		
		for (int j=0; j <size; j++)
  		{
			if(clk_tick == 100 + q[j].count)
			{
				cout << "CPU Cycle: " <<clk_tick << "\t" << "Popped element: " << q[j].type << "\t";
				std::cout << std::hex << q[j].address << std::dec <<endl; 
				for(int k=1; k <size; k++)
				{
					q[k-1].count = q[k].count;
					q[k-1].type = q[k].type;
					q[k-1].address = q[k].address;
					q[size].count = 0;
					q[size].type = 0;
					q[size].address = 0;
                }
                size--;
			}
		}
		
		for (int l = 0; l < i; l++)
		{
		
            if(clk_tick==cnt[l] && size != 16)
            {
                q[size].count= cnt[l];
                q[size].address= add[l];
                q[size].type= typ[l];
                cout << "CPU Cycle: " <<clk_tick << "\t" << "Pushed element: " << q[size].type << "\t";
			    std::cout << std::hex << q[size].address << std::dec <<endl;
                size++;
            }
            
		    else if(clk_tick == cnt[l] && size == 16)
		    {
			    cnt[l] = q[0].count + 101;
		    }
		}
		
		clk_tick++;
	}
	
}