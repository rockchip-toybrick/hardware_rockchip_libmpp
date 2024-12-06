// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#define MODULE_TAG "mpp_serivce"

#include <linux/string.h>

#include "mpp_log.h"

//#include "mpp_device_debug.h"
#include "mpp_service.h"
#include "mpp_service_api.h"
#include "rk_export_func.h"

typedef struct MppDevMppService_t {
	RK_S32 client_type;

	struct mpp_session *chnl;

	RK_S32 req_cnt;
	MppReqV1 reqs[MAX_REQ_NUM];
	RK_S32 info_count;

	struct vcodec_mppdev_svr_fn *mppdev_ops;
} MppDevMppService;

MPP_RET mpp_service_init(void *ctx, MppClientType type)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MPP_RET ret = MPP_OK;

	p->mppdev_ops = get_mppdev_svr_ops();
	if (!p->mppdev_ops) {
		mpp_err("get_mppdev_svr_ops fail");
		return MPP_NOK;
	}
	if (p->mppdev_ops->chnl_open)
		p->chnl = p->mppdev_ops->chnl_open(type);

	if (!p->chnl) {
		ret = MPP_NOK;
		mpp_err("mpp_chnl_open fail");
	}

	return ret;
}

MPP_RET mpp_service_deinit(void *ctx)
{
	MppDevMppService *p = (MppDevMppService *) ctx;

	if (p->chnl && p->mppdev_ops->chnl_release)
		p->mppdev_ops->chnl_release(p->chnl);

	return MPP_OK;
}

void mpp_service_chnl_register(void *ctx, void *func, RK_S32 chan_id)
{
	MppDevMppService *p = (MppDevMppService *) ctx;

	if (p->chnl && p->mppdev_ops->chnl_register)
		p->mppdev_ops->chnl_register(p->chnl, func, chan_id);

	return;
}

MPP_RET mpp_service_reg_wr(void *ctx, MppDevRegWrCfg * cfg)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

	if (!p->req_cnt)
		memset(p->reqs, 0, sizeof(p->reqs));

	mpp_req->cmd = MPP_CMD_SET_REG_WRITE;
	mpp_req->flag = 0;
	mpp_req->size = cfg->size;
	mpp_req->offset = cfg->offset;
	mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);
	p->req_cnt++;

	return MPP_OK;
}

MPP_RET mpp_service_reg_rd(void *ctx, MppDevRegRdCfg * cfg)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MppReqV1 *mpp_req = &p->reqs[p->req_cnt];

	if (!p->req_cnt)
		memset(p->reqs, 0, sizeof(p->reqs));

	mpp_req->cmd = MPP_CMD_SET_REG_READ;
	mpp_req->flag = 0;
	mpp_req->size = cfg->size;
	mpp_req->offset = cfg->offset;
	mpp_req->data_ptr = REQ_DATA_PTR(cfg->reg);
	p->req_cnt++;

	return MPP_OK;
}

MPP_RET mpp_service_cmd_send(void *ctx)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MPP_RET ret = MPP_OK;

	if (p->req_cnt <= 0 || p->req_cnt > MAX_REQ_NUM) {
		mpp_err_f("ctx %p invalid request count %d\n", ctx, p->req_cnt);
		return MPP_ERR_VALUE;
	}

	/* setup flag for multi message request */
	if (p->req_cnt > 1) {
		RK_S32 i;

		for (i = 0; i < p->req_cnt; i++)
			p->reqs[i].flag |= MPP_FLAGS_MULTI_MSG;
	}
	p->reqs[p->req_cnt - 1].flag |= MPP_FLAGS_LAST_MSG | MPP_FLAGS_REG_FD_NO_TRANS;
	if (p->mppdev_ops->chnl_add_req)
		ret = p->mppdev_ops->chnl_add_req(p->chnl, &p->reqs[0]);
	p->req_cnt = 0;

	return ret;
}

MPP_RET mpp_service_cmd_poll(void *ctx)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MppReqV1 dev_req;
	MPP_RET ret = 0;

	memset(&dev_req, 0, sizeof(dev_req));
	dev_req.cmd = MPP_CMD_POLL_HW_FINISH;
	dev_req.flag |= MPP_FLAGS_LAST_MSG;
	if (p->mppdev_ops->chnl_add_req)
		ret = p->mppdev_ops->chnl_add_req(p->chnl, &dev_req);

	return ret;
}

RK_U32 mpp_service_iova_address(void *ctx, struct dma_buf * buf, RK_U32 offset)
{
	RK_U32 iova_address = 0;
	MppDevMppService *p = (MppDevMppService *) ctx;

	if (p->mppdev_ops->chnl_get_iova_addr)
		iova_address = p->mppdev_ops->chnl_get_iova_addr(p->chnl, buf, offset);

	return iova_address;
}

struct device * mpp_service_get_dev(void *ctx)
{
	MppDevMppService *p = (MppDevMppService *)ctx;

	if (p->mppdev_ops->mpp_chnl_get_dev)
		return p->mppdev_ops->mpp_chnl_get_dev(p->chnl);

	return NULL;
}

RK_S32 mpp_service_run_task(void *ctx, void *param)
{
	MppDevMppService *p = (MppDevMppService *)ctx;

	if (p->mppdev_ops->chnl_run_task)
		return p->mppdev_ops->chnl_run_task(p->chnl, param);

	return 0;
}

RK_S32 mpp_service_chnl_check_running(void *ctx)
{
	MppDevMppService *p = (MppDevMppService *)ctx;

	if (p->mppdev_ops->chnl_check_running)
		return p->mppdev_ops->chnl_check_running(p->chnl);

	return 0;
}

RK_S32 mpp_service_chnl_control(void *ctx, RK_U32 cmd, void *param)
{
	MppDevMppService *p = (MppDevMppService *) ctx;
	MppReqV1 req;

	req.cmd = cmd;
	req.flag = MPP_FLAGS_LAST_MSG;
	req.data_ptr = REQ_DATA_PTR(param);
	if (p->mppdev_ops->chnl_add_req)
		return p->mppdev_ops->chnl_add_req(p->chnl, &req);

	return 0;
}

const MppDevApi mpp_service_api = {
	.name			= "mpp_service",
	.ctx_size		= sizeof(MppDevMppService),
	.init			= mpp_service_init,
	.deinit			= mpp_service_deinit,
	.reg_wr			= mpp_service_reg_wr,
	.reg_rd			= mpp_service_reg_rd,
	.cmd_send		= mpp_service_cmd_send,
	.cmd_poll		= mpp_service_cmd_poll,
	.get_address		= mpp_service_iova_address,
	.chnl_register		= mpp_service_chnl_register,
	.chnl_get_dev		= mpp_service_get_dev,
	.run_task		= mpp_service_run_task,
	.chnl_check_running 	= mpp_service_chnl_check_running,
	.control		= mpp_service_chnl_control,
};
