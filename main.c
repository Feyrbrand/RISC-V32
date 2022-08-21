
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

enum opcode_decode {R = 0x33, I = 0x13, S = 0x23, L = 0x03, B = 0x63, JALR = 0x67, JAL = 0x6F, AUIPC = 0x17, LUI = 0x37};

typedef struct {
    size_t data_mem_size_;
    uint32_t regfile_[32];
    uint32_t pc_;
    uint8_t* instr_mem_;
    uint8_t* data_mem_;
} CPU;

// Functions
//
void CPU_open_instruction_mem(CPU* cpu, const char* filename);
void CPU_load_data_mem(CPU* cpu, const char* filename);
int32_t calcimm(uint32_t instruction, char type);

CPU* CPU_init(const char* path_to_inst_mem, const char* path_to_data_mem) {
	CPU* cpu = (CPU*) malloc(sizeof(CPU));
	cpu->data_mem_size_ = 0x400000;
    cpu->pc_ = 0x0;
    CPU_open_instruction_mem(cpu, path_to_inst_mem);
    CPU_load_data_mem(cpu, path_to_data_mem);
    return cpu;
}

void CPU_open_instruction_mem(CPU* cpu, const char* filename) {
	uint32_t  instr_mem_size;
	FILE* input_file = fopen(filename, "r");
	if (!input_file) {
			printf("no input\n");
			exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1) {
			printf("error stat\n");
			perror("stat");
		    exit(EXIT_FAILURE);
	}
	printf("size of instruction memory: %d Byte\n\n",sb.st_size);
	instr_mem_size =  sb.st_size;
	cpu->instr_mem_ = malloc(instr_mem_size);
	fread(cpu->instr_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}

void CPU_load_data_mem(CPU* cpu, const char* filename) {
	FILE* input_file = fopen(filename, "r");
	if (!input_file) {
			printf("no input\n");
			exit(EXIT_FAILURE);
	}
	struct stat sb;
	if (stat(filename, &sb) == -1) {
			printf("error stat\n");
			perror("stat");
		    exit(EXIT_FAILURE);
	}
	printf("read data for data memory: %d Byte\n\n",sb.st_size);

    cpu->data_mem_ = malloc(cpu->data_mem_size_);
	fread(cpu->data_mem_, sb.st_size, 1, input_file);
	fclose(input_file);
	return;
}

int32_t calcimm(uint32_t instruction, char type){
	int32_t imm;
	switch (type)
	{
	case 'I':
		// calculate Imm for I-type
		//
		if ((instruction & 0x80000000) == 0x80000000){							//check if sign bit is 1
			imm = ((instruction & 0xFFF00000) >> 20) | 0xFFFFF000;				//ckeck/extend with 20sign
		}
		else {
			imm = ((instruction & 0xFFF00000) >> 20);
		}
		return imm;
		break;
	case 'S':
		// calculate Imm for S-type
		//
		if ((instruction & 0x80000000) == 0x80000000){													//check if sign bit is 1
			imm = ((instruction & 0xFE000000) >> 20) | ((instruction & 0xF80) >> 7) | 0xFFFFF000;		//ckeck/extend with 20sign
		}
		else {
			imm = ((instruction & 0xFE000000) >> 20) | ((instruction & 0xF80) >> 7);
		}
		return imm;
		break;
	case 'B':
		// calculate Imm for B-type
		//
		if ((instruction & 0x80000000) == 0x80000000){																																//check if sign bit is 1
			imm = ((instruction & 0x80000000) >> 19) | ((instruction & 0x7E000000) >> 20) | ((instruction & 0xF00) >> 7) | ((instruction & 0x80) << 4) | 0xFFFFE000;				//ckeck/extend with 19sign
		}
		else {
			imm = ((instruction & 0x80000000) >> 19) | ((instruction & 0x7E000000) >> 20) | ((instruction & 0xF00) >> 7) | ((instruction & 0x80) << 4);
		}
		return imm;
		break;
	case 'U':
		// calculate Imm for U-type
		//
		imm = (instruction & 0xFFFFF000);
		return imm;
		break;
	case 'J':
		// calculate Imm for J-type
		//
		if ((instruction & 0x80000000) == 0x80000000){																														//check if sign bit is 1
			imm = ((instruction & 0x80000000) >> 11) | ((instruction & 0x7FE00000) >> 20) | ((instruction & 0x100000) >> 9) | (instruction & 0xFF000) | 0xFFE00000;			//ckeck/extend with 11sign
		}
		else {
			imm = ((instruction & 0x80000000) >> 11) | ((instruction & 0x7FE00000) >> 20) | ((instruction & 0x100000) >> 9) | (instruction & 0xFF000);
		}
		return imm;
		break;
	default:
		printf("1Kein gueltiger type\n");
		break;
	}
}

/**
 * Instruction fetch Instruction decode, Execute, Memory access, Write back
 */
void CPU_execute(CPU* cpu) {

	uint32_t instruction = *(uint32_t*)(cpu->instr_mem_ + (cpu->pc_  & 0xFFFFF));

	uint8_t opcode = instruction & 0x7F;
	uint8_t rd = (instruction & 0xF80) >> 7;
	uint8_t funct3 = (instruction & 0x7000) >> 12;
	uint8_t rs1 = (instruction & 0xF8000) >> 15;
	uint8_t rs2 = (instruction & 0x1F00000) >> 20;
	uint8_t funct7 = (instruction & 0xFE000000) >> 25;

	switch(opcode) {
		// R-Type
		//
        case 0x33:
			switch (funct3)
			{
			case 0x0:
				switch (funct7)
				{
				case 0x0:
					// ADD
					// perform add of rs1 and rs2
					//
					cpu->regfile_[rd] = cpu->regfile_[rs1] + cpu->regfile_[rs2];
					cpu->pc_ += 4;
					break;
				case 0x20:
					// SUB
					// perform sub of rs2 from rs1
					//
					cpu->regfile_[rd] = cpu->regfile_[rs1] - cpu->regfile_[rs2];
					cpu->pc_ += 4;
					break;
				default:
					printf("Kein gueltiger Opcode: %x\n", funct7);
					break;
				}
				break;
			case 0x1:
				// SLL
				// perform logical left shift on the value of register rs1 by the shift amount held in lower 5 bits of rs2
				//
				cpu->regfile_[rd] = (cpu->regfile_[rs1] << (cpu->regfile_[rs2] & 0x1f));
				cpu->pc_ += 4;
				break;
			case 0x2:
				// SLT
				// perfom signed compares, 1 if true, 0 otherwise
				//
				if ((int32_t)cpu->regfile_[rs1] < (int32_t)cpu->regfile_[rs2]) {
					cpu->regfile_[rd] = 1;
				} else {
					cpu->regfile_[rd] = 0;
				}
				cpu->pc_ += 4;            
				break;
			case 0x3:
				// SLTU
				// perform unsigned ompares, 1 if true, 0 otherwise
				//
				if (cpu->regfile_[rs1] < cpu->regfile_[rs2]) {
					cpu->regfile_[rd] = 1;
				} else {
					cpu->regfile_[rd] = 0;
				}
				cpu->pc_ += 4;            
				break;
			case 0x4:
				// XOR
				// bitwise logical XOR
				//
				cpu->regfile_[rd] = cpu->regfile_[rs1] ^ cpu->regfile_[rs2];
				cpu->pc_ += 4;
				break;
			case 0x5:
				switch (funct7)
				{
				case 0x0:
					// SRL
					// perform logical right shift on the value of register rs1 by the shift amount held in lower 5 bits of rs2
					//
					cpu->regfile_[rd] = (cpu->regfile_[rs1] >> (cpu->regfile_[rs2] & 0x1f));
                    cpu->pc_ += 4;
					break;
				case 0x20:
					// SRA
					// perform arithmetic right shift on the value of register rs1 by the shift amount held in lower 5 bits of rs2
					//
					cpu->regfile_[rd] = ((int32_t)cpu->regfile_[rs1] >> (cpu->regfile_[rs2] & 0x1f));
                    cpu->pc_ += 4;
					break;
				default:
					printf("Kein gueltiger Opcode SRA\n");
					break;
				}
				break;
			case 0x6:
				// OR
				// bitwise logical OR
				//
				cpu->regfile_[rd] = (cpu->regfile_[rs1] | cpu->regfile_[rs2]);
				cpu->pc_ += 4;
				break;
			case 0x7:
				// AND 
				// bitwise logical AND
				//
				cpu->regfile_[rd] = cpu->regfile_[rs1] & cpu->regfile_[rs2];
	    		cpu->pc_ += 4;           
	    		break;
			default:
				printf("8Kein gueltiger Opcode\n");
				break;
			}
            //
            break;
		// I-Type
		//
        case 0x13:
            switch (funct3)
			{
			case 0x0:
				// ADDI
				// adds the sign-extended 12-bit imm to register rs1 (arithmetic overflow is ignored)
				//
				cpu->regfile_[rd] = cpu->regfile_[rs1] + calcimm(instruction, 'I');
				cpu->pc_ += 4;
				break;
			case 0x1:
				// SLLI
				// logical left shift imm(zeroes are shifted into the upper bits), shift amount held in lower 5 bits of rs2
				//
				cpu->regfile_[rd] = (cpu->regfile_[rs1] << (calcimm(instruction, 'I') & 0x1f));
	    		cpu->pc_ += 4;
				break;
			case 0x2:
				// SLTI
				// set less than imm - places value 1 in register rd if register rs1 is less than the sign extended imm, when both are treaded as signed numbers, else rd is 0
				//
				if ((int32_t)cpu->regfile_[rs1] < (int32_t)calcimm(instruction, 'I')) {
				cpu->regfile_[rd] = 1;
				} else {
					cpu->regfile_[rd] = 0;
				}
				cpu->pc_ += 4;           
				break;
			case 0x3:
				// SLTI-U
				// similar as slti but compares values as unsigned numbers
				//
				if (cpu->regfile_[rs1] < calcimm(instruction, 'I')) {
				cpu->regfile_[rd] = 1;
				} else {
					cpu->regfile_[rd] = 0;
				}
				cpu->pc_ += 4;           
				break;
			case 0x4:
				// XORI
				// logical operator, bitwise XOR on register rs1 and signextended 12-bit imm, place result in rd
				// 
				cpu->regfile_[rd] = (cpu->regfile_[rs1] ^ calcimm(instruction, 'I'));
	    		cpu->pc_ += 4;           
	    		break;
			case 0x5:
				switch (funct7)
				{
				case 0x0:
					// SRLI
					// logical right shift imm(zeroes are shifted into the upper bits), shift amount held in lower 5 bits of rs2
					//
					cpu->regfile_[rd] = (cpu->regfile_[rs1] >> (calcimm(instruction, 'I') & 0x1f));
					cpu->pc_ += 4;  
					break;
				case 0x20:
					// SRAI
					// arithmetic right shit imm(original sign bit is copied into the upper bits), shift amount held in lower 5 bits of rs2
					//
					cpu->regfile_[rd] = ((int32_t)cpu->regfile_[rs1] >> (calcimm(instruction, 'I') & 0x1f));
					cpu->pc_ += 4;  
					break;
				default:
					printf("Kein gueltiger Opcode bei SRLI\n");
					break;
				}
				break;
			case 0x6:
				// ORI
				// logical operator, bitwise ORI on register rs1 and signextended 12-bit imm, place result in rd
				//
				cpu->regfile_[rd] = (cpu->regfile_[rs1] | calcimm(instruction, 'I'));
	    		cpu->pc_ += 4; 
				break;
			case 0x7:
				// ANDI
				// logical operator, bitwise ANDI on register rs1 and signextended 12-bit imm, place result in rd
				// 
				cpu->regfile_[rd] = (cpu->regfile_[rs1] & calcimm(instruction, 'I'));
	    		cpu->pc_ += 4;  
				break;	   	      
			default:
				printf("Kein gueltiger Opcode bei I\n");
				break;
			}
			//
            break;
		// B-Type
		//
        case 0x63:
            switch (funct3)
			{
			case 0x0:
				// BEQ
				// take branch if register rs1 and rs2 are equal
				//
				if (cpu->regfile_[rs1] == cpu->regfile_[rs2]){
					cpu->pc_= cpu->pc_ + (int32_t)calcimm(instruction, 'B');
				}
				else {
					cpu->pc_ += 4; 
				}
				break;
			case 0x1:
				// BNE
				// take branch if register rs1 and rs2 are unequal
				//
				if (cpu->regfile_[rs1] != cpu->regfile_[rs2])
					cpu->pc_ = cpu->pc_ + (int32_t)calcimm(instruction, 'B');
				else
					cpu->pc_ += 4; 
				break;
			case 0x4:
				// BLT
				// take branch if rs1 is less than rs2 using signed comparision
				//
				if ((int32_t)cpu->regfile_[rs1] < cpu->regfile_[rs2])
					cpu->pc_ = cpu->pc_ + (int32_t)calcimm(instruction, 'B');
				else
					cpu->pc_ += 4; 
				break;
			case 0x5:
				// BGE
				// take the branch if rs1 is greater than or equal to rs2 using signed comparision
				//
				if ((int32_t)cpu->regfile_[rs1] >= (int32_t)cpu->regfile_[rs2])
					cpu->pc_ = cpu->pc_ + (int32_t)calcimm(instruction, 'B');
				else
					cpu->pc_ += 4; 
				break;
			case 0x6:
				// BLTU
				// take branch if rs1 is less than rs2 using unsigned comparision
				//
				if (cpu->regfile_[rs1] < cpu->regfile_[rs2])
					cpu->pc_ = cpu->pc_ + (uint32_t)calcimm(instruction, 'B');
				else
					cpu->pc_ += 4; 
				break;
			case 0x7:
				// BGEU
				// take the branch if rs1 is greater than or equal to rs2 using unsigned comparision
				//
				if (cpu->regfile_[rs1] >= cpu->regfile_[rs2])
					cpu->pc_ = cpu->pc_ + (uint32_t)calcimm(instruction, 'B');
				else
					cpu->pc_ += 4; 
				break;
			default:
				printf("Kein gueltiger Opcode Bei B\n");
				break;
			}
			//
            break;
		// JAL
		// jump and link - J-imm encodes signed offset, added to address of the jump instruction, to form jump target address
		//
        case 0x6F:
            cpu->regfile_[rd] = cpu->pc_ + 4;
			cpu->pc_ = cpu->pc_ + ((int32_t)calcimm(instruction, 'J'));
            break;
		// JALR
		// jump and link register - target adress (adding sign extended 12bit I-imm to reg rs1)
		//
		case 0x67:
			cpu->regfile_[rd] = cpu->pc_ + 4;
			cpu->pc_ = (cpu->regfile_[rs1]) + ((int32_t)calcimm(instruction, 'I'));
			break;
		// STORE
		//
        case 0x23:
            switch (funct3)
			{
			case 0x0:
				// SB
				// store 8bit values from the low bits of the register to rs2 memory
				//
				if (cpu->regfile_[rs1] + (int32_t)calcimm(instruction, 'S') == 0x5000){
					putchar((int8_t)cpu->regfile_[rs2]);
				}
				cpu->data_mem_[cpu->regfile_[rs1] + (int32_t)calcimm(instruction, 'S')] = (uint8_t)cpu->regfile_[rs2];
				cpu->pc_ += 4;
				break;
			case 0x1:
				// SH
				// store 16bit values from the low bits of the register to rs2 memory
				//
				*(int16_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + calcimm(instruction, 'S')) = (int16_t)cpu->regfile_[rs2];
				cpu->pc_ += 4;
				break;
			case 0x2:
				// SW
				// store 32bit values from the low bits of the register to rs2 memory
				//
				*(int32_t*)(cpu->data_mem_ + cpu->regfile_[rs1] + calcimm(instruction, 'S')) = (int32_t)cpu->regfile_[rs2];
				cpu->pc_ += 4;
				break;
			default:
				printf("Kein gueltiger Opcode Store\n");
				break;
			}
			//
            break;
		// LOAD
		//
        case 0x03:
			switch (funct3)
			{
			case 0x0:
				// LB
				// loads a 8bit value from memory, then sign extended to 32bit before store in rd
				//
				cpu->regfile_[rd] = (int8_t)cpu->data_mem_[(cpu->regfile_[rs1]) + calcimm(instruction, 'I')];
	    		cpu->pc_ += 4; 
				break;
			case 0x1:
				// LH
				// loads a 16bit value from memory, then sign extended to 32bit before store in rd
				//
				cpu->regfile_[rd] = *(int16_t*)(cpu->regfile_[rs1] + calcimm(instruction, 'I') + cpu->data_mem_);
				cpu->pc_ += 4;
				break;
			case 0x2:
				// LW
				// loads a 32bit value from memory in rd
				//
				cpu->regfile_[rd] = *(int32_t*)(cpu->regfile_[rs1] + calcimm(instruction, 'I') + cpu->data_mem_);
				cpu->pc_ += 4;
				break;
			case 0x4:
				// LBU
				// loads a 8bit value from memory, then zero extended to 32bit before store in rd
				//
				cpu->regfile_[rd] = cpu->data_mem_[cpu->regfile_[rs1] + calcimm(instruction, 'I')];
	    		cpu->pc_ += 4; 
				break;
			case 0x5:
				// LHU
				// loads a 16bit value from memory, then zero extended to 32bit before store in rd
				//
				cpu->regfile_[rd] = *(uint16_t*)(cpu->regfile_[rs1] + calcimm(instruction, 'I') + cpu->data_mem_);
				cpu->pc_ += 4;
				break;
			default:
				printf("9Kein gueltiger Opcode\n");
				break;
			}
            //
            break;
		// LUI
		// load upper imm - build 32bit constants and usese U type, place U-imm in the top 20bit of register rd, filling lowest 12 bit with zeros
		//
        case 0x37:
			cpu->regfile_[rd] = calcimm(instruction, 'U');
			cpu->pc_ += 4;
            break;
		// AUIPC
		// add upper imm to pc - pc relative addresses, forms 32bit offset from the 20bit U-imm, filling lowest 12bit with zeros, add this to pc and store in rd
		//
		case 0x17:
			cpu->regfile_[rd] = cpu->pc_ + calcimm(instruction, 'U');
			cpu->pc_ += 4;
			break;
        default: 
		    printf("10Kein gueltiger Opcode\n");
            break;
    }

	cpu->regfile_[0] = 0; //set memory on register 0 to 0
}

int main(int argc, char* argv[]) {
	printf("C Praktikum\nHU Risc-V  Emulator 2022\n");

	CPU* cpu_inst;

	cpu_inst = CPU_init(argv[1], argv[2]);
    for(uint32_t i = 0; i <70000; i++) { // run 70000 cycles
    	CPU_execute(cpu_inst);
    }

	printf("\n-----------------------RISC-V program terminate------------------------\nRegfile values:\n");

	//output Regfile
	for(uint32_t i = 0; i <= 31; i++) {
    	printf("%d: %X\n",i,cpu_inst->regfile_[i]);
    }
    fflush(stdout);

	return 0;
}
