#include<iostream>
#include<iomanip>
#include<string>
using namespace std;
int n;
double value = 30;
class computing
{
	public:
		string plus()
		{
			return "+";
		}
		string minus()
		{
			return "-";
		}
};


int main()
{
	char s;
        double *ptr_value;
	ptr_value = &value;

	string var[]{"090","09587","3746","sodaj"};

	string *ptr_var;
	ptr_var = var;

	cout<< "quanity "; cin>> n;

	for(int i=0; i<n; i++)
	{
		cout<< *(ptr_var + i) << endl;
	}
	
	cout<< *ptr_value << endl;

	cout<< "ok" << endl;


	computing cp;
	cout<< "Computing mode "; cin>> s;
	switch(s)
	{
		case '+': cout<< "jiafa" << cp.plus() << endl;
	        break;
		case '-': cout<< "jianfa" << cp.minus() << endl;
		break;
		default : cerr<< "illegal!" << endl;
	}
	system("pause");
	return 0;
}

