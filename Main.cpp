#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <process.h> 
#include <conio.h>
#include <math.h>
#include "formula.h"
#define PI 3.1415

HANDLE processos[4];
STARTUPINFO si[2];
PROCESS_INFORMATION pi[2];

//void volume_cilindro(float raio, float altura);


int main() {

	WCHAR console_principal[200];
	WCHAR unidade[10];
	WCHAR caminho[200];
	WCHAR console_secundario[2][200];
	BOOL status;



	//Esta função capta o caminho do console gerado pela execução do programa "Main.cpp"
	   //para determinar o caminho que leva a criação dos consoles que irão exibir os dados do processo e os alarmes
	GetModuleFileName(NULL, console_principal, 200);
	_wsplitpath_s(console_principal, unidade, 10, caminho, 200, NULL, 0, NULL, 0);

	swprintf_s(console_secundario[0], L"%s%s%s", unidade, caminho, L"videoAlarme.exe");
	swprintf_s(console_secundario[1], L"%s%s%s", unidade, caminho, L"videoProcesso.exe");

	//Criação dos processos
	for (int i = 0; i < 2; i++) {
		ZeroMemory(&si[i], sizeof(si[i]));
		si[i].cb = sizeof(si[i]);
		ZeroMemory(&pi[i], sizeof(pi[i]));

		// Start the child process.
		status = CreateProcess(console_secundario[i],
			NULL,        // Command line
			NULL,        // Process handle not inheritable
			NULL,        // Thread handle not inheritable
			FALSE,       // Set handle inheritance to FALSE
			CREATE_NEW_CONSOLE, // Create a New Console
			NULL,        // Use parent's environment block
			caminho,   // Use parent's starting directory
			&si[i],      // Pointer to STARTUPINFO structure
			&pi[i]);     // Pointer to PROCESS_INFORMATION structure
		if (!status)
		{
			printf("CreateProcess failed (%d).\n", GetLastError());
		}
		processos[i] = pi[i].hProcess; // Handle do Processo
		processos[i + 2] = pi[i].hThread; // Handle da Thread do processo
	}


	printf("\nCONSOLE PRINCIPAL\n");

	//Esperando os processos terminarem
	status = WaitForMultipleObjects(2, processos, TRUE, INFINITE);


	// Fechando os Handles dos processos e das threads dos processos
	/*for (INT i = 0; i < 4; i++) {
		CloseHandle(processos[i]);
	}*/

	volume_cilindro(4.0, 2.0);
	return 0;

}
