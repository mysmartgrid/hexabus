int SM_EXPORT(sm_get_instruction)(uint16_t at, struct hxb_sm_instruction* op)
{
	uint32_t offset = 0;

#define LOAD(type) ({ \
	uint32_t tmp; \
	int rc = sm_get_##type(at + offset, &tmp); \
	if (rc < 0) return -HSE_INVALID_OPCODE; \
	offset += rc; \
	tmp; })

	op->opcode = (enum hxb_sm_opcode) LOAD(u8);

	switch (op->opcode) {
	case HSO_LD_SOURCE_IP:
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
	case HSO_OP_NOT:
	case HSO_OP_SHL:
	case HSO_OP_SHR:
	case HSO_CMP_IP_LO:
	case HSO_CMP_LT:
	case HSO_CMP_LE:
	case HSO_CMP_GT:
	case HSO_CMP_GE:
	case HSO_CMP_EQ:
	case HSO_CMP_NEQ:
	case HSO_CONV_B:
	case HSO_CONV_U8:
	case HSO_CONV_U32:
	case HSO_CONV_U64:
	case HSO_CONV_F:
	case HSO_WRITE:
	case HSO_POP:
	case HSO_RET:
		break;

	case HSO_OP_DUP:
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
		uint16_t memref = LOAD(u16);
		op->mem.type = (enum hxb_sm_memtype) (memref >> 12);
		op->mem.addr = memref & 0xfff;
		break;
	}

	case HSO_LD_U8:
		op->immed.type = HXB_DTYPE_UINT8;
		op->immed.v_uint = LOAD(u8);
		break;

	case HSO_LD_U16:
		op->immed.type = HXB_DTYPE_UINT32;
		op->immed.v_uint = LOAD(u16);
		break;

	case HSO_LD_U32:
		op->immed.type = HXB_DTYPE_UINT32;
		op->immed.v_uint = LOAD(u32);
		break;

	case HSO_LD_U64:
		op->immed.type = HXB_DTYPE_UINT64;
		op->immed.v_uint64 = LOAD(u32);
		op->immed.v_uint64 <<= 32;
		op->immed.v_uint64 |= LOAD(u32);
		break;

	case HSO_LD_FLOAT: {
		uint32_t f = LOAD(u32);
		op->immed.type = HXB_DTYPE_FLOAT;
		memcpy(&op->immed.v_float, &f, sizeof(float));
		break;
	}

	case HSO_OP_DUP_I:
	case HSO_OP_ROT_I:
	case HSO_EXCHANGE:
	case HSO_OP_SWITCH_8:
	case HSO_OP_SWITCH_16:
	case HSO_OP_SWITCH_32:
		op->immed.v_uint = LOAD(u8);
		break;

	case HSO_OP_DT_DECOMPOSE:
		op->dt_mask = LOAD(u8);
		break;

	case HSO_CMP_BLOCK: {
		uint32_t tmp = LOAD(u8);
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
		op->jump_skip = LOAD(u16);
		break;

	default:
		return -HSE_INVALID_OPCODE;
	}

	return offset;

#undef LOAD
}

static bool is_int(const hxb_sm_value_t* v)
{
	return v->type == HXB_DTYPE_BOOL ||
		v->type == HXB_DTYPE_UINT8 ||
		v->type == HXB_DTYPE_UINT32 ||
		v->type == HXB_DTYPE_UINT64;
}

static bool is_arithmetic(const hxb_sm_value_t* v)
{
	return is_int(v) || v->type == HXB_DTYPE_FLOAT;
}

static int sm_convert(hxb_sm_value_t* val, uint8_t to_type)
{
	if (to_type != HXB_DTYPE_BOOL &&
		to_type != HXB_DTYPE_UINT8 &&
		to_type != HXB_DTYPE_UINT32 &&
		to_type != HXB_DTYPE_UINT64 &&
		to_type != HXB_DTYPE_FLOAT)
		return -HSE_INVALID_TYPES;

	if (val->type == to_type)
		return 0;

	switch (val->type) {
	case HXB_DTYPE_BOOL:
	case HXB_DTYPE_UINT8:
	case HXB_DTYPE_UINT32:
		switch (to_type) {
		case HXB_DTYPE_BOOL:   val->v_uint   = !!val->v_uint; break;
		case HXB_DTYPE_UINT8:  val->v_uint   = (uint8_t) val->v_uint; break;
		case HXB_DTYPE_UINT32: val->v_uint   = (uint32_t) val->v_uint; break;
		case HXB_DTYPE_UINT64: val->v_uint64 = val->v_uint; break;
		case HXB_DTYPE_FLOAT:  val->v_float  = val->v_uint; break;
		}
		break;

	case HXB_DTYPE_UINT64:
		switch (to_type) {
		case HXB_DTYPE_BOOL:   val->v_uint  = !!val->v_uint64; break;
		case HXB_DTYPE_UINT8:  val->v_uint  = (uint8_t) val->v_uint64; break;
		case HXB_DTYPE_UINT32: val->v_uint  = (uint32_t) val->v_uint64; break;
		case HXB_DTYPE_FLOAT:  val->v_float = val->v_uint64; break;
		}
		break;

	case HXB_DTYPE_FLOAT:
		switch (to_type) {
		case HXB_DTYPE_BOOL:   val->v_uint   = val->v_float != 0; break;
		case HXB_DTYPE_UINT8:  val->v_uint   = (uint8_t) val->v_float; break;
		case HXB_DTYPE_UINT32: val->v_uint   = (uint32_t) val->v_float; break;
		case HXB_DTYPE_UINT64: val->v_uint64 = (uint64_t) val->v_float; break;
		}
		break;

	default: return -HSE_INVALID_OPCODE;
	}

	val->type = to_type;

	return 0;
}

static int sm_arith_op2(hxb_sm_value_t* v1, hxb_sm_value_t* v2, int op)
{
	if (!is_arithmetic(v1) || !is_arithmetic(v2))
		return -HSE_INVALID_TYPES;

	/* widen v1 if v2 is of greater rank than v1, this covers integer
	 * conversion ranks and int<float for all ints */
	uint8_t common_type = v1->type;
	if (common_type < v2->type)
		common_type = v2->type;
	if (common_type < HXB_DTYPE_UINT32)
		common_type = HXB_DTYPE_UINT32;

	bool is_float = common_type == HXB_DTYPE_FLOAT;
	bool is_u64 = common_type == HXB_DTYPE_UINT64;

	int res = sm_convert(v1, common_type);
	if (res < 0)
		return res;

	res = sm_convert(v2, common_type);
	if (res < 0)
		return res;

	switch (op) {
#define ONE_OP(OP) \
		if (is_float) \
			v1->v_float = v1->v_float OP v2->v_float; \
		else if (is_u64) \
			v1->v_uint64 = v1->v_uint64 OP v2->v_uint64; \
		else \
			v1->v_uint = v1->v_uint OP v2->v_uint; \
		break;

	case HSO_OP_MUL: ONE_OP(*)
	case HSO_OP_DIV:
		if ((is_u64 && v2->v_uint64 == 0)
				|| (!is_u64 && !is_float && v2->v_uint == 0))
			return -HSE_DIV_BY_ZERO;
		ONE_OP(/)
	case HSO_OP_MOD:
		if ((is_u64 && v2->v_uint64 == 0)
				|| (!is_u64 && !is_float && v2->v_uint == 0)
				|| (is_float && (v2->v_float == 0 || isinf(v2->v_float))))
			return -HSE_DIV_BY_ZERO;
		if (is_float)
			v1->v_float = fmodf(v1->v_float, v2->v_float);
		else if (is_u64)
			v1->v_uint64 = v1->v_uint64 % v2->v_uint64;
		else
			v1->v_uint = v1->v_uint % v2->v_uint;
		break;
	case HSO_OP_ADD: ONE_OP(+)
	case HSO_OP_SUB: ONE_OP(-)

#undef ONE_OP
#define ONE_OP(OP) \
		v1->v_uint = is_float \
			? v1->v_float OP v2->v_float \
			: is_u64 \
				? v1->v_uint64 OP v1->v_uint64 \
				: v1->v_uint OP v1->v_uint; \
		v1->type = HXB_DTYPE_BOOL; \
		break;

	case HSO_CMP_LT: ONE_OP(<)
	case HSO_CMP_LE: ONE_OP(>=)
	case HSO_CMP_GT: ONE_OP(>)
	case HSO_CMP_GE: ONE_OP(>=)
	case HSO_CMP_EQ: ONE_OP(==)
	case HSO_CMP_NEQ: ONE_OP(!=)

#undef ONE_OP

	default: return -HSE_INVALID_OPCODE;
	}

	return 0;
}

static int sm_int_op2(hxb_sm_value_t* v1, hxb_sm_value_t* v2, int op)
{
	if (!is_int(v1) || !is_int(v2))
		return -HSE_INVALID_TYPES;

	/* widen v1 to uint32 and v2 to width of v1 if necessary */
	if (v1->type < HXB_DTYPE_UINT32)
		v1->type = HXB_DTYPE_UINT32;
	if (v2->type < v1->type)
		v2->type = v1->type;

	bool is_u64 = v1->type == HXB_DTYPE_UINT64;

#define ONE_OP(OP) \
	if (is_u64) \
		v1->v_uint64 = v1->v_uint64 OP v2->v_uint64; \
	else \
		v1->v_uint = v1->v_uint OP v2->v_uint; \
	break;

	switch (op) {
	case HSO_OP_AND: ONE_OP(&)
	case HSO_OP_OR:  ONE_OP(|)
	case HSO_OP_XOR: ONE_OP(^)
	case HSO_OP_SHL: ONE_OP(<<)
	case HSO_OP_SHR: ONE_OP(>>)

	default: return -HSE_INVALID_OPCODE;
	}

#undef ONE_OP

	return 0;
}

static int sm_cmp_block(hxb_sm_value_t* val, int op,
	uint8_t first, uint8_t last, const char* cmp)
{
	if (val->type != HXB_DTYPE_16BYTES)
		return -HSE_INVALID_TYPES;

	val->type = HXB_DTYPE_BOOL;

	if (op == HSO_CMP_BLOCK) {
		val->v_uint = memcmp(val->v_binary + first, cmp, last - first + 1) == 0;
		return 0;
	}

	if (op == HSO_CMP_IP_LO) {
		bool zero = true;
		for (uint8_t i = 0; i < 15; i++)
			zero &= val->v_binary[i] == 0;

		val->v_uint = zero && val->v_binary[15] == 1;
		return 0;
	}

	return -HSE_INVALID_OPCODE;
}

int SM_EXPORT(sm_load_mem)(const struct hxb_sm_instruction* insn, hxb_sm_value_t* value)
{
#define CHECK_MEM_SIZE(size) \
	do { \
		if (insn->mem.addr + size > SM_MEMORY_SIZE || insn->mem.addr + size < insn->mem.addr) \
			return -HSE_OOB_READ; \
	} while (0)
#define MEM_OP(field, type) \
	do { \
		type tmp; \
		CHECK_MEM_SIZE(sizeof(type)); \
		memcpy(&tmp, sm_memory + insn->mem.addr, sizeof(type)); \
		value->field = tmp; \
	} while (0)

	switch (insn->mem.type) {
	case HSM_BOOL:
		value->type = HXB_DTYPE_BOOL;
		MEM_OP(v_uint, bool);
		break;

	case HSM_U8:
		value->type = HXB_DTYPE_UINT8;
		MEM_OP(v_uint, uint8_t);
		break;

	case HSM_U32:
		value->type = HXB_DTYPE_UINT32;
		MEM_OP(v_uint, uint32_t);
		break;

	case HSM_U64:
		value->type = HXB_DTYPE_UINT64;
		MEM_OP(v_uint64, uint64_t);
		break;

	case HSM_FLOAT:
		value->type = HXB_DTYPE_FLOAT;
		MEM_OP(v_float, float);
		break;
	}

	return 0;

#undef CHECK_MEM_SIZE
#undef MEM_OP
}

int SM_EXPORT(sm_store_mem)(const struct hxb_sm_instruction* insn, const hxb_sm_value_t* value)
{
#define CHECK_MEM_SIZE(size) \
	do { \
		if (insn->mem.addr + size > SM_MEMORY_SIZE || insn->mem.addr + size < insn->mem.addr) \
			return -HSE_OOB_WRITE; \
	} while (0)
#define MEM_OP(field, type) \
	do { \
		const type tmp = value->field; \
		CHECK_MEM_SIZE(sizeof(type)); \
		memcpy(sm_memory + insn->mem.addr, &tmp, sizeof(type)); \
	} while (0)

	switch (insn->mem.type) {
	case HSM_BOOL:
		MEM_OP(v_uint, bool);
		break;

	case HSM_U8:
		MEM_OP(v_uint, uint8_t);
		break;

	case HSM_U32:
		MEM_OP(v_uint, uint32_t);
		break;

	case HSM_U64:
		MEM_OP(v_uint64, uint64_t);
		break;

	case HSM_FLOAT:
		MEM_OP(v_float, float);
		break;
	}

	return 0;

#undef CHECK_MEM_SIZE
#undef MEM_OP
}

int SM_EXPORT(run_sm)(const char* src_ip, uint32_t eid, const hxb_sm_value_t* val)
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
#define TOP_N(n) (sm_stack[top - n])
#define TOP TOP_N(0)

	if (sm_first_run) {
		memset(sm_memory, 0, SM_MEMORY_SIZE);
	}

	{
		uint32_t val;

		FAIL_AS(sm_get_u8(0, &val));
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
		case HSO_LD_SOURCE_IP:
			if (!src_ip || !val)
				FAIL_WITH(HSE_INVALID_OPERATION);
			PUSH(HXB_DTYPE_16BYTES, v_binary, src_ip);
			break;

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
		case HSO_LD_FLOAT:
			PUSH_V(insn.immed);
			break;

		case HSO_LD_SYSTIME: {
			PUSH(HXB_DTYPE_UINT64, v_uint64, sm_get_systime());
			break;
		}

		case HSO_LD_MEM:
			PUSH(HXB_DTYPE_BOOL, v_uint, 0);
			FAIL_AS(sm_load_mem(&insn, &TOP));
			break;

		case HSO_ST_MEM:
			CHECK_POP(1);
			FAIL_AS(sm_store_mem(&insn, &TOP));
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
		case HSO_OP_SHL:
		case HSO_OP_SHR:
			CHECK_POP(2);
			FAIL_AS(sm_int_op2(&TOP_N(1), &TOP_N(0), insn.opcode));
			top--;
			break;

		case HSO_OP_NOT:
			CHECK_POP(1);
			switch (TOP.type) {
			case HXB_DTYPE_BOOL:
				TOP.v_uint = !TOP.v_uint;
				break;
			case HXB_DTYPE_UINT8:
			case HXB_DTYPE_UINT32:
				TOP.v_uint = ~TOP.v_uint;
				TOP.type = HXB_DTYPE_UINT32;
				break;
			case HXB_DTYPE_UINT64:
				TOP.v_uint64 = ~TOP.v_uint64;
				break;
			default:
				FAIL_WITH(HSE_INVALID_TYPES);
			}
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
		case HSO_OP_ROT_I: {
			uint8_t offset = insn.immed.v_uint;

			CHECK_POP(offset + 1);
			hxb_sm_value_t val = TOP_N(offset);
			memmove(sm_stack + top - offset, sm_stack + top - offset + 1, offset * sizeof(sm_stack[0]));
			TOP = val;
			break;
		}

		case HSO_EXCHANGE: {
			uint8_t offset = insn.immed.v_uint;

			CHECK_POP(offset + 1);
			TOP_N(offset + 1) = TOP;
			top--;
			break;
		}

		case HSO_OP_DT_DECOMPOSE: {
			CHECK_POP(1);

			if (!is_int(&TOP))
				FAIL_WITH(HSE_INVALID_TYPES);

			uint64_t t = TOP.type == HXB_DTYPE_UINT64 ? TOP.v_uint64 : TOP.v_uint;
			struct {
				uint8_t tm_sec;
				uint8_t tm_min;
				uint8_t tm_hour;
				uint8_t tm_mday;
				uint8_t tm_mon;
				uint32_t tm_year;
				uint8_t tm_wday;
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
				long long days, secs;
				int remdays, remsecs, remyears;
				int qc_cycles, c_cycles, q_cycles;
				int years, months;
				int wday, yday, leap;
				const char days_in_month[] = {31,30,31,30,31,31,30,31,30,31,31,29};

				/* Reject time_t values whose year would overflow uint32 */
				if (t > UINT32_MAX * 31622400LL)
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
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_wday);
			if (insn.dt_mask & HSDM_YEAR)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_year);
			if (insn.dt_mask & HSDM_MONTH)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_mon + 1);
			if (insn.dt_mask & HSDM_DAY)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_mday);
			if (insn.dt_mask & HSDM_HOUR)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_hour);
			if (insn.dt_mask & HSDM_MINUTE)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_min);
			if (insn.dt_mask & HSDM_SECOND)
				PUSH(HXB_DTYPE_UINT32, v_uint, tm.tm_sec);

			break;
		}

		case HSO_OP_SWITCH_8:
		case HSO_OP_SWITCH_16:
		case HSO_OP_SWITCH_32: {
			CHECK_POP(1);

			if (!is_int(&TOP))
				FAIL_WITH(HSE_INVALID_TYPES);

			uint16_t offset = insn_length;

			if (insn.opcode == HSO_OP_SWITCH_8) {
				insn_length += insn.immed.v_uint * (1 + 2);
			} else if (insn.opcode == HSO_OP_SWITCH_16) {
				insn_length += insn.immed.v_uint * (2 + 2);
			} else {
				insn_length += insn.immed.v_uint * (4 + 2);
			}

			for (uint16_t i = 0; i < insn.immed.v_uint; i++) {
				uint32_t val, jump_val;

				int rc = -1;
				if (insn.opcode == HSO_OP_SWITCH_8) {
					rc = sm_get_u8(addr + offset, &val);
				} else if (insn.opcode == HSO_OP_SWITCH_16) {
					rc = sm_get_u16(addr + offset, &val);
				} else {
					rc = sm_get_u32(addr + offset, &val);
				}
				if (rc < 0)
					FAIL_WITH(HSE_INVALID_OPCODE);
				offset += rc;

				rc = sm_get_u16(addr + offset, &jump_val);
				if (rc < 0)
					FAIL_WITH(HSE_INVALID_OPCODE);
				offset += rc;

				if (TOP.v_uint == val) {
					jump_skip = jump_val;
					break;
				}
			}
			top--;

			break;
		}

		case HSO_CMP_BLOCK:
		case HSO_CMP_IP_LO:
			CHECK_POP(1);
			FAIL_AS(sm_cmp_block(&TOP, insn.opcode, insn.block.first,
					insn.block.last, insn.block.data));
			break;

		case HSO_JNZ:
		case HSO_JZ: {
			CHECK_POP(1);

			bool is_zero = false;

			switch (TOP.type) {
			case HXB_DTYPE_BOOL:
			case HXB_DTYPE_UINT8:
			case HXB_DTYPE_UINT32:
				is_zero = TOP.v_uint == 0;
				break;

			case HXB_DTYPE_FLOAT:
				is_zero = TOP.v_float == 0;
				break;

			default:
				FAIL_WITH(HSE_INVALID_TYPES);
			}

			bool jz = insn.opcode == HSO_JZ;

			if (is_zero == jz)
				jump_skip = insn.jump_skip;

			top--;
			break;
		}

		case HSO_JUMP:
			jump_skip = insn.jump_skip;
			break;

		case HSO_WRITE: {
			CHECK_POP(2);

			if (!is_int(&TOP_N(1)))
				FAIL_WITH(HSE_INVALID_TYPES);

			hxb_sm_value_t write_val = TOP_N(0);
			uint32_t write_eid = TOP_N(1).v_uint;

			enum hxb_error_code err = (enum hxb_error_code) sm_endpoint_write(write_eid, &write_val);

			if (err)
				FAIL_WITH(HSE_WRITE_FAILED);

			top -= 2;
			break;
		}

		case HSO_CONV_B:
		case HSO_CONV_U8:
		case HSO_CONV_U32:
		case HSO_CONV_U64:
		case HSO_CONV_F: {
			CHECK_POP(1);

			uint8_t to = 0;
			switch (insn.opcode) {
			case HSO_CONV_B: to = HXB_DTYPE_BOOL; break;
			case HSO_CONV_U8: to = HXB_DTYPE_UINT8; break;
			case HSO_CONV_U32: to = HXB_DTYPE_UINT32; break;
			case HSO_CONV_U64: to = HXB_DTYPE_UINT64; break;
			case HSO_CONV_F: to = HXB_DTYPE_FLOAT; break;
			default: FAIL_WITH(HSE_INVALID_TYPES);
			}
			FAIL_AS(sm_convert(&TOP, to));
			break;
		}

		case HSO_POP:
			CHECK_POP(1);
			top--;
			break;

		case HSO_RET:
			goto end_program;
		}

		if (addr + insn_length + jump_skip < addr)
			FAIL_WITH(HSE_INVALID_OPERATION);
		addr += insn_length + jump_skip;
	}

end_program:
	ret = 0;
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
