/**************************************************************************
 * C S 429 system emulator
 *
 * hw_elts.c - Module for emulating hardware elements.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper., P. Jamadagni
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include <assert.h>
#include "hw_elts.h"
#include "mem.h"
#include "machine.h"
#include "err_handler.h"

extern machine_t guest;

comb_logic_t
imem(uint64_t imem_addr,
     uint32_t *imem_rval, bool *imem_err)
{
    // imem_addr must be in "instruction memory" and a multiple of 4
    *imem_err = (!addr_in_imem(imem_addr) || (imem_addr & 0x3U));
    *imem_rval = (uint32_t)mem_read_I(imem_addr);
}

comb_logic_t
regfile(uint8_t src1, uint8_t src2, uint8_t dst, uint64_t val_w,
        bool w_enable,
        uint64_t *val_a, uint64_t *val_b)
{
    if (src1 < 31)
    {
        *val_a = guest.proc->GPR[src1];
    }
    else if (src1 >= 32)
    {
        *val_a = 0;
    }
    else if (src1 == 31)
    {
        *val_a = guest.proc->SP;
    }

    if (src2 < 31)
    {
        *val_b = guest.proc->GPR[src2];
    }
    else if (src2 >= 32)
    {
        *val_b = 0;
    }
    else if (src2 == 31)
    {
        *val_b = guest.proc->SP;
    }

    if (w_enable)
    {
        if (dst < 31)
        {
            guest.proc->GPR[dst] = val_w;
        }
        else if (dst == 31)
        {
            guest.proc->SP = val_w;
        }
    }
}

static bool
cond_holds(cond_t cond, uint8_t flags)
{
    if (cond == C_EQ)
    {
        return GET_ZF(flags) == 1;
    }
    else if (cond == C_NE)
    {
        return GET_ZF(flags) == 0;
    }
    else if (cond == C_CS)
    {
        return GET_CF(flags) == 1;
    }
    else if (cond == C_CC)
    {
        return GET_CF(flags) == 0;
    }
    else if (cond == C_MI)
    {
        return GET_NF(flags) == 1;
    }
    else if (cond == C_PL)
    {
        return GET_NF(flags) == 0;
    }
    else if (cond == C_VS)
    {
        return GET_VF(flags) == 1;
    }
    else if (cond == C_VC)
    {
        return GET_VF(flags) == 0;
    }
    else if (cond == C_HI)
    {
        return GET_CF(flags) == 1 && GET_ZF(flags) == 0;
    }
    else if (cond == C_LS)
    {
        return !(GET_CF(flags) == 1 && GET_ZF(flags) == 0);
    }
    else if (cond == C_GE)
    {
        return GET_NF(flags) == GET_VF(flags);
    }
    else if (cond == C_LT)
    {
        return GET_NF(flags) != GET_VF(flags);
    }
    else if (cond == C_GT)
    {
        return GET_ZF(flags) == 0 && (GET_NF(flags) == GET_VF(flags));
    }
    else if (cond == C_LE)
    {
        return GET_ZF(flags) == 1 || (GET_NF(flags) != GET_VF(flags));
    }
    else
    {
        return true;
    }
}

comb_logic_t
alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw, alu_op_t ALUop, bool set_flags, cond_t cond,
    uint64_t *val_e, bool *cond_val, uint8_t *nzcv)
{
    uint64_t res = 0xFEEDFACEDEADBEEF; // To make it easier to detect errors.
    // Student TODO

    // ADD
    if (ALUop == PLUS_OP)
    {
        uint64_t temp_unsigned = alu_vala + alu_valb;
        int64_t temp_signed = (int64_t)alu_vala + (int64_t)alu_valb;
        if (set_flags)
        {
            if (temp_signed < 0)
            {
                *nzcv = PACK_CC(1, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(0, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            if (temp_signed == 0)
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 1, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 0, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            if (temp_unsigned < alu_vala || temp_unsigned < alu_valb)
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), 1, GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), 0, GET_VF(*nzcv));
            }
            if (((int64_t)alu_vala < 0 && (int64_t)alu_valb < 0 && temp_signed >= 0) || ((int64_t)alu_vala > 0 && (int64_t)alu_valb > 0 && temp_signed <= 0))
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), GET_CF(*nzcv), 1);
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), GET_CF(*nzcv), 0);
            }
        }
        *cond_val = cond_holds(cond, *nzcv);
        *val_e = temp_signed;
    }

    // SUB
    if (ALUop == MINUS_OP)
    {
        uint64_t temp_unsigned = alu_vala - alu_valb;
        int64_t temp_signed = (int64_t)alu_vala - (int64_t)alu_valb;
        if (set_flags)
        {
            if (temp_signed < 0)
            {
                *nzcv = PACK_CC(1, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(0, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            if (temp_signed == 0)
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 1, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 0, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            uint64_t temp_complement = alu_vala + ~alu_valb + 1;
            if (temp_complement < alu_vala || alu_valb == 0 || temp_complement < (~alu_valb + 1))
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), 1, GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), 0, GET_VF(*nzcv));
            }
            int64_t temp_complement_signed = ~alu_valb + 1;
            if (((int64_t)alu_vala < 0 && (int64_t)temp_complement_signed < 0 && temp_signed >= 0) || ((int64_t)alu_vala > 0 && (int64_t)temp_complement_signed > 0 && temp_signed <= 0))
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), GET_CF(*nzcv), 1);
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), GET_ZF(*nzcv), GET_CF(*nzcv), 0);
            }
        }
        *cond_val = cond_holds(cond, *nzcv);
        *val_e = temp_signed;
    }

    // INV
    if (ALUop == INV_OP)
    {
        *val_e = alu_vala | ~alu_valb;
    }

    // OR
    if (ALUop == OR_OP)
    {
        *val_e = alu_vala | alu_valb;
    }

    // EOR
    if (ALUop == EOR_OP)
    {
        *val_e = alu_vala ^ alu_valb;
    }

    // AND
    if (ALUop == AND_OP)
    {
        int64_t temp = alu_vala & alu_valb;
        if (set_flags)
        {
            if (temp < 0)
            {
                *nzcv = PACK_CC(1, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(0, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            if (temp == 0)
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 1, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 0, GET_CF(*nzcv), GET_VF(*nzcv));
            }
        }
        *cond_val = cond_holds(cond, *nzcv);
        *val_e = temp;
    }

    // MOV
    if (ALUop == MOV_OP)
    {
        *val_e = alu_vala | (alu_valb << alu_valhw);
    }

    // LSL
    if (ALUop == LSL_OP)
    {
        *val_e = alu_vala << (alu_valb & 0x3FUL);
    }

    // LSR
    if (ALUop == LSR_OP)
    {
        *val_e = alu_vala >> (alu_valb & 0x3FUL);
    }

    // ASR
    if (ALUop == ASR_OP)
    {
        *val_e = (int64_t)alu_vala >> (alu_valb & 0x3FUL);
    }

    // PASS_A
    if (ALUop == PASS_A_OP)
    {
        int64_t temp = alu_vala;
        if (set_flags)
        {
            if (temp < 0)
            {
                *nzcv = PACK_CC(1, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(0, GET_ZF(*nzcv), GET_CF(*nzcv), GET_VF(*nzcv));
            }
            if (temp == 0)
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 1, GET_CF(*nzcv), GET_VF(*nzcv));
            }
            else
            {
                *nzcv = PACK_CC(GET_NF(*nzcv), 0, GET_CF(*nzcv), GET_VF(*nzcv));
            }
        }
        *cond_val = cond_holds(cond, *nzcv);
        *val_e = temp;
    }
}

comb_logic_t
dmem(uint64_t dmem_addr, uint64_t dmem_wval, bool dmem_read, bool dmem_write,
     uint64_t *dmem_rval, bool *dmem_err)
{
    if (!dmem_read && !dmem_write)
    {
        return;
    }
    // dmem_addr must be in "data memory" and a multiple of 8
    *dmem_err = (!addr_in_dmem(dmem_addr) || (dmem_addr & 0x7U));
    if (is_special_addr(dmem_addr))
        *dmem_err = false;
    if (dmem_read)
        *dmem_rval = (uint64_t)mem_read_L(dmem_addr);
    if (dmem_write)
        mem_write_L(dmem_addr, dmem_wval);
}
