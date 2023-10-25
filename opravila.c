/*
 * opravila.c
 *
 *  Created on: Oct 17, 2023
 *      Author: patriciobulic
 */


#include "opravila.h"
#include "stm32h7xx_hal.h"


void InicializrajOpravila(unsigned int* pStackRegion, TCB_Type pTCBTable[], void (*TaskFunctions[])()){

	unsigned int* pStackBase;

	for(int i=0; i<NTASKS; i++){
		pStackBase = pStackRegion + (i+1)*TASK_STACK_SIZE;

		UstvariOpravilo(&pTCBTable[i], pStackBase, TaskFunctions[i]);
	}



}


void UstvariOpravilo(TCB_Type* pTCB, unsigned int* pStackBase, void (*TaskFunction)()){
	HWSF_Type* pHWStackFrame;
	SWSF_Type* pSWStackFrame;

	// Nastavi kazalca na vrh HWSF in SWSF:
	pHWStackFrame = (HWSF_Type*)((void*)pStackBase - sizeof(HWSF_Type));
	pSWStackFrame = (SWSF_Type*)((void*)pHWStackFrame - sizeof(SWSF_Type));

	// napolni HW Stack Frame
	// Začetne vrednosti registrov r0-r3, r12 in lr so načeloma nepomembne
	// izberem take vrednosti, da lažjem preverim v pomnilniški sliki
	pHWStackFrame->r0 	= 0;
	pHWStackFrame->r1	= 0x11111111;
	pHWStackFrame->r2 	= 0x22222222;
	pHWStackFrame->r3 	= 0x33333333;
	pHWStackFrame->r12 	= 0xCCCCCCCC;
	pHWStackFrame->lr 	= 0xFFFFFFFF;	// v našem RTOS-u se taski nikoli ne zaključijo, zato je lr 0xFFFFFFFF (reset val)
	pHWStackFrame->pc 	= (unsigned int)TaskFunction;
	pHWStackFrame->psr 	= 0x01000000; 	// Set T bit (bit 24) in EPSR. The Cortex-M4 processor only
										// supports execution of instructions in Thumb state.
										// Attempting to execute instructions when the T bit is 0 (Debug state)
										// 		results in a fault.
	// napolni SW Stack Frame
	pSWStackFrame->r4	= 0x44444444;
	pSWStackFrame->r5	= 0x55555555;
	pSWStackFrame->r6	= 0x66666666;
	pSWStackFrame->r7	= 0x77777777;
	pSWStackFrame->r8	= 0x88888888;
	pSWStackFrame->r9	= 0x99999999;
	pSWStackFrame->r10	= 0xaaaaaaaa;
	pSWStackFrame->r11	= 0xbbbbbbbb;

	// V TCB opravila nastavi SP in PC:
	pTCB->sp 			= (unsigned int*)pSWStackFrame;
	pTCB->pTaskFunction = TaskFunction;

}


int ZamenjajOpravilo(int current_task, TCB_Type pTCB[]){

	unsigned int* current_sp;

	// če trenutno opravilo ni funkcija main:
	if (current_task != -1){
		//Shrani PSP prekinjenega opravila v njegov TCB:
		//read current SP save it into TCB on the interrupted task:
		__asm__ volatile ( "MRS %0, PSP\n\t"	// assembler instruction: Move PSP to a gen-purpose REG
						: "=r" (current_sp) 	// output operands
						:						// input operands
					 );
		pTCB[current_task].sp = current_sp;
	}

	// izberi naslednje opravilo po vrsti:
	current_task ++;
	if (current_task == NTASKS) current_task = 0;

	// Nastavi nov PSP:
	__asm__ volatile ( "MSR PSP, %0\n\t"			// assembler instruction: load PSP from gen-purpose REG
					:								// output operands
					: "r" (pTCB[current_task].sp) 	// input operands
				 );
	return current_task;
}


void Opravilo1(){
	while(1) {
		__asm__ volatile ( "nop ;"
						   "nop");
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);
	}
}

void Opravilo2(){
	while(1) {
		__asm__ volatile ( "nop ;"
						   "nop");
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);
	}
}

void Opravilo3(){
	while(1) {
		__asm__ volatile ( "nop ;"
						   "nop");
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_RESET);
	}
}

void Opravilo4(){
	while(1) {
		__asm__ volatile ( "nop ;"
						   "nop");
		HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_2, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOI, GPIO_PIN_13, GPIO_PIN_SET);
	}
}











/*
	The function creates a new task. Creating a new task involves:

	1. making space for the task's stack. Each task gets 1kB for the stack
	2. populating the initial HW stack frame for the task. HW stack frame initially resides
	   at the bottom of the task's stack. The HW stack frame is populated as follows:
	   PSR = 0x21000000 - this is the default reset value in the state register
	   PC = address of the task
	   LR = the address of the delete_task() function (if needed). In our case tasks never finish so LR=0xFFFFFFFF (reset value)
	   r12, r3-r0 = 0x00000000 - we may also pass the arguments into the tas via r0-r3, but this is not
	   	the case in our simple RTOS
	3. makes room for the initial uninitialized SW stack frame.
	4. sets the tasks stack pointer in the task table to point at the top of the SW stack frame

	After this steps a new task is ready to be executed for the first time when task switch occurs and
	the task is selected for execution.

 	Pa3cio Bulic, 18.10.2023
*/

void CreateTask(TCB_Type* pTCB, unsigned int* pStackBase, void (*TaskFunction)()){
	HWSF_Type* pHWStackFrame;
	SWSF_Type* pSWStackFrame;


	// Nastavi kazalca na vrh HWSF in SWSF:
	pHWStackFrame = (HWSF_Type*)((void*)pStackBase - sizeof(HWSF_Type));
	pSWStackFrame = (SWSF_Type*)((void*)pHWStackFrame - sizeof(SWSF_Type));


	// napolni HW Stack Frame
	// Začetne vrednosti registrov r0-r3, r12 in lr so načeloma nepomembne
	// izberem take vrednosti, da lažjem preverim v pomnilniški sliki
	pHWStackFrame->r0 	= 0;
	pHWStackFrame->r1	= 0x11111111;
	pHWStackFrame->r2 	= 0x22222222;
	pHWStackFrame->r3 	= 0x33333333;
	pHWStackFrame->r12 	= 0xCCCCCCCC;
	pHWStackFrame->lr 	= 0xFFFFFFFF;	// v našem RTOS-u se taski nikoli ne zaključijo, zato je lr 0xFFFFFFFF (reset val)
	pHWStackFrame->pc 	= (unsigned int)TaskFunction;
	pHWStackFrame->psr 	= 0x01000000; 	// Set T bit (bit 24) in EPSR. The Cortex-M4 processor only
	 	 	 	 	 	 	 	 	 	// supports execution of instructions in Thumb state.
	 	 	 	 	 	 	 	 	 	// Attempting to execute instructions when the T bit is 0 (Debug state)
										// 		results in a fault.
	// napolni SW Stack Frame
	pSWStackFrame->r4	= 0x44444444;
	pSWStackFrame->r5	= 0x55555555;
	pSWStackFrame->r6	= 0x66666666;
	pSWStackFrame->r7	= 0x77777777;
	pSWStackFrame->r8	= 0x88888888;
	pSWStackFrame->r9	= 0x99999999;
	pSWStackFrame->r10	= 0xaaaaaaaa;
	pSWStackFrame->r11	= 0xbbbbbbbb;

	// V TCB opravila nastavi SP in PC:
	pTCB->sp 			= (unsigned int*)pSWStackFrame;
	pTCB->pTaskFunction = TaskFunction;
}



/*
	The function initializes and creates all tasks in the RTOS.
	Our RTOS is very simple and has fixed number of tasks that are created
	at the start of RTOS.
	The tasks never stop and the number of the running task never changes.

 	Pa3cio Bulic, 28.10.2023
*/

void InitializeTasks(unsigned int* pStackRegion, TCB_Type pTCB[], void (*TaskFunctions[])()){
	unsigned int* pStackBase;
	// 1. nastavi kazalce, ki označujejo začetek skladu za vsako opravilo:
	for(int i=0; i<NTASKS; i++){
		pStackBase = pStackRegion + (i+1)*TASK_STACK_SIZE;
		CreateTask(&pTCB[i], pStackBase, TaskFunctions[i]);
	}

	/* PSP še nismo nastavili.
		   PSP naj kaže na vrh sklada prvega opravila */
	__asm__ volatile ( "MSR PSP, %0\n\t"	// assembler instruction: load PSP from gen-purpose REG
					:						// output operands
					: "r" (pTCB->sp) 		// input operands
					);


}


int switch_context (int current_task, TCB_Type pTCB[]) {
	void* current_sp;
    if (current_task != -1) { // samo če nismo prekinili main.

	//read current SP save it into TCB on the interrupted task:
	__asm__ volatile ( "MRS %0, PSP\n\t"	// assembler instruction: Move PSP to a gen-purpose REG
					: "=r" (current_sp) 	// output operands
					:						// input operands
				 );

	pTCB[current_task].sp = current_sp;
    }

	// select a new task in a round-robin fashion:
	current_task++;
	if (current_task == NTASKS) {
          current_task = 0;
	}

	// Nastavi nov PSP:
	__asm__ volatile ( "MSR PSP, %0\n\t"			// assembler instruction: load PSP from gen-purpose REG
					:								// output operands
					: "r" (pTCB[current_task].sp) 	// input operands
				 );

	return current_task;
}




