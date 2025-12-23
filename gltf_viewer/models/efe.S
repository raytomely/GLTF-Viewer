/* See LICENSE file for copyright and license details. */

/* from: https://gitlab.com/jype/efeasm/-/blob/main/efe.S?ref_type=heads */

/* the effect functions are not documented. IIRC there's 96 functions ranging from math functions to "draw aurora freeze"
several of these functions generate geometry on the fly and don't use 3d models. I think confuse storm rainbow strips are also generated
*/

/* Memory opcode helper macro */
.macro		efe_mem, mem
.ifc		\mem, global
.2byte		0xdaad
.else
.ifc		\mem, local
.2byte		0x0000
.else
/* Invalid memory type */
.err
.endif
.endif
.endm

/* 0x00: Load immediate halfword (setC) */

/*
 * efe_li: Load immediate halfword <value>
 *
 * Load immediate halfword <value> to accumulator.
 */
.macro		efe_li value
.2byte		0xcd00, \value, 0x0000
.endm

/* 0x01: Load value from memory (setV) */

/*
 * efe_lw: Load word from memory <mem>, <offset>
 *
 * Load word from memory <mem>, <offset> to accumulator.
 */
.macro		efe_lw mem, offset
.2byte		0x0401, \offset
		efe_mem \mem
.endm

/*
 * efe_lh: Load halfword from memory <mem>, <offset>
 *
 * Load halfword from memory <mem>, <offset> to accumulator.
 */
.macro		efe_lh mem, offset
.2byte		0x0201, \offset
		efe_mem \mem
.endm

/*
 * efe_lb: Load byte from memory <mem>, <offset>
 *
 * Load byte from memory <mem>, <offset> to accumulator.
 */
.macro		efe_lb mem, offset
.2byte		0x0101, \offset
		efe_mem \mem
.endm

/* 0x02: Load random word (rand) */

/*
 * efe_rand: Load random word
 *
 * Load random word to accumulator.
 */
.macro		efe_rand
.2byte		0x0002
.endm

/* 0x03: Load indirect value (setT) */

/* Generate indirect load macros */
.macro		efe_gen_lxy suffix, size
.macro		efe_lw\()\suffix mem, yoffset, xoffset
.2byte		(0x4003 | ((\size & 0x0f) << 8)), \yoffset, 0xdaad, \xoffset
		efe_mem \mem
.endm
.macro		efe_lh\()\suffix mem, yoffset, xoffset
.2byte		(0x2003 | ((\size & 0x0f) << 8)), \yoffset, 0xdaad, \xoffset
		efe_mem \mem
.endm
.macro		efe_lb\()\suffix mem, yoffset, xoffset
.2byte		(0x1003 | ((\size & 0x0f) << 8)), \yoffset, 0xdaad, \xoffset
		efe_mem \mem
.endm
.endm

/* efe_l<x><y>: Load indirect value from memory <mem>, <yoffset>, <xoffset>
 *
 * Load offset of type <x> from memory <mem>, <xoffset>.
 * Load value of type <y> from global memory
 * (<yoffset> + (offset * (sizeof <y>))) to accumulator.
 *
 * <x> and <y> are one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_lww, efe_lwh, efe_lwb,
 *         efe_lhw, efe_lhh, efe_lhb,
 *         efe_lbw, efe_lbh, efe_lbb
 */
		efe_gen_lxy w, 0x04
		efe_gen_lxy h, 0x02
		efe_gen_lxy b, 0x01

/* 0x04: Arithmetic/logic/store operation (setV) */

/* Generate arithmetic/logic macros */
.macro		efe_a type, size, mem, offset
.2byte		(0x0004 | ((\size & 0x0f) << 8) | ((\type & 0x0f) << 12))
.2byte		\offset
		efe_mem \mem
.endm

.macro		efe_gen_a suffix, size
.macro		efe_s\()\suffix mem, offset
		efe_a 0x00, \size, \mem, \offset
.endm
.macro		efe_add\()\suffix mem, offset
		efe_a 0x01, \size, \mem, \offset
.endm
.macro		efe_sub\()\suffix mem, offset
		efe_a 0x02, \size, \mem, \offset
.endm
.macro		efe_mul\()\suffix mem, offset
		efe_a 0x03, \size, \mem, \offset
.endm
.macro		efe_div\()\suffix mem, offset
		efe_a 0x04, \size, \mem, \offset
.endm
.macro		efe_mod\()\suffix mem, offset
		efe_a 0x05, \size, \mem, \offset
.endm
.macro		efe_sll\()\suffix mem, offset
		efe_a 0x06, \size, \mem, \offset
.endm
.macro		efe_srl\()\suffix mem, offset
		efe_a 0x07, \size, \mem, \offset
.endm
.endm

/*
 * efe_s<x>: Store to value in memory <mem>, <offset>
 *
 * Store accumulator to value of type <x> in memory <mem>, <offset>.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_sw, efe_sh, efe_sb
 */

/*
 * efe_add<x>: Add to value in memory <mem>, <offset>
 *
 * Add accumulator to value of type <x> in memory <mem>, <offset>.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_addw, efe_addh, efe_addb
 */

/*
 * efe_sub<x>: Subtract from value in memory <mem>, <offset>
 *
 * Subtract accumulator from value of type <x> in memory <mem>, <offset>.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_subw, efe_subh, efe_subb
 */

/*
 * efe_mul<x>: Multiply value in memory <mem>, <offset>
 *
 * Multiply value of type <x> in memory <mem>, <offset> by accumulator.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_mulw, efe_mulh, efe_mulb
 */

/*
 * efe_div<x>: Divide value in memory <mem>, <offset>
 *
 * Divide value of type <x> in memory <mem>, <offset> by accumulator.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_divw, efe_divh, efe_divb
 */

/*
 * efe_mod<x>: Store modulo to value in memory <mem>, <offset>
 *
 * Store modulo of value of type <x> in memory <mem>, <offset> by accumulator.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_modw, efe_modh, efe_modb
 */

/*
 * efe_sll<x>: Logical shift left value in memory <mem>, <offset>
 *
 * Logical shift left value of type <x> in memory <mem>, <offset> by
 * accumulator.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_sllw, efe_sllh, efe_sllb
 */

/*
 * efe_srl<x>: Logical shift right value in memory <mem>, <offset>
 *
 * Logical shift right value of type <x> in memory <mem>, <offset> by
 * accumulator.
 *
 * <x> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_srlw, efe_srlh, efe_srlb
 */
		efe_gen_a w, 0x04
		efe_gen_a h, 0x02
		efe_gen_a b, 0x01

/* 0x05: Halt (null) */

/*
 * efe_halt: Halt execution
 */
.macro		efe_halt
.2byte		0x0005
.endm

/* 0x07: Branch on condition (ifV) */

/* Generate branch macros */
.macro		efe_b type, size, mem, doffset, boffset
.2byte		(0x0007 | ((\size & 0x0f) << 8) | ((\type & 0x0f) << 12))
.2byte		\doffset
		efe_mem \mem
.2byte		\boffset
.2byte		0xdaad
.endm

.macro		efe_gen_b suffix, size
.macro		efe_beq\()\suffix mem, doffset, boffset
		efe_b 0x00, \size, \mem, \doffset, \boffset
.endm
.macro		efe_bne\()\suffix mem, doffset, boffset
		efe_b 0x01, \size, \mem, \doffset, \boffset
.endm
.macro		efe_blt\()\suffix mem, doffset, boffset
		efe_b 0x02, \size, \mem, \doffset, \boffset
.endm
.macro		efe_ble\()\suffix mem, doffset, boffset
		efe_b 0x03, \size, \mem, \doffset, \boffset
.endm
.macro		efe_bgt\()\suffix mem, doffset, boffset
		efe_b 0x04, \size, \mem, \doffset, \boffset
.endm
.macro		efe_bge\()\suffix mem, doffset, boffset
		efe_b 0x05, \size, \mem, \doffset, \boffset
.endm
.endm

/*
 * efe_b<x><y>: Branch on condition <mem>, <doffset>, <boffset>
 *
 * Compare accumulator with value of type <y> in memory <mem>, <doffset>.
 * If condition <x> is true, branch to global memory <boffset>.
 *
 * <x> is one of eq (equals), ne (not equals), lt (less than),
 * le (less than or equal), gt (greater than), ge (greater than or equal).
 *
 * <y> is one of w (word), h (halfword), b (byte).
 *
 * Macros: efe_beqw, efe_beqh, efe_beqb,
 *         efe_bnew, efe_bneh, efe_bneb,
 *         efe_bltw, efe_blth, efe_bltb,
 *         efe_blew, efe_bleh, efe_bleb,
 *         efe_bgtw, efe_bgth, efe_bgtb,
 *         efe_bgew, efe_bgeh, efe_bgeb
 */
		efe_gen_b w, 0x04
		efe_gen_b h, 0x02
		efe_gen_b b, 0x01

/* 0x09: Jump (goto) */

/*
 * efe_jump: Jump to global memory <offset>
 */
.macro		efe_jump offset
.2byte		0x0009, \offset, 0xdaad
.endm

/* 0x0a: Push immediate halfword to stack (pushC) */

/*
 * efe_pushi: Push immediate halfword <value> to stack
 */
.macro		efe_pushi value
.2byte		0xcd0a, \value, 0x0000
.endm

/* 0x0b: Push value from memory to stack (pushV) */

/*
 * efe_pushw: Push word from memory <mem>, <offset> to stack
 */
.macro		efe_pushw mem, offset
.2byte		0x040b, \offset
		efe_mem \mem
.endm

/*
 * efe_pushh: Push halfword from memory <mem>, <offset> to stack
 */
.macro		efe_pushh mem, offset
.2byte		0x020b, \offset
		efe_mem \mem
.endm

/*
 * efe_pushb: Push byte from memory <mem>, <offset> to stack
 */
.macro		efe_pushb mem, offset
.2byte		0x010b, \offset
		efe_mem \mem
.endm

/* 0x0c: Push address of memory to stack (pushA) */

/*
 * efe_pusha: Push address of memory <mem>, <offset> to stack
 */
.macro		efe_pusha mem, offset
/* NOTE: The only purpose of below conditional is to minimize binary diff */
.ifc		\mem, global
.2byte		0xcd0c
.else
.ifc		\mem, local
.2byte		0x040c
.else
/* Invalid memory type */
.err
.endif
.endif
.2byte		\offset
		efe_mem \mem
.endm

/* 0x0d: Call subroutine (jsr) */

/*
 * efe_call: Call subroutine in global memory <offset>
 */
.macro		efe_call offset
.2byte		0xcd0d, \offset, 0xdaad
.endm

/* 0x0e: Call effect function (funcS) */

/*
 * efe_func: Call effect function <id>
 */
.macro		efe_func id
.byte		0x0e, \id
.endm

/* 0x0f: Return from subroutine (return) */

/*
 * efe_ret: Return from subroutine
 */
.macro		efe_ret
.2byte		0xcd0f
.endm

/* 0x10: Pop word from stack to memory (popV) */

/*
 * efe_pop: Pop word from stack to memory <mem>, <offset>
 */
.macro		efe_pop mem, offset
.2byte		0x0010, \offset
		efe_mem \mem
.endm

.set		num_spawn, 0

/* 0x11: Spawn (setFunc) */

/*
 * efe_spawn: Spawn <num> effects in global memory <offset> with size <size>
 */
.macro		efe_spawn num, offset, size
.2byte		(0x0011 | (num_spawn << 8))
.2byte		\num, \offset, 0xdaad, \size, 0x0000
.set		num_spawn, num_spawn + 1
.endm