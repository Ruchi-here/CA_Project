#include "pipeline.hpp"

// Bypass register's reading function
u32 bypass_read(pipeline *pipe,struct d_unit *decode,bool isrs1){
    if(isrs1){
        if(pipe->bypass->rde==decode->rs1){
            return pipe->bypass->res_exec;
        }
        else if(pipe->bypass->rdm==decode->rs1){
            return pipe->bypass->res_mem;
        }
        return decode->rs1_val;
    }
    else{
        if(pipe->bypass->rde==decode->rs2){
            return pipe->bypass->res_exec;
        }
        else if(pipe->bypass->rdm==decode->rs2){
            return pipe->bypass->res_mem;
        }
        return decode->rs2_val;
    }
}

pipeline* pipe_init(){
    pipeline *pipe = (pipeline *)malloc(sizeof(pipeline));
    pipe->cycle = 0;
    pipe->decode = (d_unit *)malloc(sizeof(d_unit));
    pipe->fetch = (f_unit *)malloc(sizeof(f_unit));
    pipe->execute = (exec_unit *)malloc(sizeof(exec_unit));
    pipe->memory = (mem_unit *)malloc(sizeof(mem_unit));
    pipe->writeback = (wb_unit *)malloc(sizeof(wb_unit));
    pipe->bypass = (bypassreg *)malloc(sizeof(bypassreg));
    pipe->data_stall_counter = 0;
    pipeline_reset(pipe);
    return pipe;
}

bool pipeline_end(bool fetchdone,pipeline* pipe){
    if(!fetchdone)return true;
    if(pipe->fetch->inst==0 && pipe->decode->inst==0 && (i32)pipe->execute->inst==0 )return false;
    return true;
}

void bypass_reset(bypassreg *bypass){
    bypass->res_exec = 0;
    bypass->res_mem = 0;
    bypass->rde = 34;
    bypass->rdm = 34;
    bypass->new_pc =0;
}

void decode_reset(d_unit *decode){
    decode->inst = 0;
    decode->rs1 = 34;
    decode->rs2 = 34;
    decode->rd = 34;
    decode->imm = 0;
    decode->rs1_val = 0;
    decode->rs2_val = 0;
    decode->done=false;
    decode->isimm=true;
    decode->isload=false;
    decode->isstore=false;
    decode->iswrite=false;
    decode->isnoc=false;
    decode->isstype=false;
    decode->pcd=0;
}

void fetch_reset(f_unit *fetch){
    fetch->inst = 0;
    fetch->done=false;
    fetch->pcf=0;
}

void execute_reset(exec_unit *execute){
    execute->inst = 0;
    execute->op1 = 0;
    execute->op2 = 0;
    execute->result = 0;
    execute->rd = 34;
    execute->rs = 34;
    execute->size = 0;
    execute->usign = false;
    execute->done=false;
    execute->isload=false;
    execute->isstore=false;
    execute->iswrite=false;
    execute->isnoc=false;
    execute->isstype=false;
    execute->pce=0;
}

void memory_reset(mem_unit *memory){
    memory->addr = 0;
    memory->size = 0;
    memory->value = 0;
    memory->rd = 34;
    memory->usign = false;
    memory->done=false;
    memory->isload=false;
    memory->isstore=false;
    memory->iswrite=false;
    memory->isnoc=false;
    memory->isstype=false;
}

void writeback_reset(wb_unit *writeback){
    writeback->rd = 0;
    writeback->imm = 0;
    writeback->done=false;
    writeback->iswrite=false;
}

void cycle_reset(pipeline *pipe){
    pipe->cycle = 0;
}

void pipeline_reset(pipeline *pipe){
    cycle_reset(pipe);
    decode_reset(pipe->decode);
    fetch_reset(pipe->fetch);
    execute_reset(pipe->execute);
    memory_reset(pipe->memory);
    writeback_reset(pipe->writeback);
    bypass_reset(pipe->bypass);
    pipe->isbranch=false;
    pipe->newpc_offset=4;
    pipe->isjalr=false;
    pipe->isbranch2=false;
    pipe->newpc_offset2=4;
    pipe->de_stall=false;
    pipe->ex_stall=false;
    pipe->data_stall_counter = 0;
}

void changef_to_d(pipeline *pipe){
    pipe->decode->isload=false;
    pipe->decode->isstore=false;
    pipe->decode->iswrite=false;
    pipe->decode->isnoc=false;
    pipe->decode->inst = pipe->fetch->inst;
    pipe->decode->rs1 = 34;
    pipe->decode->rs2 = 34;
    pipe->decode->isimm=true;
    pipe->decode->rd = 34;
    pipe->decode->imm = 0;
    pipe->decode->rs1_val = 0;
    pipe->decode->rs2_val = 0;
    pipe->decode->done=false;
    pipe->decode->isstype=false;
    pipe->decode->pcd=pipe->fetch->pcf;
}

void changed_to_ex(pipeline *pipe){
    pipe->execute->isstore = pipe->decode->isstore;
    pipe->execute->isload = pipe->decode->isload;
    pipe->execute->iswrite = pipe->decode->iswrite;
    pipe->execute->isnoc = pipe->decode->isnoc;
    pipe->execute->inst = pipe->decode->inst;
    pipe->execute->isstype = pipe->decode->isstype;
    pipe->execute->op1=bypass_read(pipe,pipe->decode,true);
    // pipe->execute->op1 = pipe->decode->rs1_val;
    pipe->execute->rs = pipe->decode->rs2;
    pipe->execute->result = 0;
    if(pipe->decode->rs2!=34){
        // pipe->execute->op2 = pipe->decode->rs2_val;
        pipe->execute->op2=bypass_read(pipe,pipe->decode,false);
        if(pipe->decode->isimm){
            pipe->execute->rd = pipe->decode->imm;
        }
        else{
            pipe->execute->rd = pipe->decode->rd;
        }
    }
    else{
        pipe->execute->op2 = pipe->decode->imm;
        pipe->execute->rd = pipe->decode->rd;
    }
    pipe->execute->size = 0;
    pipe->execute->usign = false;
    pipe->execute->done=false;
    pipe->execute->pce=pipe->decode->pcd;
}

void changeex_to_m(pipeline *pipe){
    pipe->memory->rd =pipe->execute->rd;
    pipe->memory->usign = pipe->execute->usign;
    pipe->memory->size = pipe->execute->size;
    pipe->memory->isload = pipe->execute->isload;
    pipe->memory->isstore = pipe->execute->isstore;
    pipe->memory->iswrite = pipe->execute->iswrite;
    pipe->memory->isnoc = pipe->execute->isnoc;
    pipe->memory->isstype = pipe->execute->isstype;
    if(pipe->memory->isstore){
        pipe->memory->addr = pipe->execute->result;
        pipe->memory->value = pipe->execute->op2;
    }
    else if(pipe->memory->isload){
        pipe->memory->addr = pipe->execute->result;
    }
    else{
        pipe->memory->value = pipe->execute->result;
    }
    pipe->memory->done = false;
    pipe->memory->inst = pipe->execute->inst;
}

void changem_to_wb(pipeline *pipe){
    pipe->writeback->rd = pipe->memory->rd;
    pipe->writeback->imm = pipe->memory->value;
    pipe->writeback->iswrite = pipe->memory->iswrite;
    pipe->writeback->done = false;
    pipe->writeback->inst = pipe->memory->inst;
}

// reset the branch flags
void reset_branchflags(pipeline *pipe){
    pipe->isbranch=false;
    pipe->newpc_offset=4;
    pipe->isbranch2=false;
    pipe->isjalr=false;
    pipe->newpc_offset2=4;
}

//to try state change
void try_statechange(pipeline* pipe){
    changem_to_wb(pipe);
    memory_reset(pipe->memory);
    if(!pipe->ex_stall){
        changeex_to_m(pipe);
        execute_reset(pipe->execute);
        if(!pipe->de_stall){
            changed_to_ex(pipe);
            changef_to_d(pipe);
            fetch_reset(pipe->fetch);
        }
    }
}

//to detect and kill the pipeline
void jump_withoutBYPASSING(pipeline*pipe){
    if(pipe->isbranch){
        fetch_reset(pipe->fetch);
        decode_reset(pipe->decode);
    }
    else if(pipe->isbranch2){
        fetch_reset(pipe->fetch);
    }
    reset_branchflags(pipe);
}

void jump(pipeline*pipe,u32* pc){
    // std::cout<<pipe->cycle<<'\n';
    if(pipe->isbranch){
        if(pipe->bypass->new_pc!=pipe->decode->pcd){
            decode_reset(pipe->decode);
            if(pipe->bypass->new_pc!=pipe->fetch->pcf){
                fetch_reset(pipe->fetch);
            }
            else{
                *pc=pipe->fetch->pcf +4;
            }
        }
        else{
            *pc=pipe->decode->pcd +8;
        }
    }
    else if(pipe->isbranch2){
        if(pipe->bypass->new_pc!=pipe->fetch->pcf){
            fetch_reset(pipe->fetch);
        }
        else{
            *pc=pipe->fetch->pcf +4;
        }
    }

    reset_branchflags(pipe);
}

// to stall the pipeline
void stall_withoutBYPASSING(pipeline*pipe){
    if(pipe->execute->isstore && pipe->execute->rs!=34 && pipe->memory->rd!=34  && pipe->memory->rd==pipe->execute->rs){
        pipe->ex_stall=true;
        pipe->data_stall_counter++;
    }
    else{
        pipe->ex_stall=false;
        if(!pipe->memory->isstype && pipe->memory->rd!=34 && ((pipe->decode->rs1!=34 && pipe->memory->rd==pipe->decode->rs1)||(pipe->decode->rs2!=34 && pipe->memory->rd==pipe->decode->rs2))){
            pipe->de_stall=true;
            pipe->data_stall_counter++;
        }
        else if( !pipe->decode->isstore && !pipe->execute->isstype && pipe->execute->rd!=34 && ((pipe->decode->rs1!=34 && pipe->execute->rd==pipe->decode->rs1)||(pipe->decode->rs2!=34 && pipe->execute->rd==pipe->decode->rs2))){
            pipe->de_stall=true;
            pipe->data_stall_counter++;
        }
        else
        pipe->de_stall=false;
    }
}

void stall(pipeline*pipe){
    if(pipe->execute->isload && pipe->execute->rd!=34 && ((pipe->decode->rs1 !=34 && pipe->execute->rd==pipe->decode->rs1)||( pipe->decode->rs2 && pipe->execute->rd==pipe->decode->rs2))){
        pipe->de_stall=true;
        pipe->data_stall_counter++; }
    else
{    pipe->de_stall=false;
    pipe->ex_stall=false;
}
}

// state change functions
bool statechange_withoutBYPASSING(pipeline* pipe,bool over){
    bool notend=pipeline_end(over,pipe);
    jump_withoutBYPASSING(pipe);
    stall_withoutBYPASSING(pipe);
    try_statechange(pipe);
    if(over){
        pipe->fetch->done=true;
    }
    pipe->cycle+=1;
    return notend;
}

bool statechange(pipeline* pipe,u32* pc, bool over){
    bool notend=pipeline_end(over,pipe);
    jump(pipe,pc);
    stall(pipe);
    try_statechange(pipe);
    if(over){
        pipe->fetch->done=true;
    }
    pipe->cycle+=1;
    return notend;
}


// ignore (useless implementation) 
//  //   ideal pipeline state change function Implementation 
// void changestate(RISCV_cpu *cpu){
    // cpu->__pipe->writeback->rd = cpu->__pipe->memory->rd;
    // cpu->__pipe->writeback->imm = cpu->__pipe->memory->value;
    // cpu->__pipe->memory->rd =cpu->__pipe->execute->rd;
    // cpu->__pipe->memory->usign = cpu->__pipe->execute->usign;
    // cpu->__pipe->memory->size = cpu->__pipe->execute->size;
    // if(cpu->__pipe->isstore&0x4==0x4){
    //     cpu->__pipe->memory->addr = cpu->__pipe->execute->result;
    //     cpu->__pipe->memory->value = cpu->__pipe->execute->op2;
    // }
    // else if(cpu->__pipe->isload&0x4==0x4){
    //     cpu->__pipe->memory->addr = cpu->__pipe->execute->result;
    // }
    // else{
    //     cpu->__pipe->memory->value = cpu->__pipe->execute->result;
    // }
    // cpu->__pipe->execute->inst = cpu->__pipe->decode->inst;
    // cpu->__pipe->execute->op1 = cpu->__pipe->decode->rs1_val;
    // cpu->__pipe->execute->size = 0;
    // cpu->__pipe->execute->usign = false;
    // cpu->__pipe->execute->result = 0;
    // if(cpu->__pipe->decode->rs2!=34){
    //     cpu->__pipe->execute->op2 = cpu->__pipe->decode->rs2_val;
    //     if(cpu->__pipe->decode->isimm){
    //         cpu->__pipe->execute->rd = cpu->__pipe->decode->imm;
    //     }
    //     else{
    //         cpu->__pipe->execute->rd = cpu->__pipe->decode->rd;
    //     }
    // }
    // else{
    //     cpu->__pipe->execute->op2 = cpu->__pipe->decode->imm;
    //     cpu->__pipe->execute->rd = cpu->__pipe->decode->rd;
    // }
    // cpu->__pipe->decode->inst = cpu->__pipe->fetch->inst;
    // cpu->__pipe->decode->isimm = true;
    // cpu->__pipe->decode->rs2==34;
    // cpu->__pipe->decode->rs1==34;
// }
