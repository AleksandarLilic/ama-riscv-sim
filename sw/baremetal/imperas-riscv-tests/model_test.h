#include "../common/csr.h"
#include "../common/asm_test.S"

#define AMA_RISCV_SIM_PASS PASS

// model stubs
#define RVMODEL_IO_INIT
#define RVMODEL_IO_WRITE_STR(_SP, _STR)
#define RVMODEL_IO_CHECK()
#define RVMODEL_IO_ASSERT_GPR_EQ(_SP, _R, _I)
#define RVMODEL_IO_ASSERT_SFPR_EQ(_F, _R, _I)
#define RVMODEL_IO_ASSERT_DFPR_EQ(_D, _R, _I)

#define RVMODEL_IO_INIT
#define RVMODEL_SET_MSW_INT
#define RVMODEL_CLEAR_MSW_INT
#define RVMODEL_CLEAR_MTIMER_INT
#define RVMODEL_CLEAR_MEXT_INT
#define RVMODEL_BOOT
#define RVMODEL_HALT AMA_RISCV_SIM_PASS // redefined
#define RVMODEL_DATA_BEGIN
#define RVMODEL_DATA_END
