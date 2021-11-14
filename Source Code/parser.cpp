#include<iostream>
#include<fstream>
using namespace std;

int main() {

	//Variable Declarations
	std::string line;
	int time1, t;
	string type;
	string address, file_nm;	
	int i = 0;
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
			while (!inFile.eof())
			{
				inFile >> time1 >> t >> address;
				switch(t) {
   					case 0 :
      					type = "data read";
      					break; 
   					case 1 :
      					type = "data write";
      					break; 
					case 2 :
      					type = "instruction fetch";
      					break; 
   					default : 
      					type = "nop";
				}
				
				if (debug_md == 1) {
					cout << "Time: " << time1 <<"\t"<< "Address: " << address <<"\t\t"<< "Type: " << type <<endl;	
				}
			}
		}
	
inFile.close();
return 0;
}
