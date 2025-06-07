/**************************************************************************
 * C S 429 system emulator
 *
 * forward.c - The value forwarding logic in the pipelined processor.
 **************************************************************************/

#include "forward.h"
#include <stdbool.h>

/* STUDENT TO-DO:
 * Implement forwarding register values from
 * execute, memory, and writeback back to decode.
 */
comb_logic_t forward_reg(uint8_t D_src1, uint8_t D_src2, uint8_t X_dst,
                         uint8_t M_dst, uint8_t W_dst, uint64_t X_val_ex,
                         uint64_t M_val_ex, uint64_t M_val_mem,
                         uint64_t W_val_ex, uint64_t W_val_mem, bool M_wval_sel,
                         bool W_wval_sel, bool X_w_enable, bool M_w_enable,
                         bool W_w_enable, uint64_t *val_a, uint64_t *val_b)
{

  // Forward from W
  *val_a = ((W_dst == D_src1) && W_w_enable) ? W_wval_sel ? W_val_mem : W_val_ex : *val_a;
  *val_b = ((W_dst == D_src2) && W_w_enable) ? W_wval_sel ? W_val_mem : W_val_ex : *val_b;

  // Forward from M
  *val_a = ((M_dst == D_src1) && M_w_enable) ? M_wval_sel ? M_val_mem : M_val_ex : *val_a;
  *val_b = ((M_dst == D_src2) && M_w_enable) ? M_wval_sel ? M_val_mem : M_val_ex : *val_b;

  // Forward from X
  *val_a = (X_dst == D_src1) && X_w_enable ? X_val_ex : *val_a;
  *val_b = (X_dst == D_src2) && X_w_enable ? X_val_ex : *val_b;
}
