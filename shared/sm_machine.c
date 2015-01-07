#if !defined(__GNUC__) && !defined(__clang__)
# error Signed overflows in this file depend on GCC/clang behaviour. Check validity.
#endif

#ifdef __cplusplus
#include <cmath>
#define isNaN std::isnan
#define isFinite std::isfinite
#else
#include <math.h>
#define isNaN isnan
#define isFinite isfinite
#endif

#define be8toh(be) (be)

#ifdef __AVR__
static uint16_t be16toh(uint16_t be)
{
	return (be >> 8) | ((be & 0xff) << 8);
}

static uint32_t be32toh(uint32_t be)
{
	return ((uint32_t) be16toh(be >> 16)) | (((uint32_t) be16toh(be & 0xffff)) << 16);
}

static uint64_t be64toh(uint64_t be)
{
	return ((uint64_t) be32toh(be >> 32)) | (((uint64_t) be32toh(be & 0xffffffff)) << 32);
}
#else
#include <endian.h>
#endif

#define LOAD_UNSIGNED(width) ({ \
	uint ## width ## _t tmp; \
	int rc = sm_get_block(at + offset, sizeof(tmp), &tmp); \
	if (rc < 0) goto load_fail; \
	offset += rc; \
	be ## width ## toh(tmp); })
#define LOAD_SIGNED(width) ({ \
	uint ## width ## _t tmp = LOAD_UNSIGNED(width); \
	int ## width ## _t val; \
	memcpy(&val, &tmp, sizeof(val)); \
	val; })

#define DEFINE_SM_LOAD(width) \
	SM_EXPORT(int, sm_get_u ## width)(uint16_t at, uint32_t* u) \
	{ \
		unsigned offset = 0; \
		*u = LOAD_UNSIGNED(width); \
		return sizeof(uint ## width ## _t); \
	load_fail: \
		return -HSE_OOB_READ; \
	} \
	SM_EXPORT(int, sm_get_s ## width)(uint16_t at, int32_t* s) \
	{ \
		unsigned offset = 0; \
		*s = LOAD_SIGNED(width); \
		return sizeof(int ## width ## _t); \
	load_fail: \
		return -HSE_OOB_READ; \
	} \

DEFINE_SM_LOAD(8)
DEFINE_SM_LOAD(16)
DEFINE_SM_LOAD(32)

#undef DEFINE_SM_LOAD

SM_EXPORT(int, sm_get_instruction)(uint16_t at, struct hxb_sm_instruction* op)
{
	uint32_t offset = 0;

	op->opcode = (enum hxb_sm_opcode) LOAD_UNSIGNED(8);

	switch (op->opcode) {
	case HSO_LD_SOURCE_EID:
	case HSO_LD_SOURCE_VAL:
	case HSO_LD_SYSTIME:
	case HSO_OP_MUL:
	case HSO_OP_DIV:
	case HSO_OP_MOD:
	case HSO_OP_ADD:
	case HSO_OP_SUB:
	case HSO_OP_AND:
	case HSO_OP_OR:
	case HSO_OP_XOR:
	case HSO_OP_SHL:
	case HSO_OP_SHR:
	case HSO_CMP_LT:
	case HSO_CMP_LE:
	case HSO_CMP_GT:
	case HSO_CMP_GE:
	case HSO_CMP_EQ:
	case HSO_CMP_NEQ:
	case HSO_CONV_B:
	case HSO_CONV_U8:
	case HSO_CONV_U16:
	case HSO_CONV_U32:
	case HSO_CONV_U64:
	case HSO_CONV_S8:
	case HSO_CONV_S16:
	case HSO_CONV_S32:
	case HSO_CONV_S64:
	case HSO_CONV_F:
	case HSO_WRITE:
	case HSO_RET:
		break;

	case HSO_OP_DUP:
	case HSO_POP:
		op->immed.v_uint = 0;
		break;

	case HSO_OP_ROT:
		op->immed.v_uint = 1;
		break;

	case HSO_LD_FALSE:
		op->immed.type = HXB_DTYPE_BOOL;
		op->immed.v_uint = 0;
		break;

	case HSO_LD_TRUE:
		op->immed.type = HXB_DTYPE_BOOL;
		op->immed.v_uint = 1;
		break;

	case HSO_LD_MEM:
	case HSO_ST_MEM: {
		uint16_t memref = LOAD_UNSIGNED(16);
		op->mem.type = (enum hxb_sm_memtype) (memref >> 12);
		op->mem.addr = memref & 0xfff;
		break;
	}

	case HSO_LD_U8:
		op->immed.type = HXB_DTYPE_UINT8;
		op->immed.v_uint = LOAD_UNSIGNED(8);
		break;

	case HSO_LD_U16:
		op->immed.type = HXB_DTYPE_UINT16;
		op->immed.v_uint = LOAD_UNSIGNED(16);
		break;

	case HSO_LD_U32:
		op->immed.type = HXB_DTYPE_UINT32;
		op->immed.v_uint = LOAD_UNSIGNED(32);
		break;

	case HSO_LD_U64:
		op->immed.type = HXB_DTYPE_UINT64;
		op->immed.v_uint64 = LOAD_UNSIGNED(64);
		break;

	case HSO_LD_S8:
		op->immed.type = HXB_DTYPE_SINT8;
		op->immed.v_sint = LOAD_SIGNED(8);
		break;

	case HSO_LD_S16:
		op->immed.type = HXB_DTYPE_SINT16;
		op->immed.v_sint = LOAD_SIGNED(16);
		break;

	case HSO_LD_S32:
		op->immed.type = HXB_DTYPE_SINT32;
		op->immed.v_sint = LOAD_SIGNED(32);
		break;

	case HSO_LD_S64:
		op->immed.type = HXB_DTYPE_SINT64;
		op->immed.v_sint64 = LOAD_SIGNED(64);
		break;

	case HSO_LD_FLOAT: {
		uint32_t f = LOAD_UNSIGNED(32);
		op->immed.type = HXB_DTYPE_FLOAT;
		memcpy(&op->immed.v_float, &f, sizeof(float));
		break;
	}

	case HSO_OP_DUP_I:
	case HSO_OP_ROT_I:
	case HSO_POP_I:
	case HSO_OP_SWITCH_8:
	case HSO_OP_SWITCH_16:
	case HSO_OP_SWITCH_32:
		op->immed.v_uint = LOAD_UNSIGNED(8);
		break;

	case HSO_OP_DT_DECOMPOSE:
		op->dt_mask = LOAD_UNSIGNED(8);
		break;

	case HSO_CMP_SRC_IP: {
		uint32_t tmp = LOAD_UNSIGNED(8);
		op->block.first = tmp >> 4;
		op->block.last = tmp & 0xF;
		if (op->block.first > op->block.last)
			return -HSE_INVALID_OPCODE;
		tmp = op->block.last - op->block.first + 1;
		if (sm_get_block(at + offset, tmp, op->block.data) < 0)
			return -HSE_INVALID_OPCODE;
		offset += tmp;
		break;
	}

	case HSO_JNZ:
	case HSO_JZ:
	case HSO_JUMP:
		op->jump_skip = LOAD_UNSIGNED(16);
		break;

	default:
		return -HSE_INVALID_OPCODE;
	}

	return offset;

load_fail:
	return -HSE_INVALID_OPCODE;
}

#undef LOAD_SIGNED
#undef LOAD_UNSIGNED

static bool is_int(uint8_t type)
{
	return type >= HXB_DTYPE_BOOL && type < HXB_DTYPE_FLOAT;
}

static int sm_convert(hxb_sm_value_t* val, uint8_t to_type)
{
	if (val->type == to_type)
		return 0;

	switch ((enum hxb_datatype) val->type) {
	case HXB_DTYPE_BOOL:
	case HXB_DTYPE_UINT8:
	case HXB_DTYPE_UINT16:
		val->type = HXB_DTYPE_UINT32;
		break;

	case HXB_DTYPE_SINT8:
	case HXB_DTYPE_SINT16:
		val->type = HXB_DTYPE_SINT32;
		break;

	case HXB_DTYPE_UINT32:
	case HXB_DTYPE_SINT32:
	case HXB_DTYPE_UINT64:
	case HXB_DTYPE_SINT64:
	case HXB_DTYPE_FLOAT:
		break;

	default:
		return -HSE_INVALID_OPERATION;
	}


	if (to_type == HXB_DTYPE_BOOL && val->type == HXB_DTYPE_FLOAT) {
		val->v_uint = isNaN(val->v_float) && val->v_float != 0;
		val->type = HXB_DTYPE_BOOL;
		return 0;
	}
	if (val->type == HXB_DTYPE_FLOAT && !isFinite(val->v_float))
		return -HSE_INVALID_OPERATION;

#define CONVERT_ANY(from) \
	do { \
		switch (to_type) { \
		case HXB_DTYPE_BOOL:   val->v_uint = val->from != 0; break; \
		case HXB_DTYPE_UINT8:  val->v_uint = (uint8_t) val->from; break; \
		case HXB_DTYPE_UINT16: val->v_uint = (uint16_t) val->from; break; \
		case HXB_DTYPE_UINT32: val->v_uint = (uint32_t) val->from; break; \
		case HXB_DTYPE_UINT64: val->v_uint64 = (uint64_t) val->from; break; \
		case HXB_DTYPE_SINT8:  val->v_sint = (int8_t) val->from; break; \
		case HXB_DTYPE_SINT16: val->v_sint = (int16_t) val->from; break; \
		case HXB_DTYPE_SINT32: val->v_sint = (int32_t) val->from; break; \
		case HXB_DTYPE_SINT64: val->v_sint64 = (int64_t) val->from; break; \
		case HXB_DTYPE_FLOAT:  val->v_float = (float) val->from; break; \
		} \
	} while (0)

	switch (val->type) {
	case HXB_DTYPE_UINT32: CONVERT_ANY(v_uint); break;
	case HXB_DTYPE_SINT32: CONVERT_ANY(v_sint); break;
	case HXB_DTYPE_UINT64: CONVERT_ANY(v_uint64); break;
	case HXB_DTYPE_SINT64: CONVERT_ANY(v_sint64); break;
	case HXB_DTYPE_FLOAT: CONVERT_ANY(v_float); break;
	}

#undef CONVERT_ANY

	val->type = to_type;

	return 0;
}

static uint8_t sm_type_rank(uint8_t t)
{
	switch (t) {
	case HXB_DTYPE_BOOL:
		return 0;

	case HXB_DTYPE_UINT8:
	case HXB_DTYPE_SINT8:
		return 1;

	case HXB_DTYPE_UINT16:
	case HXB_DTYPE_SINT16:
		return 2;

	case HXB_DTYPE_UINT32:
	case HXB_DTYPE_SINT32:
		return 3;

	case HXB_DTYPE_UINT64:
	case HXB_DTYPE_SINT64:
		return 4;
	}

	return 0xff;
}

static uint8_t sm_type_width(uint8_t t)
{
	switch (t) {
	case HXB_DTYPE_BOOL:
		return 1;

	case HXB_DTYPE_UINT8:
	case HXB_DTYPE_SINT8:
		return 8;

	case HXB_DTYPE_UINT16:
	case HXB_DTYPE_SINT16:
		return 16;

	case HXB_DTYPE_UINT32:
	case HXB_DTYPE_SINT32:
		return 32;

	case HXB_DTYPE_UINT64:
	case HXB_DTYPE_SINT64:
		return 64;
	}

	return 0xff;
}

static uint8_t sm_common_type(uint8_t t1, uint8_t t2)
{
	if (t1 == HXB_DTYPE_FLOAT || t2 == HXB_DTYPE_FLOAT)
		return HXB_DTYPE_FLOAT;

	bool t1signed = t1 >= HXB_DTYPE_SINT8 && t1 <= HXB_DTYPE_SINT64;
	bool t2signed = t2 >= HXB_DTYPE_SINT8 && t2 <= HXB_DTYPE_SINT64;

	uint8_t t1rank = sm_type_rank(t1);
	uint8_t t2rank = sm_type_rank(t2);
	uint8_t mrank = t1rank > t2rank ? t1rank : t2rank;

	switch (mrank) {
	case 0:
	case 1:
	case 2:
	case 3:
		return (t1signed && t2signed) || (t1signed && !t2signed && t1rank > t2rank) || (!t1signed && t2signed && t1rank < t2rank)
			? HXB_DTYPE_SINT32
			: HXB_DTYPE_UINT32;

	case 4:
		return (t1signed && t2signed) || (t1signed && !t2signed && t1rank > t2rank) || (!t1signed && t2signed && t1rank < t2rank)
			? HXB_DTYPE_SINT64
			: HXB_DTYPE_UINT64;
	}

	return HXB_DTYPE_UNDEFINED;
}

static int sm_arith_op2(hxb_sm_value_t* v1, hxb_sm_value_t* v2, int op)
{
	uint8_t common_type = sm_common_type(v1->type, v2->type);

	if (sm_convert(v1, common_type) < 0)
		return -HSE_INVALID_OPERATION;

	if (sm_convert(v2, common_type) < 0)
		return -HSE_INVALID_OPERATION;

	if (op == HSO_OP_DIV || op == HSO_OP_MOD) {
		switch (common_type) {
		case HXB_DTYPE_UINT32:
			if (v2->v_uint == 0)
				return -HSE_DIV_BY_ZERO;
			break;

		case HXB_DTYPE_UINT64:
			if (v2->v_uint64 == 0)
				return -HSE_DIV_BY_ZERO;
			break;

		case HXB_DTYPE_SINT32:
			if (v2->v_sint == 0)
				return -HSE_DIV_BY_ZERO;
			if (v1->v_sint == INT32_MIN && v2->v_sint == -1) {
				v1->v_sint = op == HSO_OP_DIV ? v1->v_sint : 0;
				return 0;
			}
			break;

		case HXB_DTYPE_SINT64:
			if (v2->v_sint64 == 0)
				return -HSE_DIV_BY_ZERO;
			if (v1->v_sint64 == INT64_MIN && v2->v_sint64 == -1) {
				v1->v_sint64 = op == HSO_OP_DIV ? v1->v_sint64 : 0;
				return 0;
			}
			break;

		case HXB_DTYPE_FLOAT:
			if (op == HSO_OP_DIV)
				break;
			if (v2->v_float == 0 || isinf(v2->v_float))
				return -HSE_DIV_BY_ZERO;
			break;
		}
	}

	switch (op) {
#define ONE_OP(OP) \
	do { \
		switch (common_type) { \
		case HXB_DTYPE_UINT32: v1->v_uint   = v1->v_uint   OP v2->v_uint; break; \
		case HXB_DTYPE_SINT32: v1->v_sint   = v1->v_sint   OP v2->v_sint; break; \
		case HXB_DTYPE_UINT64: v1->v_uint64 = v1->v_uint64 OP v2->v_uint64; break; \
		case HXB_DTYPE_SINT64: v1->v_sint64 = v1->v_sint64 OP v2->v_sint64; break; \
		case HXB_DTYPE_FLOAT:  v1->v_float  = v1->v_float  OP v2->v_float; break; \
		} \
	} while (0)

	case HSO_OP_MUL: ONE_OP(*); break;
	case HSO_OP_DIV: ONE_OP(/); break;
	case HSO_OP_MOD:
		switch (common_type) {
		case HXB_DTYPE_UINT32: v1->v_uint   %= v2->v_uint; break;
		case HXB_DTYPE_SINT32: v1->v_sint   %= v2->v_sint; break;
		case HXB_DTYPE_UINT64: v1->v_uint64 %= v2->v_uint64; break;
		case HXB_DTYPE_SINT64: v1->v_sint64 %= v2->v_sint64; break;
		case HXB_DTYPE_FLOAT:  v1->v_float  = fmodf(v1->v_float, v2->v_float); break;
		}
		break;
	case HSO_OP_ADD: ONE_OP(+); break;
	case HSO_OP_SUB: ONE_OP(-); break;

#undef ONE_OP
#define ONE_OP(OP) \
	do { \
		switch (common_type) { \
		case HXB_DTYPE_UINT32: v1->v_uint = v1->v_uint   OP v2->v_uint; break; \
		case HXB_DTYPE_SINT32: v1->v_uint = v1->v_sint   OP v2->v_sint; break; \
		case HXB_DTYPE_UINT64: v1->v_uint = v1->v_uint64 OP v2->v_uint64; break; \
		case HXB_DTYPE_SINT64: v1->v_uint = v1->v_sint64 OP v2->v_sint64; break; \
		case HXB_DTYPE_FLOAT:  v1->v_uint = v1->v_float  OP v2->v_float; break; \
		} \
		v1->type = HXB_DTYPE_BOOL; \
	} while (0)

	case HSO_CMP_LT:  ONE_OP(<); break;
	case HSO_CMP_LE:  ONE_OP(>=); break;
	case HSO_CMP_GT:  ONE_OP(>); break;
	case HSO_CMP_GE:  ONE_OP(>=); break;
	case HSO_CMP_EQ:  ONE_OP(==); break;
	case HSO_CMP_NEQ: ONE_OP(!=); break;

#undef ONE_OP

	default: return -HSE_INVALID_OPCODE;
	}

	return 0;
}

static int sm_int_op2_bitwise(hxb_sm_value_t* v1, hxb_sm_value_t* v2, int op)
{
	uint8_t common = sm_common_type(v1->type, v2->type);

	if (!is_int(common))
		return -HSE_INVALID_TYPES;

	if (sm_convert(v1, common) < 0)
		return -HSE_INVALID_OPERATION;

	if (sm_convert(v2, common) < 0)
		return -HSE_INVALID_OPERATION;

	switch (op) {
#define ONE_OP(OP) \
	do { \
		switch (common) { \
		case HXB_DTYPE_UINT32: v1->v_uint   = v1->v_uint   OP v2->v_uint; break; \
		case HXB_DTYPE_SINT32: v1->v_sint   = v1->v_sint   OP v2->v_sint; break; \
		case HXB_DTYPE_UINT64: v1->v_uint64 = v1->v_uint64 OP v2->v_uint64; break; \
		case HXB_DTYPE_SINT64: v1->v_sint64 = v1->v_sint64 OP v2->v_sint64; break; \
		} \
	} while (0)

	case HSO_OP_AND: ONE_OP(&); break;
	case HSO_OP_OR:  ONE_OP(|); break;
	case HSO_OP_XOR: ONE_OP(^); break;

#undef ONE_OP

	default: return -HSE_INVALID_OPCODE;
	}

	return 0;
}

static int sm_int_op2_shift(hxb_sm_value_t* v1, hxb_sm_value_t* v2, int op)
{
	uint8_t result_type = sm_common_type(v1->type, HXB_DTYPE_BOOL);
	if (!is_int(result_type))
		return -HSE_INVALID_TYPES;

	uint8_t shamt_type = sm_common_type(v2->type, HXB_DTYPE_BOOL);
	if (!is_int(shamt_type))
		return -HSE_INVALID_TYPES;

	if (sm_convert(v1, result_type) < 0)
		return -HSE_INVALID_OPERATION;

	if (sm_convert(v2, shamt_type) < 0)
		return -HSE_INVALID_OPERATION;

	if (op != HSO_OP_SHL && op != HSO_OP_SHR)
		return -HSE_INVALID_OPCODE;

	uint8_t shamt;
	switch (shamt_type) {
	case HXB_DTYPE_UINT32:
		shamt = v2->v_uint > 64 ? 64 : v2->v_uint;
		break;

	case HXB_DTYPE_UINT64:
		shamt = v2->v_uint64 > 64 ? 64 : v2->v_uint64;
		break;

	case HXB_DTYPE_SINT32:
		if (v2->v_sint < 0) {
			op = op == HSO_OP_SHL ? HSO_OP_SHR : HSO_OP_SHL;
			shamt = v2->v_sint < -64 ? 64 : -v2->v_sint;
		} else {
			shamt = v2->v_sint > 64 ? 64 : v2->v_sint;
		}
		break;

	case HXB_DTYPE_SINT64:
		if (v2->v_sint64 < 0) {
			op = op == HSO_OP_SHL ? HSO_OP_SHR : HSO_OP_SHL;
			shamt = v2->v_sint64 < -64 ? 64 : -v2->v_sint64;
		} else {
			shamt = v2->v_sint64 > 64 ? 64 : v2->v_sint64;
		}
		break;

	default:
		return -HSE_INVALID_OPERATION;
	}

	if (shamt >= sm_type_width(result_type)) {
		switch (result_type) {
		case HXB_DTYPE_UINT32: v1->v_uint = 0; break;
		case HXB_DTYPE_UINT64: v1->v_uint64 = 0; break;
		case HXB_DTYPE_SINT32: v1->v_sint = v1->v_sint < 0 && op == HSO_OP_SHR ? -1 : 0; break;
		case HXB_DTYPE_SINT64: v1->v_sint64 = v1->v_sint64 < 0 && op == HSO_OP_SHR ? -1 : 0; break;
		}

		return 0;
	}

	if (op == HSO_OP_SHL) {
		switch (result_type) {
		case HXB_DTYPE_UINT32: v1->v_uint <<= shamt; break;
		case HXB_DTYPE_UINT64: v1->v_uint64 <<= shamt; break;

		case HXB_DTYPE_SINT32: {
			uint32_t u;

			memcpy(&u, &v1->v_sint, sizeof(u));
			u <<= shamt;
			memcpy(&v1->v_sint, &u, sizeof(u));
			break;
		}

		case HXB_DTYPE_SINT64: {
			uint64_t u;

			memcpy(&u, &v1->v_sint64, sizeof(u));
			u <<= shamt;
			memcpy(&v1->v_sint64, &u, sizeof(u));
			break;
		}
		}
	} else {
		switch (result_type) {
		case HXB_DTYPE_UINT32: v1->v_uint >>= shamt; break;
		case HXB_DTYPE_UINT64: v1->v_uint64 >>= shamt; break;
		case HXB_DTYPE_SINT32: v1->v_sint >>= shamt; break;
		case HXB_DTYPE_SINT64: v1->v_sint64 >>= shamt; break;
		}
	}

	return 0;
}

SM_EXPORT(int, sm_load_mem)(const struct hxb_sm_instruction* insn, hxb_sm_value_t* value)
{
#define CHECK_MEM_SIZE(size) \
	do { \
		if (insn->mem.addr + size > SM_MEMORY_SIZE || insn->mem.addr + size < insn->mem.addr) \
			return -HSE_OOB_READ; \
	} while (0)
#define MEM_OP(field, TYPE, dtype) \
	do { \
		TYPE tmp; \
		CHECK_MEM_SIZE(sizeof(TYPE)); \
		memcpy(&tmp, sm_memory + insn->mem.addr, sizeof(TYPE)); \
		value->field = tmp; \
		value->type = dtype; \
	} while (0)

	switch (insn->mem.type) {
	case HSM_BOOL:  MEM_OP(v_uint, bool, HXB_DTYPE_BOOL); break;
	case HSM_U8:    MEM_OP(v_uint, uint8_t, HXB_DTYPE_UINT8); break;
	case HSM_U16:   MEM_OP(v_uint, uint16_t, HXB_DTYPE_UINT16); break;
	case HSM_U32:   MEM_OP(v_uint, uint32_t, HXB_DTYPE_UINT32); break;
	case HSM_U64:   MEM_OP(v_uint64, uint64_t, HXB_DTYPE_UINT64); break;
	case HSM_S8:    MEM_OP(v_sint, int8_t, HXB_DTYPE_SINT8); break;
	case HSM_S16:   MEM_OP(v_sint, int16_t, HXB_DTYPE_SINT16); break;
	case HSM_S32:   MEM_OP(v_sint, int32_t, HXB_DTYPE_SINT32); break;
	case HSM_S64:   MEM_OP(v_sint64, int64_t, HXB_DTYPE_SINT64); break;
	case HSM_FLOAT: MEM_OP(v_float, float, HXB_DTYPE_FLOAT); break;
	}

	return 0;

#undef CHECK_MEM_SIZE
#undef MEM_OP
}

SM_EXPORT(int, sm_store_mem)(const struct hxb_sm_instruction* insn, hxb_sm_value_t value)
{
#define CHECK_MEM_SIZE(size) \
	do { \
		if (insn->mem.addr + size > SM_MEMORY_SIZE || insn->mem.addr + size < insn->mem.addr) \
			return -HSE_OOB_WRITE; \
	} while (0)
#define MEM_OP(field, type) \
	do { \
		const type tmp = value.field; \
		CHECK_MEM_SIZE(sizeof(type)); \
		memcpy(sm_memory + insn->mem.addr, &tmp, sizeof(type)); \
	} while (0)

	if (sm_convert(&value, insn->mem.type) < 0)
		return -HSE_INVALID_OPERATION;

	switch (insn->mem.type) {
	case HSM_BOOL:  MEM_OP(v_uint, bool); break;
	case HSM_U8:    MEM_OP(v_uint, uint8_t); break;
	case HSM_U16:   MEM_OP(v_uint, uint16_t); break;
	case HSM_U32:   MEM_OP(v_uint, uint32_t); break;
	case HSM_U64:   MEM_OP(v_uint64, uint64_t); break;
	case HSM_S8:    MEM_OP(v_sint, int8_t); break;
	case HSM_S16:   MEM_OP(v_sint, int16_t); break;
	case HSM_S32:   MEM_OP(v_sint, int32_t); break;
	case HSM_S64:   MEM_OP(v_sint64, int64_t); break;
	case HSM_FLOAT: MEM_OP(v_float, float); break;
	}

	return 0;

#undef CHECK_MEM_SIZE
#undef MEM_OP
}

SM_EXPORT(int, run_sm)(const char* src_ip, uint32_t eid, const hxb_sm_value_t* val)
{
	uint16_t addr = 0;
	int8_t top = -1;
	struct hxb_sm_instruction insn;
	int ret = 0;

#define FAIL_WITH(code) \
	do { \
		ret = code; \
		sm_diag_msg(ret, __PRETTY_FUNCTION__, __LINE__); \
		goto fail; \
	} while (0)
#define FAIL_AS(...) \
	do { \
		ret = __VA_ARGS__; \
		if (ret < 0) \
			FAIL_WITH(-ret); \
	} while (0)

#define CHECK_PUSH() \
	do { \
		if (top >= SM_STACK_SIZE) \
			FAIL_WITH(HSE_STACK_ERROR); \
	} while (0)
#define CHECK_POP(n) \
	do { \
		if (top + 1 < n) \
			FAIL_WITH(HSE_STACK_ERROR); \
	} while (0)
#define PUSH(dtype, member, value) \
	do { \
		CHECK_PUSH(); \
		top++; \
		TOP.type = dtype; \
		TOP.member = value; \
	} while (0)
#define PUSH_V(val) \
	do { \
		CHECK_PUSH(); \
		top++; \
		TOP = val; \
	} while (0)
#define TOP_N(n) (sm_stack[top - (n)])
#define TOP TOP_N(0)

	if (sm_first_run) {
		memset(sm_memory, 0, SM_MEMORY_SIZE);
	}

again:
	{
		uint32_t val;

		FAIL_AS(sm_get_u8(0, &val));
		if (val == 0xFF)
			goto end_program;
		if (val != 0)
			FAIL_WITH(HSE_INVALID_HEADER);

		if (sm_first_run)
			FAIL_AS(sm_get_u16(1 + HSVO_INIT, &val));
		else if (src_ip)
			FAIL_AS(sm_get_u16(1 + HSVO_PACKET, &val));
		else
			FAIL_AS(sm_get_u16(1 + HSVO_PERIODIC, &val));

		addr = val;
	}

	if (addr == 0xFFFF)
		goto end_program;

	for (;;) {
		int insn_length = sm_get_instruction(addr, &insn);
		uint16_t jump_skip = 0;

		FAIL_AS(insn_length);

		switch (insn.opcode) {
		case HSO_LD_SOURCE_EID:
			if (!src_ip || !val)
				FAIL_WITH(HSE_INVALID_OPERATION);
			PUSH(HXB_DTYPE_UINT32, v_uint, eid);
			break;

		case HSO_LD_SOURCE_VAL:
			if (!src_ip || !val)
				FAIL_WITH(HSE_INVALID_OPERATION);
			PUSH_V(*val);
			break;

		case HSO_LD_FALSE:
		case HSO_LD_TRUE:
		case HSO_LD_U8:
		case HSO_LD_U16:
		case HSO_LD_U32:
		case HSO_LD_U64:
		case HSO_LD_S8:
		case HSO_LD_S16:
		case HSO_LD_S32:
		case HSO_LD_S64:
		case HSO_LD_FLOAT:
			PUSH_V(insn.immed);
			break;

		case HSO_LD_SYSTIME:
			PUSH(HXB_DTYPE_SINT64, v_sint64, sm_get_systime());
			break;

		case HSO_LD_MEM:
			PUSH(HXB_DTYPE_BOOL, v_uint, 0);
			FAIL_AS(sm_load_mem(&insn, &TOP));
			break;

		case HSO_ST_MEM:
			CHECK_POP(1);
			FAIL_AS(sm_store_mem(&insn, TOP));
			top--;
			break;

		case HSO_OP_MUL:
		case HSO_OP_DIV:
		case HSO_OP_MOD:
		case HSO_OP_ADD:
		case HSO_OP_SUB:
		case HSO_CMP_LT:
		case HSO_CMP_LE:
		case HSO_CMP_GT:
		case HSO_CMP_GE:
		case HSO_CMP_EQ:
		case HSO_CMP_NEQ:
			CHECK_POP(2);
			FAIL_AS(sm_arith_op2(&TOP_N(1), &TOP_N(0), insn.opcode));
			top--;
			break;

		case HSO_OP_AND:
		case HSO_OP_OR:
		case HSO_OP_XOR:
			CHECK_POP(2);
			FAIL_AS(sm_int_op2_bitwise(&TOP_N(1), &TOP_N(0), insn.opcode));
			top--;
			break;

		case HSO_OP_SHL:
		case HSO_OP_SHR:
			CHECK_POP(2);
			FAIL_AS(sm_int_op2_shift(&TOP_N(1), &TOP_N(0), insn.opcode));
			top--;
			break;

		case HSO_OP_DUP:
		case HSO_OP_DUP_I: {
			CHECK_PUSH();

			uint8_t offset = insn.immed.v_uint;

			CHECK_POP(offset + 1);
			hxb_sm_value_t val = TOP_N(offset);
			PUSH_V(val);
			break;
		}

		case HSO_OP_ROT:
		case HSO_OP_ROT_I:
		case HSO_POP:
		case HSO_POP_I: {
			uint8_t offset = insn.immed.v_uint;

			CHECK_POP(offset + 1);
			hxb_sm_value_t val = TOP_N(offset);
			memmove(sm_stack + top - offset, sm_stack + top - offset + 1, offset * sizeof(sm_stack[0]));
			if (insn.opcode == HSO_POP || insn.opcode == HSO_POP_I)
				top--;
			else
				TOP = val;
			break;
		}

		case HSO_OP_DT_DECOMPOSE: {
			CHECK_POP(1);

			if (!is_int(TOP.type))
				FAIL_WITH(HSE_INVALID_TYPES);

			FAIL_AS(sm_convert(&TOP, HXB_DTYPE_SINT64) < 0);

			int64_t t = TOP.v_sint64;
			struct {
				int8_t tm_sec;
				int8_t tm_min;
				int8_t tm_hour;
				int8_t tm_mday;
				int8_t tm_mon;
				int32_t tm_year;
				int8_t tm_wday;
			} tm;

			top--;

			/* this is taken from __secs_to_tm of musl libc, since avr libc doesn't
			 * seem to provide localtime_r. the documentation says it's there, but it
			 * actually isn't in ubuntu 14.04LTS packages ... */

			/* 2000-03-01 (mod 400 year, immediately after feb29 */
			#define LEAPOCH (946684800LL + 86400*(31+29))

			#define DAYS_PER_400Y (365UL*400 + 97)
			#define DAYS_PER_100Y (365UL*100 + 24)
			#define DAYS_PER_4Y   (365UL*4   + 1)

			{
				int64_t days, secs;
				int32_t remdays, remsecs, remyears;
				int32_t qc_cycles, c_cycles, q_cycles;
				int32_t years, months;
				int32_t wday, yday, leap;
				const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

				/* Reject time_t values whose year would overflow int32 */
				if (t < INT32_MIN * 31622400LL || t > INT32_MAX * 31622400LL)
					FAIL_WITH(HSE_INVALID_OPERATION);

				secs = t - LEAPOCH;
				days = secs / 86400;
				remsecs = secs % 86400;
				if (remsecs < 0) {
					remsecs += 86400;
					days--;
				}

				wday = (3+days)%7;
				if (wday < 0) wday += 7;

				qc_cycles = days / DAYS_PER_400Y;
				remdays = days % DAYS_PER_400Y;
				if (remdays < 0) {
					remdays += DAYS_PER_400Y;
					qc_cycles--;
				}

				c_cycles = remdays / DAYS_PER_100Y;
				if (c_cycles == 4) c_cycles--;
				remdays -= c_cycles * DAYS_PER_100Y;

				q_cycles = remdays / DAYS_PER_4Y;
				if (q_cycles == 25) q_cycles--;
				remdays -= q_cycles * DAYS_PER_4Y;

				remyears = remdays / 365;
				if (remyears == 4) remyears--;
				remdays -= remyears * 365;

				leap = !remyears && (q_cycles || !c_cycles);
				yday = remdays + 31 + 28 + leap;
				if (yday >= 365+leap) yday -= 365+leap;

				years = remyears + 4*q_cycles + 100*c_cycles + 400*qc_cycles;

				for (months=0; days_in_month[months] <= remdays; months++)
					remdays -= days_in_month[months];

				if (years+100 > INT32_MAX || years+100 < INT32_MIN)
					FAIL_WITH(HSE_INVALID_OPERATION);

				tm.tm_year = years + 100;
				tm.tm_mon = months + 2;
				if (tm.tm_mon >= 12) {
					tm.tm_mon -=12;
					tm.tm_year++;
				}
				tm.tm_mday = remdays + 1;
				tm.tm_wday = wday;

				tm.tm_hour = remsecs / 3600;
				tm.tm_min = remsecs / 60 % 60;
				tm.tm_sec = remsecs % 60;
			}

			/* end musl code */
			#undef LEAPOCH
			#undef DAYS_PER_400Y
			#undef DAYS_PER_100Y
			#undef DAYS_PER_4Y

			if (insn.dt_mask & HSDM_WEEKDAY)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_wday);
			if (insn.dt_mask & HSDM_YEAR)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_year);
			if (insn.dt_mask & HSDM_MONTH)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_mon);
			if (insn.dt_mask & HSDM_DAY)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_mday);
			if (insn.dt_mask & HSDM_HOUR)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_hour);
			if (insn.dt_mask & HSDM_MINUTE)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_min);
			if (insn.dt_mask & HSDM_SECOND)
				PUSH(HXB_DTYPE_SINT32, v_sint, tm.tm_sec);

			break;
		}

		case HSO_OP_SWITCH_8:
		case HSO_OP_SWITCH_16:
		case HSO_OP_SWITCH_32: {
			CHECK_POP(1);

			if (!is_int(TOP.type))
				FAIL_WITH(HSE_INVALID_TYPES);

			bool is_signed = TOP.type >= HXB_DTYPE_SINT8 && TOP.type <= HXB_DTYPE_SINT64;
			FAIL_AS(sm_convert(&TOP, HXB_DTYPE_UINT64));

			uint16_t offset = insn_length;

			if (insn.opcode == HSO_OP_SWITCH_8) {
				insn_length += insn.immed.v_uint * (1 + 2);
			} else if (insn.opcode == HSO_OP_SWITCH_16) {
				insn_length += insn.immed.v_uint * (2 + 2);
			} else {
				insn_length += insn.immed.v_uint * (4 + 2);
			}

			for (uint16_t i = 0; i < insn.immed.v_uint; i++) {
				uint64_t label;
				uint32_t jump_val;

				int rc = -1;
#define LOAD_LABEL(width) \
	do { \
		if (is_signed) { \
			int32_t b = 0; \
			rc = sm_get_s ## width(addr + offset, &b); \
			label = b; \
		} else { \
			uint32_t b = 0; \
			rc = sm_get_u ## width(addr + offset, &b); \
			label = b; \
		} \
	} while (0)
				if (insn.opcode == HSO_OP_SWITCH_8) {
					LOAD_LABEL(8);
				} else if (insn.opcode == HSO_OP_SWITCH_16) {
					LOAD_LABEL(16);
				} else {
					LOAD_LABEL(32);
				}
#undef LOAD_LABEL
				if (rc < 0)
					FAIL_WITH(HSE_INVALID_OPCODE);
				offset += rc;

				rc = sm_get_u16(addr + offset, &jump_val);
				if (rc < 0)
					FAIL_WITH(HSE_INVALID_OPCODE);
				offset += rc;

				if (TOP.v_uint64 == label) {
					jump_skip = jump_val;
					break;
				}
			}
			top--;

			break;
		}

		case HSO_CMP_SRC_IP:
			if (!src_ip || !val)
				FAIL_WITH(HSE_INVALID_OPERATION);

			PUSH(HXB_DTYPE_BOOL, v_uint,
				memcmp(src_ip + insn.block.first, insn.block.data, insn.block.last - insn.block.first + 1) == 0);
			break;

		case HSO_JNZ:
		case HSO_JZ: {
			CHECK_POP(1);

			FAIL_AS(sm_convert(&TOP, HXB_DTYPE_BOOL));

			bool wanted = insn.opcode == HSO_JZ ? false : true;

			if (TOP.v_uint == wanted)
				jump_skip = insn.jump_skip;

			top--;
			break;
		}

		case HSO_JUMP:
			jump_skip = insn.jump_skip;
			break;

		case HSO_WRITE: {
			CHECK_POP(2);

			uint32_t write_eid = TOP_N(0).v_uint;
			hxb_sm_value_t write_val = TOP_N(1);

			enum hxb_error_code err = (enum hxb_error_code) sm_endpoint_write(write_eid, &write_val);

			if (err)
				FAIL_WITH(HSE_WRITE_FAILED);

			top -= 2;
			break;
		}

		case HSO_CONV_B:
		case HSO_CONV_U8:
		case HSO_CONV_U16:
		case HSO_CONV_U32:
		case HSO_CONV_U64:
		case HSO_CONV_S8:
		case HSO_CONV_S16:
		case HSO_CONV_S32:
		case HSO_CONV_S64:
		case HSO_CONV_F: {
			CHECK_POP(1);

			uint8_t to = HXB_DTYPE_BOOL + (insn.opcode - HSO_CONV_B);
			FAIL_AS(sm_convert(&TOP, to));
			break;
		}

		case HSO_RET:
			goto end_program;
		}

		if (addr + insn_length + jump_skip < addr)
			FAIL_WITH(HSE_INVALID_OPERATION);
		addr += insn_length + jump_skip;
	}

end_program:
	ret = 0;
	if (sm_first_run) {
		sm_first_run = false;
		goto again;
	}
	return ret;
fail:
	sm_first_run = false;
	return ret;

#undef FAIL_WITH
#undef FAIL_AS
#undef CHECK_PUSH
#undef CHECK_POP
#undef PUSH
#undef PUSH_V
#undef TOP_N
#undef TOP
}

#undef be8toh
#undef isNaN
#undef isFinite
