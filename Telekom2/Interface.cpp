#include <iostream>
#include "Interface.h"
#include "Port.h"

using namespace std;

void startGo() {

	cout << "\t-- XMODEM PROTOCOL --\n";
	cout << "Mateusz Michalec i Bartlomiej Motyl \n\n";
	Port port = Port();
	bool doEnd = false;
	bool isCRC;
	while (!doEnd) {
		system("CLS");
		cout << "Wybierz tryb dzialania\n\n1.\tTest komunikacji\n2.\tOdbiornik\n3.\tNadajnik\n0.\tZakoncz\n\n";
		int choice;
		cin >> choice;

		switch (choice)
		{
		case 1:
		{
			port.communicationTest();
			break;
		}
		case 2:
		{
			int checksum;
			cout << "Wybierz rodzaj kontroli bledow\n\n1.\tCRC (C)\n2.\tChecksum (NAK)\n";
			cin >> checksum;
			if (checksum == 1) isCRC = true;
			else isCRC = false;

			port.receiver(isCRC);
			break;
		}
		case 3:
		{
			port.transmitter();
			break;
		}
		case 0:
		{
			doEnd = true;
			break;
		}

		default:
			break;
		}
	}

}