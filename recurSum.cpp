#include <iostream>

using namespace std;

int recurSum(int X) {
	if (X == 1) {
		return X;
	}
	return X + recurSum(X - 1);
}

int main() {
	int X = 0;
	cout << "Enter X : ";
	cin >> X;
	cout << "The sum of " << X << " natural numbers is : " << recurSum(X) << endl;
	return 0;
}
