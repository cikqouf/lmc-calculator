#include<iostream>
#include<string>
using namespace std;
string Ep;
char p;

void inout()
{
    int n = 0;
    for(int i =1; n < i; i++)
        {
            cout<< "[In" << i << "]>>> " ; cin>> Ep;
            cout<< "[Out" << i << "]<<< " << Ep <<endl;
            cout<< "End? "; cin>> p;
            switch (p)
            {
                case 'N':
                n = 0;
                break;
                case 'Y':
                n = 999;
                break;
                default:
                cerr<< "wrong! ";
            }

        }
            
}

int main()
{
    inout();   
    system("pause");
    return 0;
}