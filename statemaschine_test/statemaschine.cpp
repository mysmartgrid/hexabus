#include <iostream>
#include "sm_format.h"

using namespace std;

bool eval(Condition *cond, uint8_t ip, uint8_t eid, uint8_t value) { 		// FIXME: Pointer to packet 
	// Do nothing if ip and eid don't match
	if(cond->ip != ip || cond->eid != eid) {
		//cout << "Nope, Chuck Testa." << endl;
		return false;
	}
	// Great, now we have to do actual work
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

int testfunction(int i)
{
  return 1;
}

int main() {
	//Simple Example:
	Condition trigger;
	trigger.ip = 1;
	trigger.eid = 1;
	trigger.op = eq;
	trigger.value = 1;

	Transition offToOn;
	offToOn.fromState = 0;
	offToOn.cond = &trigger;
	offToOn.action = 1;
	offToOn.toCool = 1;
	offToOn.toUncool = 0;

	Transition onToOff;
	onToOff.fromState = 1;
	onToOff.cond = &trigger;
	onToOff.action = 0;
	onToOff.toCool = 0;
	onToOff.toUncool = 1;
	
	Transition table[2] = {offToOn, onToOff};
	uint8_t curState;
	uint8_t ip, eid, value;
	int tmp;
	cout << "Enter IP (uint8):";
	cin >> tmp;
	ip = tmp;
	cout << "Enter EID (uint8)";
	cin >> tmp;
	eid = tmp;
	cout << "Enter value (uint8)";
	cin >> tmp;
	value = tmp;
	
	// Search for the current state and check for matching conditions
	for(uint8_t i = 0;i < (sizeof(table)/sizeof(Transition));i++) {
		if((table[i].fromState == curState) && (eval(table[i].cond, ip, eid, value))) {
			// Found match
			cout << "Executing function number " << (int)table[i].action << endl;
			if(testfunction((int)table[i].action) == 0)
      {
			  cout << "New State = Cool State = " << (int)table[i].toCool << endl;
      } else {
        cout << "New State = Uncool State = " << (int)table[i].toUncool << endl;
      }
			return 0;
		}
	}
	cout << "No match found" << endl;
	return 0;
}

