#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <process.h> 
#include <string>
#include <iostream>
#include <conio.h>
#include "..\\TP_ATR_etapa1\\processMessage.cpp"

using namespace std;

struct DataMessage {
    int NSEQ;
    int ORIGEM;
    double VEL;
    int SENSOR_IC;
    int SENSOR_FC;
    string TIMESTAMP;
};

int main() {
    HANDLE hReadPipe;
    hReadPipe = GetStdHandle(STD_INPUT_HANDLE);

    DataMessage message;
    DWORD bytesRead;

    while (ReadFile(hReadPipe, &message, sizeof(message), &bytesRead, NULL) && bytesRead > 0) {
        cout << message.TIMESTAMP << " NSEQ: " << message.NSEQ << " VEL: " << message.VEL
            << " SENSOR IC: " << message.SENSOR_IC << " SENSOR FC: " << message.SENSOR_FC << endl;
    }

    return 0;
}