#define WIN32_LEAN_AND_MEAN 
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <string>
#include <chrono>
#include <random>
#include <conio.h>
#include <Windows.h>


#define WHITE   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define HLGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define HLRED   FOREGROUND_RED | FOREGROUND_INTENSITY
#define HLBLUE  FOREGROUND_BLUE | FOREGROUND_INTENSITY


#define _CHECKERROR 1 // Ativa função CheckForError
#include "CheckForError.h"

// Definindo o limite de mensagens na lista circular
const int MAX_MESSAGES = 200;
// Definindo o limite de mensagens no arquivo
const int MAX_ARQ_MESSAGES = 100;

typedef unsigned (WINAPI* CAST_FUNCTION)(LPVOID);
typedef unsigned* CAST_LPDWORD;

#define ESC 27

using namespace std;

// Estrutura para representar mensagens de dados de processo do CLP
struct DataMessage {
    int NSEQ;
    int ORIGEM;
    double VEL;
    int SENSOR_IC;
    int SENSOR_FC;
    string TIMESTAMP;
};

// Estrutura para representar mensagens de alarmes da BALANÇA
struct AlarmMessage {
    int NSEQ;
    int ORIGEM;
    int CODIGO;
    string DESC;
    string TIMESTAMP;
};

// Lista circular em memória para mensagens de dados de processo
list<DataMessage> processDataMessages;
mutex processDataMutex;

// Lista circular em memória para mensagens de alarmes
list<AlarmMessage> alarmMessages;
mutex alarmMutex;

// Eventos para sincronização
HANDLE dataMessageEvent;
HANDLE processDataEvent;
HANDLE alarmEvent;
HANDLE eventESC;

// Semáforos
HANDLE dataMessageSemaphore;
HANDLE processSemaphore;
HANDLE alarmSemaphore;

HANDLE objects[2] = { dataMessageSemaphore, alarmSemaphore };

// Mutexes
HANDLE processDataMutexHandle;
HANDLE alarmMutexHandle;
HANDLE txt;

//Pipes
HANDLE ProcReadPipe;
HANDLE ProcWritePipe;
HANDLE AlarmReadPipe;
HANDLE AlarmWritePipe;

//Arquivos
HANDLE hFile;

// Variáveis dos processos 
HANDLE processos[4];
STARTUPINFO si[2];
PROCESS_INFORMATION pi[2];

// Variável de controle para terminar threads
volatile bool keepRunning = true;

// Função para definir cor do texto no terminal
void SetConsoleColor(WORD color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

// Função para gerar timestamp atual
string getCurrentTimestamp() {
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &in_time_t);
    char timestamp[9];
    strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &buf);
    return string(timestamp);
}

// Função para retornar estado dos sensores
string sensorState(int sensorValue) {
    return sensorValue == 1 ? "Ligado" : "Desligado";
}

// Função para gerar alarmes aleatórios
AlarmMessage generateRandomAlarm() {
    AlarmMessage alarm;
    static int NSEQ = 0;
    alarm.NSEQ = NSEQ++;
    alarm.ORIGEM = (rand() % 2) * 99;  // 1 ou 2
    alarm.CODIGO = rand() % 15 + 101; // 101 a 115
    alarm.DESC = ""; // Descrição será preenchida abaixo
    alarm.TIMESTAMP = getCurrentTimestamp();

    switch (alarm.CODIGO) {
    case 101:
        alarm.DESC = "Velocidade do vagao abaixo do limite minimo.";
        break;
    case 102:
        alarm.DESC = "Velocidade do vagao acima do limite maximo.";
        break;
    case 103:
        alarm.DESC = "Falha no sensor de inicio de carregamento.";
        break;
    case 104:
        alarm.DESC = "Falha no sensor de final de carregamento.";
        break;
    case 105:
        alarm.DESC = "Sobrecarga no vagao.";
        break;
    case 106:
        alarm.DESC = "Falha na comunicacao com o CLP.";
        break;
    case 107:
        alarm.DESC = "Sistema de freio do vagao acionado durante o carregamento.";
        break;
    case 108:
        alarm.DESC = "Nivel de minerio no vagao abaixo do minimo aceitavel.";
        break;
    case 109:
        alarm.DESC = "Nivel de minerio no vagao acima do maximo aceitavel.";
        break;
    case 110:
        alarm.DESC = "Falha no sistema de pesagem da balanca.";
        break;
    case 111:
        alarm.DESC = "Falha no sistema de controle de carga.";
        break;
    case 112:
        alarm.DESC = "Tentativa de carregamento sem vagao posicionado corretamente.";
        break;
    case 113:
        alarm.DESC = "Falha no sistema de liberacao do vagao apos o carregamento.";
        break;
    case 114:
        alarm.DESC = "Interferencia externa detectada durante o carregamento.";
        break;
    case 115:
        alarm.DESC = "Falha no sistema de acionamento da esteira de carregamento.";
        break;
    }

    return alarm;
}

// Função para escrever as mensagens de dados de processo no arquivo
void writeProcessoToFile(const DataMessage& message) {
    //cout << "writeProcessoToFile started" << endl;
    string formattedMessage = message.TIMESTAMP + " NSEQ: " + to_string(message.NSEQ) + " VEL: " +
        to_string(message.VEL) + " SENSOR IC: " + to_string(message.SENSOR_IC) + " SENSOR FC: " +
        to_string(message.SENSOR_FC) + "\n";

    DWORD bytesWritten;
    WriteFile(hFile, formattedMessage.c_str(), formattedMessage.size(), &bytesWritten, NULL);
}


// Função para a tarefa de leitura do CLP de processo
void ReadProcessCLP() {
    int NSEQ = 0;
    while (keepRunning) {
        if (WaitForSingleObject(eventESC, 500) == WAIT_OBJECT_0) break;
        DataMessage message;
        message.NSEQ = NSEQ++;
        message.ORIGEM = 99;
        message.VEL = 23.00 + (rand() % 100) / 100.0;
        message.SENSOR_IC = rand() % 2;
        message.SENSOR_FC = 1 - message.SENSOR_IC;
        message.TIMESTAMP = getCurrentTimestamp();

        {
            lock_guard<mutex> lock(processDataMutex);
            if (processDataMessages.size() >= MAX_MESSAGES) {
                processDataMessages.pop_front();
            }
            processDataMessages.push_back(message);
        }
        ReleaseSemaphore(dataMessageSemaphore, 1, NULL);
    }
}

// Função para a tarefa de leitura do CLP de monitoração de alarmes
void ReadAlarmCLP(int id) {
    int NSEQ = 0;
    while (keepRunning) {
        random_device rd;
        default_random_engine generator(rd());
        uniform_int_distribution<int> distribution(1, 5);
        int randomPeriod = distribution(generator);
        if (WaitForSingleObject(eventESC, randomPeriod) == WAIT_OBJECT_0) break;
        AlarmMessage alarm = generateRandomAlarm();

        {
            lock_guard<mutex> lock(alarmMutex);
            if (alarmMessages.size() >= MAX_MESSAGES) {
                alarmMessages.pop_front();
            }
            alarmMessages.push_back(alarm);
        }
        ReleaseSemaphore(alarmSemaphore, 1, NULL);
    }
}

// Função para a tarefa de retirada de mensagens
void ProcessMessages() {
    //cout << "ProcessMessages started" << endl;
    while (keepRunning) {
        DWORD result = WaitForMultipleObjects(2, objects, FALSE, INFINITE);
        if (result == WAIT_OBJECT_0);
        if (true) {
            DataMessage dataMessage;
            {
                lock_guard<mutex> lock(processDataMutex);
                if (!processDataMessages.empty()) {
                    //cout << "ProcessMessages 2" << endl;
                    dataMessage = processDataMessages.front();
                    processDataMessages.pop_front();
                }
            }

            // Escrever a última mensagem no arquivo
            writeProcessoToFile(dataMessage);
            //cout << "ProcessMessages 3" << endl;

            // Envie a mensagem de dados de processo para o processo filho
            DWORD bytesWritten;
            WriteFile(ProcWritePipe, &dataMessage, sizeof(dataMessage), &bytesWritten, NULL);
            //SetEvent(dataMessageEvent);
            //cout << "ProcessMessages 4" << endl;
        }
        else if (result == (WAIT_OBJECT_0 + 1));
        if (true){
            AlarmMessage alarmMessage;
    
            {
                //cout << "ProcessMessages 5" << endl;
                lock_guard<mutex> lock(alarmMutex);
                if (!alarmMessages.empty()) {
                    alarmMessage = alarmMessages.front();
                    alarmMessages.pop_front();
                }
            }

            // Envie a mensagem de alarme para o processo filho
            DWORD bytesWritten;
            WriteFile(AlarmWritePipe, &alarmMessage, sizeof(alarmMessage), &bytesWritten, NULL);
            //SetEvent(alarmEvent);
            //cout << "ProcessMessages 6" << endl;
        }
    }
}

// Função para a tarefa de exibição de dados de processo
void DisplayData() {
    while (keepRunning) {
        // Aguarda a sinalização do evento ou o evento ESC
        DWORD result = WaitForMultipleObjects(2, new HANDLE[2]{ dataMessageEvent, eventESC }, FALSE, INFINITE);

        // Verifica qual evento foi sinalizado
        if (result == WAIT_OBJECT_0) {  // dataMessageEvent sinalizado
            lock_guard<mutex> lock(processDataMutex);
            for (const auto& message : processDataMessages) {
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLBLUE);
                cout << message.TIMESTAMP << " NSEQ: " << message.NSEQ << " VEL: " << message.VEL
                    << " SENSOR IC: " << sensorState(message.SENSOR_IC) << " SENSOR FC: " << sensorState(message.SENSOR_FC) << endl;
                ReleaseMutex(txt);
            }
            ResetEvent(dataMessageEvent); // Reseta o evento para esperar pelo próximo sinal
        }
        else if (result == WAIT_OBJECT_0 + 1) { // eventESC sinalizado
            break; // Sai do loop quando o evento ESC é sinalizado
        }
    }
}

// Função para a tarefa de exibição de alarmes
void DisplayAlarms() {
    while (1) {
        // Aguarda a sinalização do evento ou o evento ESC
        DWORD result = WaitForMultipleObjects(2, new HANDLE[2]{ alarmEvent, eventESC }, FALSE, INFINITE);

        // Verifica qual evento foi sinalizado
        if (result == WAIT_OBJECT_0) {  // alarmEvent sinalizado
            lock_guard<mutex> lock(alarmMutex);
            for (const auto& alarm : alarmMessages) {
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLRED);
                cout << alarm.TIMESTAMP << " NSEQ: " << alarm.NSEQ << " ORIGEM: " << alarm.ORIGEM
                    << " " << alarm.CODIGO << " " << alarm.DESC << endl;
                ReleaseMutex(txt);
            }
            ResetEvent(alarmEvent); // Reseta o evento para esperar pelo próximo sinal
        }
        else if (result == WAIT_OBJECT_0 + 1) { // eventESC sinalizado
            break; // Sai do loop quando o evento ESC é sinalizado
        }
    }
}

// Função para a tarefa de leitura do teclado
void ReadKeyboard() {
    char userInput;
    bool dataMessageEventState = false;
    bool alarmEventState = false;
    bool processDataEventState = false;

    do {
        userInput = _getch();
        switch (userInput) {
        case 'b':
            // Alterna o estado da tarefa de leitura da balança
            dataMessageEventState = !dataMessageEventState;
            if (dataMessageEventState) {
                SetEvent(dataMessageEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Leitura da balanca: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(dataMessageEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Leitura da balanca: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case 'c':
            // Alterna o estado da tarefa de leitura do CLP
            dataMessageEventState = !dataMessageEventState;
            if (dataMessageEventState) {
                SetEvent(dataMessageEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Leitura do CLP: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(dataMessageEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Leitura do CLP: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case 'a':
            // Alterna o estado da tarefa de leitura da balança de monitoração de alarmes
            alarmEventState = !alarmEventState;
            if (alarmEventState) {
                SetEvent(alarmEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Leitura de alarmes da balanca: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(alarmEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Leitura de alarmes da balanca: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case 'd':
            // Alterna o estado da tarefa de leitura do CLP de monitoração de alarmes
            alarmEventState = !alarmEventState;
            if (alarmEventState) {
                SetEvent(alarmEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Leitura de alarmes do CLP: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(alarmEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Leitura de alarmes do CLP: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case '1':
            // Alterna o estado da tarefa de exibição de dados de processo
            processDataEventState = !processDataEventState;
            if (processDataEventState) {
                SetEvent(processDataEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Exibicao de dados de processo: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(processDataEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Exibicao de dados de processo: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case '2':
            // Alterna o estado da tarefa de exibição de alarmes
            processDataEventState = !processDataEventState;
            if (processDataEventState) {
                SetEvent(processDataEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(HLGREEN);
                cout << "Exibicao de alarmes: ON" << endl;
                ReleaseMutex(txt);
            }
            else {
                ResetEvent(processDataEvent);
                WaitForSingleObject(txt, INFINITE);
                SetConsoleColor(WHITE);
                cout << "Exibicao de alarmes: OFF" << endl;
                ReleaseMutex(txt);
            }
            break;
        case ESC:  // Tecla ESC (código ASCII)
            // Sinalize todas as tarefas para encerrar
            keepRunning = false;
            SetEvent(eventESC);
            break;
        default:
            WaitForSingleObject(txt, INFINITE);
            SetConsoleColor(WHITE);
            cout << "Comando invalido." << endl;
            ReleaseMutex(txt);
        }
    } while (userInput != ESC);
}


int main() {
    BOOL bStatus;
    // Inicialize todas as estruturas de dados, mutexes, semáforos, eventos e outros objetos

    // Nomes para os objetos de sincronização
    LPCWSTR nameEvent[4] = { L"Message_Event", L"Process_Event", L"Alarm_Event", L"Finishing" };
    LPCWSTR nameSemaphore[3] = { L"Message_Semaphore", L"Process_Semaphore", L"Alarm_Semaphore" };
    LPCWSTR nameMutex[3] = { L"Data_MutexHandle", L"Alarm_MutexHandle", L"txt_MutexHandle"};

    // Eventos
    dataMessageEvent = CreateEvent(NULL, FALSE, FALSE, nameEvent[0]);
    processDataEvent = CreateEvent(NULL, FALSE, FALSE, nameEvent[1]);
    alarmEvent = CreateEvent(NULL, FALSE, FALSE, nameEvent[2]);
    eventESC = CreateEvent(NULL, TRUE, FALSE, nameEvent[3]); // reset manual

    // Semáforos
    dataMessageSemaphore = CreateSemaphore(NULL, 0, 1, nameSemaphore[0]);
    processSemaphore = CreateSemaphore(NULL, 0, 1, nameSemaphore[1]);
    alarmSemaphore = CreateSemaphore(NULL, 0, 1, nameSemaphore[2]);

    // Mutexes
    processDataMutexHandle = CreateMutex(NULL, FALSE, nameMutex[0]);
    alarmMutexHandle = CreateMutex(NULL, FALSE, nameMutex[1]);
    txt = CreateMutex(NULL, FALSE, nameMutex[2]);

    // Pipes
    bStatus = CreatePipe(&ProcReadPipe, &ProcWritePipe, NULL, 0);
    if (!bStatus) {
        cout << "Error creating process pipe. Error code: " << GetLastError() << endl;
        return 1;
    }

    bStatus = CreatePipe(&AlarmReadPipe, &AlarmWritePipe, NULL, 0);
    if (!bStatus) {
        cout << "Error creating alarm pipe. Error code: " << GetLastError() << endl;
        return 1;
    }

    // Arquivos
    LPCWSTR fileName = L"processo.txt";
    hFile = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        printf("Falha ao criar/abrir o arquivo. Código de erro: %lu\n", GetLastError());
        return 1;
    }

    // Definir a posição de escrita no final do arquivo
    SetFilePointer(hFile, 0, NULL, FILE_END);

    // Variáveis para detectar os caminhos dos arquivos de execução para a criação dos consoles dos dados do processo e do alarme
    WCHAR console_principal[200];
    WCHAR unidade[10];
    WCHAR caminho[200];
    WCHAR console_secundario[2][200];
    BOOL status;

    GetModuleFileName(NULL, console_principal, 200);
    _wsplitpath_s(console_principal, unidade, 10, caminho, 200, NULL, 0, NULL, 0);

    swprintf_s(console_secundario[0], L"%s%s%s", unidade, caminho, L"videoAlarme.exe");
    swprintf_s(console_secundario[1], L"%s%s%s", unidade, caminho, L"videoProcesso.exe");

    // Criação dos processos
    for (int i = 0; i < 2; i++) {
        ZeroMemory(&si[i], sizeof(si[i]));
        si[i].cb = sizeof(si[i]);
        si[i].dwFlags |= STARTF_USESTDHANDLES;

        // Redirect STD_INPUT_HANDLE to the correct pipe for each process
        /*if (i == 0) {
            // For videoProcesso.exe
            si[i].hStdInput = ProcReadPipe;
        }
        else {
            // For videoAlarme.exe
            si[i].hStdInput = AlarmReadPipe;
        }*/

        ZeroMemory(&pi[i], sizeof(pi[i]));

        // Start the child process.
        status = CreateProcess(console_secundario[i],
            NULL,        // Command line
            NULL,        // Process handle not inheritable
            NULL,        // Thread handle not inheritable
            TRUE,        // Set handle inheritance to TRUE
            CREATE_NEW_CONSOLE, // Create a New Console
            NULL,        // Use parent's environment block
            caminho,     // Use parent's starting directory
            &si[i],      // Pointer to STARTUPINFO structure
            &pi[i]);     // Pointer to PROCESS_INFORMATION structure
        if (!status) {
            printf("CreateProcess failed (%d).\n", GetLastError());
        }
        processos[i] = pi[i].hProcess; // Handle do Processo
        processos[i + 2] = pi[i].hThread; // Handle da Thread do processo
    }

    // Esperando os processos terminarem
    status = WaitForMultipleObjects(2, processos, TRUE, INFINITE);

    // Crie threads para as tarefas
    thread processCLP(ReadProcessCLP);
    thread alarmBAL(ReadAlarmCLP, 1);
    thread alarmCLP(ReadAlarmCLP, 2);
    thread processMessages(ProcessMessages);
    thread displayData(DisplayData);
    thread displayAlarms(DisplayAlarms);
    thread readKeyboard(ReadKeyboard);

    // Aguarde o término das threads
    processCLP.join();
    alarmBAL.join();
    alarmCLP.join();
    processMessages.join();
    displayData.join();
    displayAlarms.join();
    readKeyboard.join();

    cout << "Todas as threads encerradas." << endl;

    // Fechar handles
    CloseHandle(dataMessageEvent);
    CloseHandle(processDataEvent);
    CloseHandle(alarmEvent);
    CloseHandle(eventESC);
    CloseHandle(dataMessageSemaphore);
    CloseHandle(processSemaphore);
    CloseHandle(alarmSemaphore);
    CloseHandle(processDataMutexHandle);
    CloseHandle(alarmMutexHandle);
    CloseHandle(txt);
    CloseHandle(ProcReadPipe);
    CloseHandle(ProcWritePipe);
    CloseHandle(AlarmReadPipe);
    CloseHandle(AlarmWritePipe);
    CloseHandle(hFile);

    return 0;
}

