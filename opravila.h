/*
 * opravila.h
 *
 *  Created on: Oct 17, 2023
 *      Author: patriciobulic
 */

#ifndef INC_OPRAVILA_H_
#define INC_OPRAVILA_H_

#define NTASKS 4
#define TASK_STACK_SIZE 256


typedef struct{
	unsigned int *sp;			// nazadnje vidseni SP opravila
	void (*pTaskFunction)();	// naslov finkcije, ki implementira opravilo
} TCB_Type;

typedef struct{
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r12;
	unsigned int lr;
	unsigned int pc;
	unsigned int psr;
} HWSF_Type;

typedef struct{
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
} SWSF_Type;


void InicializrajOpravila(unsigned int* pStackRegion, TCB_Type pTCBTable[], void (*TaskFunctions[])());
void UstvariOpravilo(TCB_Type* pTCB, unsigned int* pStackBase, void (*TaskFunction)());
int ZamenjajOpravilo(int current_task, TCB_Type pTCB[]);

void Opravilo1();
void Opravilo2();
void Opravilo3();
void Opravilo4();







// PB:
void CreateTask(TCB_Type* pTCB, unsigned int* pStackBase, void (*TaskFunction)());
void InitializeTasks(unsigned int* pStackRegion, TCB_Type pTCB[], void (*TaskFunctions[])());

int switch_context (int current_task, TCB_Type pTCB[]);



#endif /* INC_OPRAVILA_H_ */
