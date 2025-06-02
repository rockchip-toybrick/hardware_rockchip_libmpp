/*************************************************************************
    > File Name: utils/mpi_dec_slt_utils.h
    > Author: LiHongjin
    > Mail: 872648180@qq.com
    > Created Time: Mon 02 Jun 2025 04:38:43 PM CST
 ************************************************************************/

#ifndef MPI_DEC_SLT_UTILS_H__
#define MPI_DEC_SLT_UTILS_H__

#include "mpp_soc.h"
#include "mpp_frame.h"
#include "mpi_dec_utils.h"
#include "mpp_lock.h"

typedef struct {
    RK_U32 en_debug;
    char *config_f;
    char *result_o;
} dec_slt_cmd_paras;

typedef enum slt_type_e {
    MPP_SLT_BASE,
    MPP_SLT_MOD_MULT,
    MPP_SLT_STRESS,
    MPP_SLT_BUT,
} slt_type;

#define SLT_FNAME_LEN (256)

typedef struct slt_v_node_t {
    spinlock_t lock;
    char v_name[SLT_FNAME_LEN];
    MpiDecTestCmd dec_cmd_ctx;
    void *mult_info;
    MppCodingType type;
    RK_U32 thd_cnt;
    RK_U32 frm_cnt;
    RK_U32 core_cnt;
    RK_U64 total_time;
    RK_U32 avg_fps;
    RK_U32 max_fps;
    RK_U32 min_fps;
    RK_U32 err_info;
    RK_U32 o_fps;
    MppFrameFormat o_fmt;
    RK_U32 res;
} slt_v_node;

typedef struct slt_item_t {
    RockchipSocType plt;
    slt_type type;
    RK_U32 v_cap;
    RK_U32 v_cnt;
    slt_v_node *v_nodes;
} slt_item;

#define SLT_DEF_CFG_FILE "/sdcard/config.cfg";
#define SLT_DEF_RES_FILE "/sdcard/result.cfg";

int proc_paras(int argc, char* argv[], dec_slt_cmd_paras *cmd_paras);
void write_config_file(const char* fname);
MPP_RET slt_config_init(const char* fname, slt_item **cfg);
MPP_RET slt_config_deinit(slt_item **cfg);
void slt_dec_analyze(dec_slt_cmd_paras *cmd_paras, slt_item *cfgs);
void dump_cfg(slt_item *cfg);

#endif /* MPI_DEC_SLT_UTILS_H__ */
