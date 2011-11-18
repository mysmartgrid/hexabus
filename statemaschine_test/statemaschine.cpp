#include <iostream>
#include "sm_format.h"
#include <fstream>

using namespace std;

Condition cond_table[1];
Transition trans_table[2];

bool eval(uint8_t cond_index, uint8_t ip, uint8_t eid, uint8_t value) { 		// FIXME: Pointer to packet 
  cout << "Evaluating condition #" << (int)cond_index << endl;
  Condition* cond = &cond_table[cond_index];
	// Do nothing if ip and eid don't match
	if(cond->ip != ip || cond->eid != eid) {
		//cout << "Nope, Chuck Testa." << endl;
		return false;
	}

	switch(cond->op) {
		case eq:
			cout << "Check for ==" << endl;
			return (cond->value == value);
		case leq:
			cout << "Check for <=" << endl;
			return (cond->value <= value);
		case geq:
			cout << "Check for >=" << endl;
			return (cond->value >= value);
		case lt:
			cout << "Check for <" << endl;
			return (cond->value < value);
		case gt:
			cout << "Check for >" << endl;
			return (cond->value > value);
		default:
			return false;
	}
}

int testfunction(uint8_t i)
{
  cout << "Function " << (int)i << " running. Return Value? (0 = Cool, anything else = Uncool): ";
  int ret;
  cin >> ret;
  return ret;
}

//Read transition and condition table from file
int readConfig(const char *transFileName, uint8_t transTableLength, const char *condFileName, uint8_t condTableLength) {
  //Read Transition table
  ifstream transIn;
  transIn.open(transFileName, ios::in | ios::binary);
  transIn.read((char*)&trans_table, transTableLength*sizeof(struct Transition));
  transIn.close();
  cout << "Transition Table:" << endl;
  for(int i = 0;i < transTableLength;i++) {
	cout << (int)trans_table[i].fromState << " " << (int)trans_table[i].cond << " " << (int)trans_table[i].action << " " << (int)trans_table[i].toCool << " " << (int)trans_table[i].toUncool << endl;
  }
  
  //Read Condition Table
  ifstream condIn;
  condIn.open(condFileName, ios::in | ios::binary);
  condIn.read((char*)&cond_table, condTableLength*sizeof(struct Condition));
  condIn.close();
  cout << "Condition Table:" << endl;
  for(int i = 0;i < condTableLength;i++) {
	cout << (int)cond_table[i].ip<< " " << (int)cond_table[i].eid << " " << cond_table[i].op << " " << (int)cond_table[i].value << endl;
  return 0;
  }
}

int main() {
	readConfig("trans.dat", 2, "cond.dat", 1);
	uint8_t curState;
	uint8_t ip, eid, value;
	int tmp;

  // So now let's do an actual state machine
  curState = 0; // start out in State 0.
  while(true)
  {
	// ask user for input
    cout << "Enter IP (uint8): ";
    cin >> tmp;
    ip = tmp;
    cout << "Enter EID (uint8): ";
    cin >> tmp;
    eid = tmp;
    cout << "Enter value (uint8): ";
    cin >> tmp;
    value = tmp;

    for(uint8_t i = 0;i < (sizeof(trans_table)/sizeof(struct Transition));i++)
    {
      if((trans_table[i].fromState == curState) && (eval(trans_table[i].cond, ip, eid, value)))
      {
        // Found match
        cout << "Executing function number " << (int)trans_table[i].action << endl;
        if(testfunction(trans_table[i].action) == 0)
        {
          cout << "New State = Cool State = " << (int)trans_table[i].toCool << endl;
          curState = trans_table[i].toCool;
          break;
        } else {
          cout << "New State = Uncool State = " << (int)trans_table[i].toUncool << endl;
          curState = trans_table[i].toUncool;
          break;
        }
      }
    }
  }
	return 0;
}

