#include "mptree.h"
using namespace std;
int main() {
	MPTree TEST;
	TEST.Insert("a711355","cat");
	TEST.Insert("a77d337","dog");
	TEST.Insert("a7f9365","pig");
	TEST.Insert("a77d397","duck");

	TEST.Print();

	TEST.Delete("a77d397");
	TEST.Print();

	cout<<TEST.Get("a77d397")<<endl;
	cout<<TEST.Get("a711355")<<endl;
	return 0;
}