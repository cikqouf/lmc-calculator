#include<iostream>
#include<string>
using namespace std;
string Ep;
char p;
int n = 0;
int i =1;

class computing
{
    public:

        char plus = '+';
        char minus = '-';
        char multiply = '*';
};

void console()
{
    cout<< "quit? "; cin>> p;
            switch (p)
            {
                case 'N':
                n = 0;
                break;
                case 'Y':
                n = i + 10;
                break;
                default:
                cerr<< "wrong! ";
            }
}

void inout()
{
    for(int i =1; n < i; i++)
        {
            cout<< "[In" << i << "]>>> " ; cin>> Ep;
            cout<< "[Out" << i << "]<<< " << Ep <<endl;
            console();
        }
}

int main()
{
    inout();   
    system("pause");
    return 0;
}