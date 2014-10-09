#ifndef SHARED_SM__TYPES_H_FBE69B5E7D88EBB1
#define SHARED_SM__TYPES_H_FBE69B5E7D88EBB1

enum hxb_sm_opcode {
	HSO_LD_SOURCE_IP,
	HSO_LD_SOURCE_EID,
	HSO_LD_SOURCE_VAL,
	HSO_LD_FALSE,
	HSO_LD_TRUE,
	HSO_LD_U8,
	HSO_LD_U16,
	HSO_LD_U32,
	HSO_LD_U64,
	HSO_LD_S8,
	HSO_LD_S16,
	HSO_LD_S32,
	HSO_LD_S64,
	HSO_LD_FLOAT,
	HSO_LD_SYSTIME,

	HSO_LD_MEM,
	HSO_ST_MEM,

	HSO_OP_MUL,
	HSO_OP_DIV,
	HSO_OP_MOD,
	HSO_OP_ADD,
	HSO_OP_SUB,
	HSO_OP_AND,
	HSO_OP_OR,
	HSO_OP_XOR,
	HSO_OP_SHL,
	HSO_OP_SHR,
	HSO_OP_DUP,
	HSO_OP_DUP_I,
	HSO_OP_ROT,
	HSO_OP_ROT_I,
	HSO_OP_DT_DECOMPOSE,
	HSO_OP_SWITCH_8,
	HSO_OP_SWITCH_16,
	HSO_OP_SWITCH_32,

	HSO_CMP_BLOCK,
	HSO_CMP_IP_LO,
	HSO_CMP_LT,
	HSO_CMP_LE,
	HSO_CMP_GT,
	HSO_CMP_GE,
	HSO_CMP_EQ,
	HSO_CMP_NEQ,

	// keep these in order with the corresponding HXB_DTYPE_*
	HSO_CONV_B,
	HSO_CONV_U8,
	HSO_CONV_U16,
	HSO_CONV_U32,
	HSO_CONV_U64,
	HSO_CONV_S8,
	HSO_CONV_S16,
	HSO_CONV_S32,
	HSO_CONV_S64,
	HSO_CONV_F,

	HSO_JNZ,
	HSO_JZ,
	HSO_JUMP,

	HSO_WRITE,

	HSO_POP,
	HSO_EXCHANGE,

	HSO_RET,
};

enum hxb_sm_dtmask {
	HSDM_SECOND  = 1,
	HSDM_MINUTE  = 2,
	HSDM_HOUR    = 4,
	HSDM_DAY     = 8,
	HSDM_MONTH   = 16,
	HSDM_YEAR    = 32,
	HSDM_WEEKDAY = 64,
};

enum hxb_sm_memtype {
	// keep these in order with the corresponding HXB_DTYPE_*
	HSM_BOOL,
	HSM_U8,
	HSM_U16,
	HSM_U32,
	HSM_U64,
	HSM_S8,
	HSM_S16,
	HSM_S32,
	HSM_S64,
	HSM_FLOAT,
};

struct hxb_sm_instruction {
	enum hxb_sm_opcode opcode;

	hxb_sm_value_t immed;
	uint8_t dt_mask;
	uint16_t jump_skip;
	struct {
		uint8_t first;
		uint8_t last;
		char data[16];
	} block;
	struct {
		enum hxb_sm_memtype type;
		uint16_t addr;
	} mem;
};

enum hxb_sm_error {
	HSE_SUCCESS = 0,

	HSE_OOB_READ,
	HSE_OOB_WRITE,
	HSE_INVALID_OPCODE,
	HSE_INVALID_TYPES,
	HSE_DIV_BY_ZERO,
	HSE_INVALID_HEADER,
	HSE_INVALID_OPERATION,
	HSE_STACK_ERROR,
	HSE_WRITE_FAILED,
};

enum hxb_sm_vector_offsets {
	HSVO_INIT     = 0x0000,
	HSVO_PACKET   = 0x0002,
	HSVO_PERIODIC = 0x0004,
};

#endif
