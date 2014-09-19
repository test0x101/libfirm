/*
 * This file is part of libFirm.
 * Copyright (C) 2012 University of Karlsruhe.
 */

/**
 * @file
 * @brief    The main TEMPLATE backend driver file.
 */
#include "bearch_TEMPLATE_t.h"

#include "irgwalk.h"
#include "irprog.h"
#include "ircons.h"
#include "irdump.h"
#include "irgmod.h"
#include "lower_calls.h"
#include "lower_builtins.h"
#include "debug.h"
#include "panic.h"
#include "be_t.h"
#include "bearch.h"
#include "benode.h"
#include "belower.h"
#include "besched.h"
#include "bemodule.h"
#include "begnuas.h"
#include "belistsched.h"
#include "bestack.h"
#include "bespillutil.h"

#include "TEMPLATE_new_nodes.h"
#include "gen_TEMPLATE_regalloc_if.h"
#include "TEMPLATE_transform.h"
#include "TEMPLATE_emitter.h"

DEBUG_ONLY(static firm_dbg_module_t *dbg = NULL;)

static ir_entity *TEMPLATE_get_frame_entity(const ir_node *node)
{
	(void)node;
	/* TODO: return the ir_entity assigned to the frame */
	return NULL;
}

/**
 * This function is called by the generic backend to correct offsets for
 * nodes accessing the stack.
 */
static void TEMPLATE_set_frame_offset(ir_node *irn, int offset)
{
	(void)irn;
	(void)offset;
	/* TODO: correct offset if irn accesses the stack */
}

static int TEMPLATE_get_sp_bias(const ir_node *irn)
{
	(void)irn;
	return 0;
}

/* fill register allocator interface */

static const arch_irn_ops_t TEMPLATE_irn_ops = {
	.get_frame_entity = TEMPLATE_get_frame_entity,
	.set_frame_offset = TEMPLATE_set_frame_offset,
	.get_sp_bias      = TEMPLATE_get_sp_bias,
};

/**
 * Transforms the standard firm graph into
 * a TEMLPATE firm graph
 */
static void TEMPLATE_prepare_graph(ir_graph *irg)
{
	/* transform nodes into assembler instructions */
	be_timer_push(T_CODEGEN);
	TEMPLATE_transform_graph(irg);
	be_timer_pop(T_CODEGEN);
	be_dump(DUMP_BE, irg, "code-selection");
}



/**
 * Last touchups and emitting of the generated code of a function.
 */
static void TEMPLATE_emit(ir_graph *irg)
{
	/* fix stack entity offsets */
	be_abi_fix_stack_nodes(irg);
	be_abi_fix_stack_bias(irg);
	/* emit code */
	TEMPLATE_emit_function(irg);
}


static void TEMPLATE_before_ra(ir_graph *irg)
{
	(void)irg;
	/* Some stuff you need to do after scheduling but before register allocation */
}


extern const arch_isa_if_t TEMPLATE_isa_if;
static TEMPLATE_isa_t TEMPLATE_isa_template = {
	{
		&TEMPLATE_isa_if,            /* isa interface implementation */
		N_TEMPLATE_REGISTERS,
		TEMPLATE_registers,
		N_TEMPLATE_CLASSES,
		TEMPLATE_reg_classes,
		&TEMPLATE_registers[REG_SP], /* stack pointer register */
		&TEMPLATE_registers[REG_BP], /* base pointer register */
		2,                           /* power of two stack alignment for calls, 2^2 == 4 */
		7,                           /* costs for a spill instruction */
		5,                           /* costs for a reload instruction */
	},
};

static void TEMPLATE_init(void)
{
	TEMPLATE_register_init();
	TEMPLATE_create_opcodes(&TEMPLATE_irn_ops);
}

static void TEMPLATE_finish(void)
{
	TEMPLATE_free_opcodes();
}

static arch_env_t *TEMPLATE_begin_codegeneration(void)
{
	TEMPLATE_isa_t *isa = XMALLOC(TEMPLATE_isa_t);
	*isa = TEMPLATE_isa_template;

	return &isa->base;
}

/**
 * Closes the output file and frees the ISA structure.
 */
static void TEMPLATE_end_codegeneration(void *self)
{
	free(self);
}

static void TEMPLATE_lower_for_target(void)
{
	lower_builtins(0, NULL);
	be_after_irp_transform("lower-builtins");

	/* lower compound param handling */
	lower_calls_with_compounds(LF_RETURN_HIDDEN);
	be_after_irp_transform("lower-calls");
}

static int TEMPLATE_is_mux_allowed(ir_node *sel, ir_node *mux_false,
                                   ir_node *mux_true)
{
	(void)sel;
	(void)mux_false;
	(void)mux_true;
	return false;
}

/**
 * Returns the libFirm configuration parameter for this backend.
 */
static const backend_params *TEMPLATE_get_backend_params(void)
{
	static backend_params p = {
		false, /* false: little-endian, true: big-endian */
		false, /* PIC code supported */
		false, /* unaligned memory access supported */
		32,    /* modulo_shift */
		NULL,  /* architecture dependent settings, will be set later */
		TEMPLATE_is_mux_allowed,  /* parameter for if conversion */
		32,    /* machine size - a 32bit CPU */
		NULL,  /* float arithmetic mode */
		NULL,  /* long long type */
		NULL,  /* unsigned long long type */
		NULL,  /* long double type */
		4,     /* alignment of stack parameter: typically 4 (32bit) or 8 (64bit) */
		ir_overflow_min_max
	};
	return &p;
}

static int TEMPLATE_is_valid_clobber(const char *clobber)
{
	(void)clobber;
	return false;
}

static ir_node *TEMPLATE_new_spill(ir_node *value, ir_node *after)
{
	(void)value;
	(void)after;
	panic("TEMPLATE: spilling not implemented yet");
}

static ir_node *TEMPLATE_new_reload(ir_node *value, ir_node *spill,
                                    ir_node *before)
{
	(void)value;
	(void)spill;
	(void)before;
	panic("TEMPLATE: reload not implemented yet");
}

const arch_isa_if_t TEMPLATE_isa_if = {
	TEMPLATE_init,
	TEMPLATE_finish,
    TEMPLATE_get_backend_params,
	TEMPLATE_lower_for_target,
	TEMPLATE_is_valid_clobber,

	TEMPLATE_begin_codegeneration,
	TEMPLATE_end_codegeneration,
	NULL,
	NULL, /* mark remat */
	TEMPLATE_new_spill,
	TEMPLATE_new_reload,
	NULL,

	NULL, /* handle intrinsics */
	TEMPLATE_prepare_graph,
	TEMPLATE_before_ra,
	TEMPLATE_emit,
};

BE_REGISTER_MODULE_CONSTRUCTOR(be_init_arch_TEMPLATE)
void be_init_arch_TEMPLATE(void)
{
	be_register_isa_if("TEMPLATE", &TEMPLATE_isa_if);
	FIRM_DBG_REGISTER(dbg, "firm.be.TEMPLATE.cg");
	TEMPLATE_init_transform();
}
