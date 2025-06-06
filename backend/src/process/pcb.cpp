#include "process/pcb.h"

PCB::PCB(pid_t id) : pid(id), state(ProcessState::NEW), program_counter(0) {
    // 初始化其他成员
} 