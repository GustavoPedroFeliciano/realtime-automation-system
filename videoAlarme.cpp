#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <string>
#include <iostream>
#include <process.h> 
#include <conio.h>
#include "..\\TP_ATR_etapa1\\alarmMessage.cpp"


using namespace std;

struct AlarmMessage {
    int NSEQ;
    int ORIGEM;
    int CODIGO;
    string DESC;
    string TIMESTAMP;
};

int main() {
    HANDLE hReadPipe;
    hReadPipe = GetStdHandle(STD_INPUT_HANDLE);

    AlarmMessage alarm;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, &alarm, sizeof(alarm), &bytesRead, NULL) && bytesRead > 0) {
        cout << alarm.TIMESTAMP << " NSEQ: " << alarm.NSEQ << " ORIGEM: " << alarm.ORIGEM
            << " " << alarm.CODIGO << " " << alarm.DESC << endl;
    }

    return 0;
}