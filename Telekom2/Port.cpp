#include "Port.h"
    // std::cin, std::cout


char filename[255];
ofstream file;
ifstream file2;
char dataBlock[128];
char first3Bytes[3];
char dopelnienieDo255;
char sign;
unsigned long sizeOfSign = sizeof(sign);
int numOfSigns = 1;      //potrzebne przy czytaniu i pisaniu
bool connection = false;
bool isCorrectPacket;
int packetNum;
char errorControlSum[2];      //odebrane sumaKontrolnaCRC
USHORT tmpCRC;
bool isCRC;
int code;

const char SOH_ = 0x01; // Start Of Heading
const char NAK_ = 0x15; // Negative Acknowledge
const char CAN_ = 0x18; // Flaga do przerwania połączenia (24?) cancel
const char ACK_ = 0x06; // Zgoda na rzesylanie danych           acknowledge
const char EOT_ = 0x04; // End Of Transmission
const char C_ = 0x43;


Port::Port() {
	openPort();

	if (handle != INVALID_HANDLE_VALUE)
	{
		setupPort.DCBlength = sizeof(setupPort);
		GetCommState(handle, &setupPort);
		setupPort.BaudRate = CBR_9600;     // predkosc transmisji
		setupPort.Parity = NOPARITY;       // bez bitu parzystosci
		setupPort.StopBits = ONESTOPBIT;    // ustawienie bitu stopu (jeden bit)
		setupPort.ByteSize = 8;       // liczba wysylanych bitow

		setupPort.fParity = TRUE;
		setupPort.fDtrControl = DTR_CONTROL_DISABLE; //Kontrola linii DTR: DTR_CONTROL_DISABLE (sygnal nieaktywny)
		setupPort.fRtsControl = RTS_CONTROL_DISABLE; //Kontrola linii RTR: DTR_CONTROL_DISABLE (sygnal nieaktywny)
		setupPort.fOutxCtsFlow = FALSE;
		setupPort.fOutxDsrFlow = FALSE;
		setupPort.fDsrSensitivity = FALSE;
		setupPort.fAbortOnError = FALSE;
		setupPort.fOutX = FALSE;
		setupPort.fInX = FALSE;
		setupPort.fErrorChar = FALSE;
		setupPort.fNull = FALSE;

		timeouts.ReadIntervalTimeout = 10000;
		timeouts.ReadTotalTimeoutMultiplier = 10000;
		timeouts.ReadTotalTimeoutConstant = 10000;
		timeouts.WriteTotalTimeoutMultiplier = 100;
		timeouts.WriteTotalTimeoutConstant = 100;

		SetCommState(handle, &setupPort);
		SetCommTimeouts(handle, &timeouts);
		ClearCommError(handle, &error, &portResources);
	}
	else {
		cout << "Nieudane polaczenie. Sprawdz czy port istnieje lub nie jest juz zajety\n";
		exit(1);
	}
}



void Port::openPort()
{
	//1. Otwieranie portu, 
    //1.1 trzeba przekazac LPCTSTR do funkcji z WinAPI, która pozwoli "przejac port".
	string passedComNumber;
	cout << "Podaj numer portu COM (np. COM1)\n";
	std::cin >> passedComNumber;
    
    for (int i = 0; i < passedComNumber.length(); i++) {
        putchar(toupper(passedComNumber[i]));
    };
    cout << "\n";

    //1.2 Zamiana imputu na Long Pointer Constant String
	std::basic_string<TCHAR> converted(passedComNumber.begin(), passedComNumber.end());
	const TCHAR* comString = converted.c_str();
    //1.3 tak przez WinApi tworzy się połączenie
	handle = CreateFile(comString, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
}

void Port::communicationTest()
{
	string text;
	cout << "Podaj tekst ktory chcesz wyswietlic w HyperTerminalu:\n";
	cin >> text;
	for (int i = 0; i<text.length(); i++) {
		WriteFile(handle, &text.at(i), numOfSigns, &sizeOfSign, NULL);
	}
}

void Port::readCOM(char* sign) {
    ReadFile(handle, sign, numOfSigns, &sizeOfSign, NULL);
 }

void Port::readCOM(char* sign,int lenght) {
    DWORD pos = 0, num;
    while (pos < lenght) {
        ReadFile(handle, sign + pos, lenght - pos, &num, NULL);
        pos += num;
    }
}

void Port::writeCOM(char* sign, int length) {
    DWORD num;
    WriteFile(handle, sign, length, &num, NULL);
}

//================================================================================================================================//
int calculateCRC(char* wsk, int count)
{
    int CRCchecksum = 0;

    while (--count >= 0)
    {
        CRCchecksum = CRCchecksum ^ (int)*wsk++ << 8; 								 // wez sign i dopisz osiem zer
        for (int i = 0; i < 8; ++i)
            if (CRCchecksum & 0x8000) CRCchecksum = CRCchecksum << 1 ^ 0x1021; // jezli lewy bit == 1 wykonuj XOR generatorm 1021
            else CRCchecksum = CRCchecksum << 1; 									 // jezli nie to XOR przez 0000, czyli przez to samo
    }
    return (CRCchecksum & 0xFFFF);
}

int calculateChecksum(char* wsk) {
    int checksum = 0;
    for (int i = 0; i < 128; i++) {
        checksum += (unsigned char)wsk[i];
    }
    checksum %= 256;
    return checksum;
}

int isEven(int x, int y)
{
    if (y == 0) return 1;
    if (y == 1) return x;

    int wynik = x;

    for (int i = 2; i <= y; i++)
        wynik = wynik * x;

    return wynik;
}


char calculateSymbolCRC(int n, int signNum) //przeliczanie CRC na postac binarna
{
    int x, binarna[16];

    for (int z = 0; z < 16; z++) binarna[z] = 0;

    for (int i = 0; i < 16; i++)
    {
        x = n % 2;
        if (x == 1) n = (n - 1) / 2;
        if (x == 0) n = n / 2;
        binarna[15 - i] = x;
    }

    //obliczamy poszczegolne signi sumaKontrolnaCRC (1-szy lub 2-gi)
    x = 0;
    int k;

    if (signNum == 1) k = 7;
    if (signNum == 2) k = 15;

    for (int i = 0; i < 8; i++)
        x = x + isEven(2, i) * binarna[k - i];

    return (char)x;//zwraca 1 lub 2 chary (bo 2 chary to 2 bajty, czyli 16 bitów)
}


void Port::receiver(bool withCRC)
{

    cout << "Podaj nazwe pliku\n";
    std::cin >> filename;

    if (!withCRC) {
        sign = NAK_;
        isCRC = false;
    }
    else {
        sign = C_;
        isCRC = true;
    }

	for (int i = 0; i < 6; i++)
	{
		cout << "\nWysylanie\n";
		//WYSYLANIE NAK/C
        writeCOM(&sign, numOfSigns);
        
		
		cout << "Oczekiwanie\n";
		//Czytanie 3 pierwszych bajtow
        readCOM(first3Bytes,3);
		cout << sign << endl;
		if (first3Bytes[0] == SOH)
		{
			cout << "Połączono\n";
			connection = true;
			break;
		}
	}

	//NIE ODEBRANO SOH
	if (!connection)
	{
		cout << "Nie udało się połączyć\n";
		exit(1);
	}
    
	file.open(filename, ios::binary);
	cout << "ODBIERANIE";

    packetNum = (int)first3Bytes[1];
	dopelnienieDo255 = first3Bytes[2];
    readCOM(dataBlock, 128);

//===================== C R C ============================
    if (withCRC) {
        readCOM(errorControlSum, 2);
        isCorrectPacket = true;

        if ((char)(255 - packetNum) != dopelnienieDo255)
        {
            cout << "Bledny numer pakietu!\n";
            sign = NAK_;
            writeCOM(&sign, numOfSigns);
            isCorrectPacket = false;

        }

        if (isCorrectPacket)
        {
            for (int i = 0; i < 128; i++)
            {
                if (dataBlock[i] != 26)
                    file << dataBlock[i];
            }
            cout << "Przeslano pakiet\n";
            sign = ACK_;
            writeCOM(&sign, numOfSigns);
        }
        // Czytanie pliku do momentu otrzymania signu EOT (end of transmission) albo CAN (canceled)
        while (1)
        {

            //ReadFile(handle, &sign, numOfSigns, &sizeOfSign, NULL);
            readCOM(&sign);
            if (sign == EOT || sign == CAN) break;
            cout << "Odbieranie";

            readCOM(&sign);
            packetNum = (int)sign;

            readCOM(&sign);
            dopelnienieDo255 = sign;

            readCOM(dataBlock,128);
            readCOM(errorControlSum, 2);

            isCorrectPacket = true;
            if ((char)(255 - packetNum) != dopelnienieDo255)
            {
                cout << "Niewlasciwy numer pakietu!\n";
                sign = NAK_;
                writeCOM(&sign, numOfSigns);
                isCorrectPacket = false;
            }
            else
            {
                tmpCRC = calculateCRC(dataBlock, 128);

                if (calculateSymbolCRC(tmpCRC, 1) != errorControlSum[0] || calculateSymbolCRC(tmpCRC, 2) != errorControlSum[1])
                {
                    cout << "Bledna suma kontrolna!\n";
                    sign = NAK_;
                    writeCOM(&sign, numOfSigns);
                    isCorrectPacket = false;
                }
            }
            if (isCorrectPacket)
            {
                for (int i = 0; i < 128; i++)
                {
                    if (dataBlock[i] != 26)
                        file << dataBlock[i];
                }

                cout << "Przeslanie pakietu zakonczone powodzeniem!\n";
                sign = ACK_;
                writeCOM(&sign, numOfSigns);
            }
        }
    }

    // ================== CHECKSUM ============================
    else {

        readCOM(errorControlSum,1);
        isCorrectPacket = true;


        if ((char)(255 - packetNum) != dopelnienieDo255)
        {
            cout << "Bledny numer pakietu!\n";
            sign = NAK_;
            writeCOM(&sign, numOfSigns);
            isCorrectPacket = false;

        }
 
        if (isCorrectPacket)
        {
            for (int i = 0; i < 128; i++)
            {
                if (dataBlock[i] != 26)
                    file << dataBlock[i];
            }
            cout << "Przeslano pakiet\n";
            WriteFile(handle, &ACK_, numOfSigns, &sizeOfSign, NULL);
        }
        // Czytanie pliku do momentu otrzymania signu EOT (end of transmission) albo CAN (canceled)
        while (1)
        {
            readCOM(&sign);
            if (sign == EOT || sign == CAN) break;
            cout << "Odbieranie";

            readCOM(&sign);
            packetNum = (int)sign;

            readCOM(&sign);
            dopelnienieDo255 = sign;

            readCOM(dataBlock, 128);


            readCOM(&sign);
            errorControlSum[0] = sign;


            isCorrectPacket = true;
            if ((char)(255 - packetNum) != dopelnienieDo255)
            {
                cout << "Niewlasciwy numer pakietu!\n";
                sign = NAK_;
                writeCOM(&sign, numOfSigns);
                isCorrectPacket = false;
            }
            else
            {
                tmpCRC = calculateChecksum(dataBlock);

                if (calculateSymbolCRC(tmpCRC, 1) != errorControlSum[0] )
                {
                    cout << "Bledna suma kontrolna!\n";
                    sign = NAK_;
                    writeCOM(&sign, numOfSigns);
                    isCorrectPacket = false;
                }
            }
            if (isCorrectPacket)
            {
                for (int i = 0; i < 128; i++)
                {
                    if (dataBlock[i] != 26)
                        file << dataBlock[i];
                }

                cout << "Przeslanie pakietu zakonczone powodzeniem!\n";
                sign = ACK_;
                writeCOM(&sign, numOfSigns);
            }
        }


    }
    sign = ACK_;
    writeCOM(&sign, numOfSigns);

    file.close();
    //CloseHandle(handle);

    if (sign == CAN) cout << "ERROR - polaczenie zostalo przerwane! \n";
    else cout << "Odebrano plik";
    std::cin.get();
}

void Port::transmitter() {
    packetNum = 1;
    cout << "Podaj nazwe pliku ktory chcesz wyslac\n";
    std::cin >> filename;

    cout << "\nOczekiwanie na rozpoczecie transmisji...\n";
    for (int i = 0; i < 6; i++) {
        readCOM(&sign, numOfSigns);
            
        //Sprawdzanie czy odbiornik uzyje CRC czy Checksum
        if (sign == C_) {
            cout << "\n Otrzymano znak C (CRC) \n" << sign << endl;
            Sleep(5000);
            isCRC = true;
            connection = true;
            break;
        }
        else if (sign == NAK_) {
            cout << "\n Otrzymano znak NAK (Checksum) \n" << sign << endl;
            Sleep(5000);
            isCRC = false;
            connection = true;
            break;
        }
    }
    if (!connection) exit(1);

    file2.open(filename, ios::binary);
    while (!file2.eof())
    {
        //tablica do czyszczenia, pozbywa sie SUBSUBSUB<<<
        for (int i = 0; i < 128; i++)
            dataBlock[i] = (char)26;

        //tworzenie datablocku
        int x = 0;
        while (x < 128 && !file2.eof())
        {
            dataBlock[x] = file2.get();
            if (file2.eof()) dataBlock[x] = (char)26; //dopełnianie bloku danych SUB'em, pozbywa sie znaku Y na koncuy
            x++;
        }
        isCorrectPacket = false;

        while (!isCorrectPacket)
        {
            cout << "Trwa wysylanie pakietu. Prosze czekac...\n";
            //Wysylanie 3 pierwszych bajtow
            // wysylanie SOH
            sign = SOH_;
            writeCOM(&sign, numOfSigns);

            // wyslanie numeru paczki
            sign = (char)packetNum;
            writeCOM(&sign, numOfSigns);

            //wyslanie dopelnienia
            sign = (char)255 - packetNum;
            writeCOM(&sign, numOfSigns);


            //wysyłanie bloku danych
            writeCOM(dataBlock, 128);

            if (isCRC == false) //suma kontrolna 1 bajt
            {
                char checksum = NULL;
                checksum += calculateChecksum(dataBlock);
                writeCOM(&checksum, numOfSigns);
            }
            else if (isCRC == true) //CRC 2 bajty
            {
                //nasz wyliczony CRC (2bajtowa - 16 bitowa wartosc)
                tmpCRC = calculateCRC(dataBlock, 128);
                //wysyłanie obliczonego CRC - odbiornik sobie sprawdza
                sign = calculateSymbolCRC(tmpCRC, 1);
                writeCOM(&sign, numOfSigns);
                sign = calculateSymbolCRC(tmpCRC, 2);
                writeCOM(&sign, numOfSigns);
            }


            while (1)
            {
                
                readCOM(&sign);

                if (sign == ACK_)
                {
                    isCorrectPacket = true;
                    cout << "Przeslano poprawnie pakiet danych!";
                    break;
                }
                if (sign == NAK_)
                {
                    cout << "ERROR - otrzymano NAK!\n";
                    break;
                }
                if (sign == CAN_)
                {
                    cout << "ERROR - polaczenie zostalo przerwane!\n";
                    exit (1);
                }
            }
        }
        //zwiekszamy numer pakietu
        if (packetNum == 255) packetNum = 1;
        else packetNum++;

    }
    file2.close();

    while (1)
    {
        //konczenie komunikacji
        sign = EOT_;
        writeCOM(&sign, numOfSigns);
        readCOM(&sign);
        if (sign == ACK_) break;
    }

    cout << "Przesłano plik";
    Sleep(5000);
    
}



Port::~Port() {
	CloseHandle(handle);
}


