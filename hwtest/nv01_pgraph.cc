/*
 * Copyright (C) 2012 Marcin Kościelnicki <koriakin@0x04.net>
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hwtest.h"
#include "nvhw/pgraph.h"
#include "nvhw/chipset.h"
#include "nva.h"
#include "util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/*
 * TODO:
 *  - cliprects
 *  - point VTX methods
 *  - lin/line VTX methods
 *  - tri VTX methods
 *  - rect VTX methods
 *  - tex* COLOR methods
 *  - ifc/bitmap VTX methods
 *  - ifc COLOR methods
 *  - bitmap MONO methods
 *  - blit VTX methods
 *  - blit pixel ops
 *  - tex* pixel ops
 *  - point rasterization
 *  - line rasterization
 *  - lin rasterization
 *  - tri rasterization
 *  - rect rasterization
 *  - texlin calculations
 *  - texquad calculations
 *  - quad rasterization
 *  - blit/ifc/bitmap rasterization
 *  - itm methods
 *  - ifm methods
 *  - notify
 *  - range interrupt
 *  - interrupt handling
 *  - ifm rasterization
 *  - ifm dma
 *  - itm rasterization
 *  - itm dma
 *  - itm pixel ops
 *  - cleanup & refactor everything
 */

static const uint32_t nv01_pgraph_state_regs[] = {
	/* INTR, INVALID */
	0x400100, 0x400104,
	/* INTR_EN, INVALID_EN */
	0x400140, 0x400144,
	/* CTX_* */
	0x400180, 0x400190,
	/* ICLIP */
	0x400450, 0x400454,
	/* UCLIP */
	0x400460, 0x400468, 0x400464, 0x40046c,
	/* VTX_X */
	0x400400, 0x400404, 0x400408, 0x40040c,
	0x400410, 0x400414, 0x400418, 0x40041c,
	0x400420, 0x400424, 0x400428, 0x40042c,
	0x400430, 0x400434, 0x400438, 0x40043c,
	0x400440, 0x400444,
	/* VTX_Y */
	0x400480, 0x400484, 0x400488, 0x40048c,
	0x400490, 0x400494, 0x400498, 0x40049c,
	0x4004a0, 0x4004a4, 0x4004a8, 0x4004ac,
	0x4004b0, 0x4004b4, 0x4004b8, 0x4004bc,
	0x4004c0, 0x4004c4,
	/* VTX_BETA */
	0x400700, 0x400704, 0x400708, 0x40070c,
	0x400710, 0x400714, 0x400718, 0x40071c,
	0x400720, 0x400724, 0x400728, 0x40072c,
	0x400730, 0x400734,
	/* PATTERN_RGB, _A */
	0x400600, 0x400608, 0x400604, 0x40060c,
	/* PATTERN_MONO, _SHAPE */
	0x400610, 0x400614, 0x400618,
	/* BITMAP_COLOR */
	0x40061c, 0x400620,
	/* ROP, PLANE, CHROMA, BETA */
	0x400624, 0x400628, 0x40062c, 0x400630,
	/* CANVAS_CONFIG */
	0x400634,
	/* CANVAS */
	0x400688, 0x40068c,
	/* CLIPRECT */
	0x400690, 0x400698, 0x400694, 0x40069c,
	/* CLIPRECT_CTRL */
	0x4006a0,
	/* VALID, SOURCE_COLOR, SUBDIVIDE, EDGEFILL */
	0x400650, 0x400654, 0x400658, 0x40065c,
	/* XY_MISC */
	0x400640, 0x400644, 0x400648, 0x40064c,
	/* DMA, NOTIFY */
	0x400680, 0x400684,
	/* ACCESS */
	0x4006a4,
	/* DEBUG */
	0x400080, 0x400084, 0x400088,
	/* STATUS */
	0x4006b0,
	/* PFB_CONFIG */
	0x600200,
	0x600000,
};

static void nv01_pgraph_gen_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	state->chipset = ctx->chipset;
	state->debug[0] = jrand48(ctx->rand48) & 0x11111110;
	state->debug[1] = jrand48(ctx->rand48) & 0x31111101;
	state->debug[2] = jrand48(ctx->rand48) & 0x11111111;
	state->intr = 0;
	state->invalid = 0;
	state->intr_en = jrand48(ctx->rand48) & 0x11111011;
	state->invalid_en = jrand48(ctx->rand48) & 0x00011111;
	state->ctx_switch[0] = jrand48(ctx->rand48) & 0x807fffff;
	state->ctx_control = jrand48(ctx->rand48) & 0x11010103;
	for (int i = 0; i < 18; i++) {
		state->vtx_x[i] = jrand48(ctx->rand48);
		state->vtx_y[i] = jrand48(ctx->rand48);
	}
	for (int i = 0; i < 14; i++)
		state->vtx_beta[i] = jrand48(ctx->rand48) & 0x01ffffff;
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->uclip_min[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->uclip_max[i] = jrand48(ctx->rand48) & 0x3ffff;
		state->pattern_mono_rgb[i] = jrand48(ctx->rand48) & 0x3fffffff;
		state->pattern_mono_a[i] = jrand48(ctx->rand48) & 0xff;
		state->pattern_mono_bitmap[i] = jrand48(ctx->rand48) & 0xffffffff;
		state->bitmap_color[i] = jrand48(ctx->rand48) & 0x7fffffff;
	}
	state->pattern_config = jrand48(ctx->rand48) & 3;
	state->rop = jrand48(ctx->rand48) & 0xff;
	state->plane = jrand48(ctx->rand48) & 0x7fffffff;
	state->chroma = jrand48(ctx->rand48) & 0x7fffffff;
	state->beta = jrand48(ctx->rand48) & 0x7f800000;
	state->canvas_config = jrand48(ctx->rand48) & 0x01111011;
	state->xy_misc_0 = jrand48(ctx->rand48) & 0xf1ff11ff;
	state->xy_misc_1[0] = jrand48(ctx->rand48) & 0x03177331;
	state->xy_misc_4[0] = jrand48(ctx->rand48) & 0x30ffffff;
	state->xy_misc_4[1] = jrand48(ctx->rand48) & 0x30ffffff;
	state->valid[0] = jrand48(ctx->rand48) & 0x111ff1ff;
	state->misc32[0] = jrand48(ctx->rand48);
	state->subdivide = jrand48(ctx->rand48) & 0xffff00ff;
	state->edgefill = jrand48(ctx->rand48) & 0xffff0113;
	state->ctx_switch[1] = jrand48(ctx->rand48) & 0xffff;
	state->notify = jrand48(ctx->rand48) & 0x11ffff;
	state->dst_canvas_min = jrand48(ctx->rand48) & 0xffffffff;
	state->dst_canvas_max = jrand48(ctx->rand48) & 0x0fff0fff;
	state->cliprect_min[0] = jrand48(ctx->rand48) & 0x0fff0fff;
	state->cliprect_min[1] = jrand48(ctx->rand48) & 0x0fff0fff;
	state->cliprect_max[0] = jrand48(ctx->rand48) & 0x0fff0fff;
	state->cliprect_max[1] = jrand48(ctx->rand48) & 0x0fff0fff;
	state->cliprect_ctrl = jrand48(ctx->rand48) & 0x113;
	state->access = jrand48(ctx->rand48) & 0x0001f000;
	state->access |= 0x0f000111;
	state->status = 0;
	state->pfb_config = jrand48(ctx->rand48) & 0x1370;
	state->pfb_config |= nva_rd32(ctx->cnum, 0x600200) & ~0x1371;
	state->pfb_boot = nva_rd32(ctx->cnum, 0x600000);
}

static void nv01_pgraph_load_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	nva_wr32(ctx->cnum, 0x000200, 0xffffefff);
	nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
	nva_wr32(ctx->cnum, 0x4006a4, 0x04000100);
	nva_wr32(ctx->cnum, 0x400100, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400104, 0xffffffff);
	nva_wr32(ctx->cnum, 0x400140, state->intr_en);
	nva_wr32(ctx->cnum, 0x400144, state->invalid_en);
	nva_wr32(ctx->cnum, 0x400180, state->ctx_switch[0]);
	nva_wr32(ctx->cnum, 0x400190, state->ctx_control);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400450 + i * 4, state->iclip[i]);
		nva_wr32(ctx->cnum, 0x400460 + i * 8, state->uclip_min[i]);
		nva_wr32(ctx->cnum, 0x400464 + i * 8, state->uclip_max[i]);
	}
	for (int i = 0; i < 18; i++) {
		nva_wr32(ctx->cnum, 0x400400 + i * 4, state->vtx_x[i]);
		nva_wr32(ctx->cnum, 0x400480 + i * 4, state->vtx_y[i]);
	}
	for (int i = 0; i < 14; i++)
		nva_wr32(ctx->cnum, 0x400700 + i * 4, state->vtx_beta[i]);
	nva_wr32(ctx->cnum, 0x40061c, state->bitmap_color[0]);
	nva_wr32(ctx->cnum, 0x400620, state->bitmap_color[1]);
	nva_wr32(ctx->cnum, 0x400624, state->rop);
	nva_wr32(ctx->cnum, 0x400630, state->beta);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400600 + i * 8, state->pattern_mono_rgb[i]);
		nva_wr32(ctx->cnum, 0x400604 + i * 8, state->pattern_mono_a[i]);
		nva_wr32(ctx->cnum, 0x400610 + i * 4, state->pattern_mono_bitmap[i]);
	}
	nva_wr32(ctx->cnum, 0x400618, state->pattern_config);
	nva_wr32(ctx->cnum, 0x400628, state->plane);
	nva_wr32(ctx->cnum, 0x40062c, state->chroma);
	nva_wr32(ctx->cnum, 0x400688, state->dst_canvas_min);
	nva_wr32(ctx->cnum, 0x40068c, state->dst_canvas_max);
	nva_wr32(ctx->cnum, 0x400634, state->canvas_config);
	for (int i = 0; i < 2; i++) {
		nva_wr32(ctx->cnum, 0x400690 + i * 8, state->cliprect_min[i]);
		nva_wr32(ctx->cnum, 0x400694 + i * 8, state->cliprect_max[i]);
	}
	nva_wr32(ctx->cnum, 0x4006a0, state->cliprect_ctrl);
	nva_wr32(ctx->cnum, 0x400640, state->xy_misc_0);
	nva_wr32(ctx->cnum, 0x400644, state->xy_misc_1[0]);
	nva_wr32(ctx->cnum, 0x400648, state->xy_misc_4[0]);
	nva_wr32(ctx->cnum, 0x40064c, state->xy_misc_4[1]);
	nva_wr32(ctx->cnum, 0x400650, state->valid[0]);
	nva_wr32(ctx->cnum, 0x400654, state->misc32[0]);
	nva_wr32(ctx->cnum, 0x400658, state->subdivide);
	nva_wr32(ctx->cnum, 0x40065c, state->edgefill);
	nva_wr32(ctx->cnum, 0x400680, state->ctx_switch[1]);
	nva_wr32(ctx->cnum, 0x400684, state->notify);
	nva_wr32(ctx->cnum, 0x400080, state->debug[0]);
	nva_wr32(ctx->cnum, 0x400084, state->debug[1]);
	nva_wr32(ctx->cnum, 0x400088, state->debug[2]);
	nva_wr32(ctx->cnum, 0x4006a4, state->access);
	nva_wr32(ctx->cnum, 0x600200, state->pfb_config);
}

static void nv01_pgraph_dump_state(struct hwtest_ctx *ctx, struct pgraph_state *state) {
	int ctr = 0;
	while((state->status = nva_rd32(ctx->cnum, 0x4006b0))) {
		ctr++;
		if (ctr > 100000) {
			fprintf(stderr, "PGRAPH locked up [%08x]!\n", state->status);
			uint32_t save_intr_en = nva_rd32(ctx->cnum, 0x400140);
			uint32_t save_invalid_en = nva_rd32(ctx->cnum, 0x400144);
			uint32_t save_ctx_ctrl = nva_rd32(ctx->cnum, 0x400190);
			uint32_t save_access = nva_rd32(ctx->cnum, 0x4006a4);
			nva_wr32(ctx->cnum, 0x000200, 0xffffefff);
			nva_wr32(ctx->cnum, 0x000200, 0xffffffff);
			nva_wr32(ctx->cnum, 0x400140, save_intr_en);
			nva_wr32(ctx->cnum, 0x400144, save_invalid_en);
			nva_wr32(ctx->cnum, 0x400190, save_ctx_ctrl);
			nva_wr32(ctx->cnum, 0x4006a4, save_access);
			break;
		}
	}
	state->access = nva_rd32(ctx->cnum, 0x4006a4);
	state->xy_misc_1[0] = nva_rd32(ctx->cnum, 0x400644); /* this one can be disturbed by *reading* VTX mem */
	nva_wr32(ctx->cnum, 0x4006a4, 0x04000100);
	state->trap_addr = nva_rd32(ctx->cnum, 0x4006b4);
	state->trap_data[0] = nva_rd32(ctx->cnum, 0x4006b8);
	state->intr = nva_rd32(ctx->cnum, 0x400100) & ~0x100;
	state->invalid = nva_rd32(ctx->cnum, 0x400104);
	state->intr_en = nva_rd32(ctx->cnum, 0x400140);
	state->invalid_en = nva_rd32(ctx->cnum, 0x400144);
	state->ctx_switch[0] = nva_rd32(ctx->cnum, 0x400180);
	state->ctx_control = nva_rd32(ctx->cnum, 0x400190) & ~0x00100000;
	for (int i = 0; i < 2; i++) {
		state->iclip[i] = nva_rd32(ctx->cnum, 0x400450 + i * 4);
		state->uclip_min[i] = nva_rd32(ctx->cnum, 0x400460 + i * 8);
		state->uclip_max[i] = nva_rd32(ctx->cnum, 0x400464 + i * 8);
	}
	for (int i = 0; i < 18; i++) {
		state->vtx_x[i] = nva_rd32(ctx->cnum, 0x400400 + i * 4);
		state->vtx_y[i] = nva_rd32(ctx->cnum, 0x400480 + i * 4);
	}
	for (int i = 0; i < 14; i++) {
		state->vtx_beta[i] = nva_rd32(ctx->cnum, 0x400700 + i * 4);
	}
	state->bitmap_color[0] = nva_rd32(ctx->cnum, 0x40061c);
	state->bitmap_color[1] = nva_rd32(ctx->cnum, 0x400620);
	state->rop = nva_rd32(ctx->cnum, 0x400624);
	state->plane = nva_rd32(ctx->cnum, 0x400628);
	state->beta = nva_rd32(ctx->cnum, 0x400630);
	for (int i = 0; i < 2; i++) {
		state->pattern_mono_rgb[i] = nva_rd32(ctx->cnum, 0x400600 + i * 8);
		state->pattern_mono_a[i] = nva_rd32(ctx->cnum, 0x400604 + i * 8);
		state->pattern_mono_bitmap[i] = nva_rd32(ctx->cnum, 0x400610 + i * 4);
	}
	state->pattern_config = nva_rd32(ctx->cnum, 0x400618);
	state->chroma = nva_rd32(ctx->cnum, 0x40062c);
	state->canvas_config = nva_rd32(ctx->cnum, 0x400634);
	state->dst_canvas_min = nva_rd32(ctx->cnum, 0x400688);
	state->dst_canvas_max = nva_rd32(ctx->cnum, 0x40068c);
	for (int i = 0; i < 2; i++) {
		state->cliprect_min[i] = nva_rd32(ctx->cnum, 0x400690 + i * 8);
		state->cliprect_max[i] = nva_rd32(ctx->cnum, 0x400694 + i * 8);
	}
	state->cliprect_ctrl = nva_rd32(ctx->cnum, 0x4006a0);
	state->valid[0] = nva_rd32(ctx->cnum, 0x400650);
	state->misc32[0] = nva_rd32(ctx->cnum, 0x400654);
	state->subdivide = nva_rd32(ctx->cnum, 0x400658);
	state->edgefill = nva_rd32(ctx->cnum, 0x40065c);
	state->xy_misc_0 = nva_rd32(ctx->cnum, 0x400640);
	state->xy_misc_4[0] = nva_rd32(ctx->cnum, 0x400648);
	state->xy_misc_4[1] = nva_rd32(ctx->cnum, 0x40064c);
	state->ctx_switch[1] = nva_rd32(ctx->cnum, 0x400680);
	state->notify = nva_rd32(ctx->cnum, 0x400684);
	state->debug[0] = nva_rd32(ctx->cnum, 0x400080);
	state->debug[1] = nva_rd32(ctx->cnum, 0x400084);
	state->debug[2] = nva_rd32(ctx->cnum, 0x400088);
}

static int nv01_pgraph_cmp_state(struct pgraph_state *orig, struct pgraph_state *exp, struct pgraph_state *real, bool broke = false) {
	bool print = false;
#define CMP(reg, name, ...) \
	if (print) \
		printf("%08x %08x %08x " name " %s\n", \
			orig->reg, exp->reg, real->reg , \
			## __VA_ARGS__, (exp->reg == real->reg ? "" : "*")); \
	else if (exp->reg != real->reg) { \
		printf("Difference in reg " name ": expected %08x real %08x\n" , \
		## __VA_ARGS__, exp->reg, real->reg); \
		broke = true; \
	}
restart:
	CMP(status, "STATUS")
	// XXX: figure these out someday
#if 0
	CMP(trap_addr, "TRAP_ADDR")
	CMP(trap_data[0], "TRAP_DATA[0]")
#endif
	CMP(debug[0], "DEBUG[0]")
	CMP(debug[1], "DEBUG[1]")
	CMP(debug[2], "DEBUG[2]")
	CMP(intr, "INTR")
	CMP(intr_en, "INTR_EN")
	CMP(invalid, "INVALID")
	CMP(invalid_en, "INVALID_EN")
	CMP(ctx_switch[0], "CTX_SWITCH[0]")
	CMP(ctx_switch[1], "CTX_SWITCH[1]")
	CMP(notify, "NOTIFY")
	CMP(ctx_control, "CTX_CONTROL")
	for (int i = 0; i < 2; i++) {
		CMP(iclip[i], "ICLIP[%d]", i)
	}
	for (int i = 0; i < 2; i++) {
		CMP(uclip_min[i], "UCLIP_MIN[%d]", i)
		CMP(uclip_max[i], "UCLIP_MAX[%d]", i)
	}
	for (int i = 0; i < 18; i++) {
		CMP(vtx_x[i], "VTX_X[%d]", i)
		CMP(vtx_y[i], "VTX_Y[%d]", i)
	}
	for (int i = 0; i < 14; i++) {
		CMP(vtx_beta[i], "VTX_BETA[%d]", i)
	}
	CMP(xy_misc_0, "XY_MISC_0")
	CMP(xy_misc_1[0], "XY_MISC_1[0]")
	CMP(xy_misc_4[0], "XY_MISC_4[0]")
	CMP(xy_misc_4[1], "XY_MISC_4[1]")
	CMP(valid[0], "VALID[0]")
	CMP(misc32[0], "MISC32[0]")
	CMP(subdivide, "SUBDIVIDE")
	CMP(edgefill, "EDGEFILL")
	CMP(pattern_mono_rgb[0], "PATTERN_MONO_RGB[0]")
	CMP(pattern_mono_a[0], "PATTERN_MONO_A[0]")
	CMP(pattern_mono_rgb[1], "PATTERN_MONO_RGB[1]")
	CMP(pattern_mono_a[1], "PATTERN_MONO_A[1]")
	CMP(pattern_mono_bitmap[0], "PATTERN_MONO_BITMAP[0]")
	CMP(pattern_mono_bitmap[1], "PATTERN_MONO_BITMAP[1]")
	CMP(pattern_config, "PATTERN_CONFIG")
	CMP(bitmap_color[0], "BITMAP_COLOR[0]")
	CMP(bitmap_color[1], "BITMAP_COLOR[1]")
	CMP(rop, "ROP")
	CMP(beta, "BETA")
	CMP(plane, "PLANE")
	CMP(chroma, "CHROMA")
	CMP(dst_canvas_min, "DST_CANVAS_MIN")
	CMP(dst_canvas_max, "DST_CANVAS_MAX")
	CMP(canvas_config, "CANVAS_CONFIG")
	for (int i = 0; i < 2; i++) {
		CMP(cliprect_min[i], "CLIPRECT_MIN[%d]", i)
		CMP(cliprect_max[i], "CLIPRECT_MAX[%d]", i)
	}
	CMP(cliprect_ctrl, "CLIPRECT_CTRL")
	CMP(access, "ACCESS")
	if (broke && !print) {
		print = true;
		goto restart;
	}
	return broke;
}

static void nv01_pgraph_reset(struct pgraph_state *state) {
	state->valid[0] = 0;
	state->edgefill &= 0xffff0000;
	state->xy_misc_0 &= 0x1000;
	state->xy_misc_1[0] &= 0x03000000;
	state->xy_misc_4[0] &= 0xff000000;
	state->xy_misc_4[0] |= 0x00555500;
	state->xy_misc_4[1] &= 0xff000000;
	state->xy_misc_4[1] |= 0x00555500;
}

static int test_scan_access(struct hwtest_ctx *ctx) {
	uint32_t val = nva_rd32(ctx->cnum, 0x4006a4);
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t nv = jrand48(ctx->rand48);
		uint32_t next = val;
		nva_wr32(ctx->cnum, 0x4006a4, nv);
		if (nv & 1 << 24)
			insrt(next, 0, 1, extr(nv, 0, 1));
		if (nv & 1 << 25)
			insrt(next, 4, 1, extr(nv, 4, 1));
		if (nv & 1 << 26)
			insrt(next, 8, 1, extr(nv, 8, 1));
		if (nv & 1 << 27)
			insrt(next, 12, 5, extr(nv, 12, 5));
		uint32_t real = nva_rd32(ctx->cnum, 0x4006a4);
		if (real != next) {
			printf("ACCESS mismatch: prev %08x write %08x expected %08x real %08x\n", val, nv, next, real);
			return HWTEST_RES_FAIL;
		}
		val = next;
	}
	return HWTEST_RES_PASS;
}

static int test_scan_debug(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400080, 0x11111110, 0);
	TEST_BITSCAN(0x400084, 0x31111101, 0);
	TEST_BITSCAN(0x400088, 0x11111111, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_control(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400140, 0x11111111, 0);
	TEST_BITSCAN(0x400144, 0x00011111, 0);
	TEST_BITSCAN(0x400180, 0x807fffff, 0);
	TEST_BITSCAN(0x400190, 0x11010103, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_canvas(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400634, 0x01111011, 0);
	nva_wr32(ctx->cnum, 0x400688, 0x7fff7fff);
	if(nva_rd32(ctx->cnum, 0x400688) != 0x7fff7fff) {
		return HWTEST_RES_FAIL;
	}
	TEST_BITSCAN(0x400688, 0xffffffff, 0);
	TEST_BITSCAN(0x40068c, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400690, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400694, 0x0fff0fff, 0);
	TEST_BITSCAN(0x400698, 0x0fff0fff, 0);
	TEST_BITSCAN(0x40069c, 0x0fff0fff, 0);
	TEST_BITSCAN(0x4006a0, 0x113, 0);
	return HWTEST_RES_PASS;
}

static int test_scan_vtx(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	for (i = 0 ; i < 18; i++) {
		TEST_BITSCAN(0x400400 + i * 4, 0xffffffff, 0);
		TEST_BITSCAN(0x400480 + i * 4, 0xffffffff, 0);
		if (i < 14)
			TEST_BITSCAN(0x400700 + i * 4, 0x01ffffff, 0);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_clip(struct hwtest_ctx *ctx) {
	int i;
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400450, 0x0003ffff, 0);
	TEST_BITSCAN(0x400454, 0x0003ffff, 0);
	for (i = 0; i < 1000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t v0 = jrand48(ctx->rand48);
		uint32_t v1 = jrand48(ctx->rand48);
		uint32_t v2 = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400460 + idx * 8, v0);
		nva_wr32(ctx->cnum, 0x400464 + idx * 8, v1);
		v0 &= 0x3ffff;
		v1 &= 0x3ffff;
		TEST_READ(0x400460 + idx * 8, v0, "v0 %08x v1 %08x", v0, v1);
		TEST_READ(0x400464 + idx * 8, v1, "v0 %08x v1 %08x", v0, v1);
		if (jrand48(ctx->rand48) & 1) {
			nva_wr32(ctx->cnum, 0x400460 + idx * 8, v2);
		} else {
			nva_wr32(ctx->cnum, 0x400464 + idx * 8, v2);
		}
		v2 &= 0x3ffff;
		TEST_READ(0x400460 + idx * 8, v1, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
		TEST_READ(0x400464 + idx * 8, v2, "v0 %08x v1 %08x v2 %08x", v0, v1, v2);
	}
	return HWTEST_RES_PASS;
}

static int test_scan_context(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400600, 0x3fffffff, 0);
	TEST_BITSCAN(0x400604, 0xff, 0);
	TEST_BITSCAN(0x400608, 0x3fffffff, 0);
	TEST_BITSCAN(0x40060c, 0xff, 0);
	TEST_BITSCAN(0x400610, 0xffffffff, 0);
	TEST_BITSCAN(0x400614, 0xffffffff, 0);
	TEST_BITSCAN(0x400618, 3, 0);
	TEST_BITSCAN(0x40061c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400620, 0x7fffffff, 0);
	TEST_BITSCAN(0x400624, 0xff, 0);
	TEST_BITSCAN(0x400628, 0x7fffffff, 0);
	TEST_BITSCAN(0x40062c, 0x7fffffff, 0);
	TEST_BITSCAN(0x400680, 0x0000ffff, 0);
	TEST_BITSCAN(0x400684, 0x0011ffff, 0);
	int i;
	for (i = 0; i < 1000; i++) {
		uint32_t orig = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, 0x400630, orig);
		uint32_t exp = orig & 0x7f800000;
		if (orig & 0x80000000)
			exp = 0;
		uint32_t real = nva_rd32(ctx->cnum, 0x400630);
		if (real != exp) {
			printf("BETA scan mismatch: orig %08x expected %08x real %08x\n", orig, exp, real);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_scan_vstate(struct hwtest_ctx *ctx) {
	nva_wr32(ctx->cnum, 0x4006a4, 0x0f000111);
	TEST_BITSCAN(0x400640, 0xf1ff11ff, 0);
	TEST_BITSCAN(0x400644, 0x03177331, 0);
	TEST_BITSCAN(0x400648, 0x30ffffff, 0);
	TEST_BITSCAN(0x40064c, 0x30ffffff, 0);
	TEST_BITSCAN(0x400650, 0x111ff1ff, 0);
	TEST_BITSCAN(0x400654, 0xffffffff, 0);
	TEST_BITSCAN(0x400658, 0xffff00ff, 0);
	TEST_BITSCAN(0x40065c, 0xffff0113, 0);
	return HWTEST_RES_PASS;
}

static int test_state(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 1000; i++) {
		struct pgraph_state orig, real;
		nv01_pgraph_gen_state(ctx, &orig);
		nv01_pgraph_load_state(ctx, &orig);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &orig, &real)) {
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_soft_reset(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x400080, exp.debug[0] | 1);
		nv01_pgraph_reset(&exp);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_read(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct pgraph_state exp, real;
		nv01_pgraph_gen_state(ctx, &exp);
		int idx = nrand48(ctx->rand48) % ARRAY_SIZE(nv01_pgraph_state_regs);
		uint32_t reg = nv01_pgraph_state_regs[idx];
		nv01_pgraph_load_state(ctx, &exp);
		nva_rd32(ctx->cnum, reg);
		if ((reg & ~0xf) == 0x400460) {
			exp.xy_misc_1[0] &= ~0xfff000;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&exp, &exp, &real)) {
			printf("After reading %08x\n", reg);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		exp = orig;
		uint32_t reg;
		uint32_t val = jrand48(ctx->rand48);
		int idx;
		nv01_pgraph_load_state(ctx, &exp);
		switch (nrand48(ctx->rand48) % 69) {
			default:
				reg = 0x400140;
				exp.intr_en = val & 0x11111111;
				break;
			case 1:
				reg = 0x400144;
				exp.invalid_en = val & 0x11111;
				break;
			case 2:
				reg = 0x400180;
				exp.ctx_switch[0] = val & 0x807fffff;
				insrt(exp.debug[1], 0, 1, 0);
				insrt(exp.ctx_control, 24, 1, 0);
				break;
			case 3:
				reg = 0x400190;
				exp.ctx_control = val & 0x11010103;
				break;
			case 4:
				idx = nrand48(ctx->rand48) % 18;
				reg = 0x400400 + idx * 4;
				exp.vtx_x[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 0, idx, val, 0, -1, 0);
				break;
			case 5:
				idx = nrand48(ctx->rand48) % 18;
				reg = 0x400480 + idx * 4;
				exp.vtx_y[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 1, idx, val, 0, -1, 0);
				break;
			case 6:
				idx = nrand48(ctx->rand48) % 18;
				reg = 0x400500 + idx * 4;
				exp.vtx_x[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 0, idx, val, 1, -1, idx & 3);
				break;
			case 7:
				idx = nrand48(ctx->rand48) % 18;
				reg = 0x400580 + idx * 4;
				exp.vtx_y[idx] = val;
				nv01_pgraph_vtx_fixup(&exp, 1, idx, val, 1, -1, idx & 3);
				break;
			case 8:
				idx = nrand48(ctx->rand48) % 14;
				reg = 0x400700 + idx * 4;
				exp.vtx_beta[idx] = val & 0x01ffffff;
				break;
			case 9:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400450 + idx * 4;
				insrt(exp.xy_misc_1[0], 14, 1, 0);
				insrt(exp.xy_misc_1[0], 18, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				nv01_pgraph_iclip_fixup(&exp, idx, val, 0);
				break;
			case 10:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400550 + idx * 4;
				insrt(exp.xy_misc_1[0], 14, 1, 0);
				insrt(exp.xy_misc_1[0], 18, 1, 0);
				insrt(exp.xy_misc_1[0], 20, 1, 0);
				nv01_pgraph_iclip_fixup(&exp, idx, val, 1);
				break;
			case 11:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400460 + idx * 4;
				nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 0);
				break;
			case 12:
				idx = jrand48(ctx->rand48) & 3;
				reg = 0x400560 + idx * 4;
				nv01_pgraph_uclip_fixup(&exp, idx >> 1, idx & 1, val, 1);
				break;
			case 13:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400600 + idx * 8;
				exp.pattern_mono_rgb[idx] = val & 0x3fffffff;
				break;
			case 14:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400604 + idx * 8;
				exp.pattern_mono_a[idx] = val & 0xff;
				break;
			case 15:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400610 + idx * 4;
				exp.pattern_mono_bitmap[idx] = val;
				break;
			case 16:
				reg = 0x400618;
				exp.pattern_config = val & 3;
				break;
			case 17:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x40061c + idx * 4;
				exp.bitmap_color[idx] = val & 0x7fffffff;
				break;
			case 18:
				reg = 0x400624;
				exp.rop = val & 0xff;
				break;
			case 19:
				reg = 0x400628;
				exp.plane = val & 0x7fffffff;
				break;
			case 20:
				reg = 0x40062c;
				exp.chroma = val & 0x7fffffff;
				break;
			case 21:
				reg = 0x400630;
				exp.beta = val & 0x7f800000;
				if (val & 0x80000000)
					exp.beta = 0;
				break;
			case 22:
				reg = 0x400634;
				exp.canvas_config = val & 0x01111011;
				break;
			case 23:
				reg = 0x400688;
				exp.dst_canvas_min = val & 0xffffffff;
				break;
			case 24:
				reg = 0x40068c;
				exp.dst_canvas_max = val & 0x0fff0fff;
				break;
			case 25:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400690 + idx * 8;
				exp.cliprect_min[idx] = val & 0x0fff0fff;
				break;
			case 26:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400694 + idx * 8;
				exp.cliprect_max[idx] = val & 0x0fff0fff;
				break;
			case 27:
				reg = 0x4006a0;
				exp.cliprect_ctrl = val & 0x113;
				break;
			case 28:
				reg = 0x400640;
				exp.xy_misc_0 = val & 0xf1ff11ff;
				break;
			case 29:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400644;
				exp.xy_misc_1[0] = val & 0x03177331;
				break;
			case 31:
				idx = jrand48(ctx->rand48) & 1;
				reg = 0x400648 + idx * 4;
				exp.xy_misc_4[idx] = val & 0x30ffffff;
				break;
			case 32:
				reg = 0x400650;
				exp.valid[0] = val & 0x111ff1ff;
				break;
			case 33:
				reg = 0x400654;
				exp.misc32[0] = val;
				break;
			case 34:
				reg = 0x400658;
				exp.subdivide = val & 0xffff00ff;
				break;
			case 35:
				reg = 0x40065c;
				exp.edgefill = val & 0xffff0113;
				break;
			case 44:
				reg = 0x400680;
				exp.ctx_switch[1] = val & 0xffff;
				break;
			case 45:
				reg = 0x400684;
				exp.notify = val & 0x11ffff;
				break;
			case 46:
				reg = 0x4006a4;
				if (extr(val, 24, 1))
					insrt(exp.access, 0, 1, extr(val, 0, 1));
				if (extr(val, 25, 1))
					insrt(exp.access, 4, 1, extr(val, 4, 1));
				if (extr(val, 26, 1))
					insrt(exp.access, 8, 1, extr(val, 8, 1));
				if (extr(val, 27, 1))
					insrt(exp.access, 12, 5, extr(val, 12, 5));
				break;
			case 47:
				reg = 0x400080;
				exp.debug[0] = val & 0x11111110;
				if (extr(val, 0, 1))
					nv01_pgraph_reset(&exp);
				break;
			case 48:
				reg = 0x400084;
				exp.debug[1] = val & 0x31111101;
				break;
			case 49:
				reg = 0x400088;
				exp.debug[2] = val & 0x11111111;
				break;
		}
		nva_wr32(ctx->cnum, reg, val);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_clip_status(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		struct pgraph_state exp;
		nv01_pgraph_gen_state(ctx, &exp);
		uint32_t cls = extr(exp.access, 12, 5);
		nv01_pgraph_load_state(ctx, &exp);
		int32_t min, max;
		int32_t min_exp[2], max_exp[2];
		nv01_pgraph_clip_bounds(&exp, min_exp, max_exp);
		if (nv01_pgraph_is_tex_class(cls)) {
			min = max = 0x40000000;
			int bit;
			for (bit = 30; bit >= 15; bit--) {
				nva_wr32(ctx->cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(ctx->cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(ctx->cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(ctx->cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
			min >>= 15;
			max >>= 15;
			if (exp.xy_misc_1[0] & 0x02000000) {
				min >>= 4, max >>= 4;
				if (min_exp[xy] & 0x800)
					min_exp[xy] = 0x7ff;
				if (max_exp[xy] & 0x800)
					max_exp[xy] = 0x7ff;
			}
		} else {
			min = max = 0x20000;
			int bit;
			for (bit = 17; bit >= 0; bit--) {
				nva_wr32(ctx->cnum, 0x400400 + xy * 0x80, min ^ 1 << bit);
				if (nva_rd32(ctx->cnum, 0x400648 + xy * 4) & 0x300) {
					min ^= 1 << bit;
				}
				nva_wr32(ctx->cnum, 0x400400 + xy * 0x80, max ^ 1 << bit);
				if (!(nva_rd32(ctx->cnum, 0x400648 + xy * 4) & 0x400)) {
					max ^= 1 << bit;
				}
			}
		}
		if (min_exp[xy] != min || max_exp[xy] != max) {
			printf("%08x %08x %08x %08x  %08x %08x  %08x %08x  %08x  %03x %03x\n", cls, exp.xy_misc_1[0], min, max, exp.dst_canvas_min, exp.dst_canvas_max, exp.uclip_min[xy], exp.uclip_max[xy], exp.iclip[xy], min_exp[xy], max_exp[xy]);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_vtx_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = nrand48(ctx->rand48) % 18;
		int xy = jrand48(ctx->rand48) & 1;
		int rel = jrand48(ctx->rand48) & 1;
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		if (jrand48(ctx->rand48) & 1) {
			/* rare and complicated enough to warrant better testing */
			idx &= 1;
			idx |= 0x10;
		}
		if (jrand48(ctx->rand48) & 1) {
			/* rare and complicated enough to warrant better testing */
			orig.access = 0x0f00d111 + (jrand48(ctx->rand48) & 0x11000);
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400400 + idx * 4 + xy * 0x80 + rel * 0x100;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv01_pgraph_vtx_fixup(&exp, xy, idx, val, rel, -1, rel ? idx & 3 : 0);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_iclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		int rel = jrand48(ctx->rand48) & 1;
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400450 + xy * 4 + rel * 0x100;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		insrt(exp.xy_misc_1[0], 14, 1, 0);
		insrt(exp.xy_misc_1[0], 18, 1, 0);
		insrt(exp.xy_misc_1[0], 20, 1, 0);
		nv01_pgraph_iclip_fixup(&exp, xy, val, rel);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mmio_uclip_write(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int xy = jrand48(ctx->rand48) & 1;
		int idx = jrand48(ctx->rand48) & 1;
		int rel = jrand48(ctx->rand48) & 1;
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t reg = 0x400460 + xy * 8 + idx * 4 + rel * 0x100;
		uint32_t val = jrand48(ctx->rand48);
		nva_wr32(ctx->cnum, reg, val);
		nv01_pgraph_uclip_fixup(&exp, xy, idx, val, rel);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("After writing %08x <- %08x\n", reg, val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ctx_switch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		int cls = classes[nrand48(ctx->rand48) % 20];
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &exp);
		exp.notify &= ~0x010000;
		orig = exp;
		nv01_pgraph_load_state(ctx, &exp);
		nva_wr32(ctx->cnum, 0x400000 | cls << 16, val);
		int chsw = 0;
		int och = extr(exp.ctx_switch[0], 16, 7);
		int nch = extr(val, 16, 7);
		if ((val & 0x007f8000) != (exp.ctx_switch[0] & 0x007f8000))
			chsw = 1;
		if (!extr(exp.ctx_control, 16, 1))
			chsw = 1;
		int volatile_reset = val >> 31 && extr(exp.debug[2], 28, 1) && (!extr(exp.ctx_control, 16, 1) || och == nch);
		if (chsw) {
			exp.ctx_control |= 0x01010000;
			exp.intr |= 0x10;
			exp.access &= ~0x101;
		} else {
			exp.ctx_control &= ~0x01000000;
		}
		insrt(exp.access, 12, 5, cls);
		insrt(exp.debug[1], 0, 1, volatile_reset);
		if (volatile_reset) {
			exp.bitmap_color[0] &= 0x3fffffff;
			exp.bitmap_color[1] &= 0x3fffffff;
			exp.valid[0] &= 0x11000000;
			exp.xy_misc_0 = 0;
			exp.xy_misc_1[0] &= 0x33300;
			exp.xy_misc_4[0] = 0x555500;
			exp.xy_misc_4[1] = 0x555500;
			exp.misc32[0] &= 0x00ff00ff;
			exp.subdivide &= 0xffff0000;
		}
		exp.ctx_switch[0] = val & 0x807fffff;
		if (exp.notify & 0x100000) {
			exp.intr |= 0x10000001;
			exp.invalid |= 0x10000;
			exp.access &= ~0x101;
			exp.notify &= ~0x100000;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d\n", i);
			printf("Switch to %02x %08x%s\n", cls, val, volatile_reset?" *":"");
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_notify(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1) {
			val &= 0xf;
		}
		int classes[20] = {
			0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
			0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x1d, 0x1e,
			0x10, 0x11, 0x12, 0x13, 0x14,
		};
		int cls = classes[nrand48(ctx->rand48) % 20];
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &exp);
		orig = exp;
		nv01_pgraph_load_state(ctx, &exp);
		nva_wr32(ctx->cnum, 0x400104 | cls << 16, val);
		if (val && (cls & 0xf) != 0xd && (cls & 0xf) != 0xe)
			exp.invalid |= 0x10;
		if (exp.notify & 0x100000 && !exp.invalid)
			exp.intr |= 0x10000000;
		if (!(exp.ctx_switch[0] & 0x100))
			exp.invalid |= 0x100;
		if (exp.notify & 0x110000)
			exp.invalid |= 0x1000;
		if (exp.invalid) {
			exp.intr |= 1;
			exp.access &= ~0x101;
		} else {
			exp.notify |= 0x10000;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %d\n", i);
			printf("Notify to %08x cls %02x\n", val, cls);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_beta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x410300, val);
		exp.beta = val;
		if (exp.beta & 0x80000000)
			exp.beta = 0;
		exp.beta &= 0x7f800000;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Beta set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int check_mthd_invalid(struct hwtest_ctx *ctx, int cls, int mthd) {
	int i;
	for (i = 0; i < 10; i++) {
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x400000 | cls << 16 | mthd, 0);
		exp.intr |= 1;
		exp.invalid |= 1;
		exp.access &= ~0x101;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Used cls %02x mthd %04x iter %d\n", cls, mthd, i);
			return 1;
		}
	}
	return 0;
}

static int test_mthd_invalid(struct hwtest_ctx *ctx) {
	int i;
	int res = 0;
	int classes[20] = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
		0x08, 0x09, 0x0a, 0x0b, 0x0c,
		0x0d, 0x0e, 0x1d, 0x1e,
		0x10, 0x11, 0x12, 0x13, 0x14,
	};
	for (i = 0; i < 20; i++) {
		int cls = classes[i];
		int mthd;
		for (mthd = 0; mthd < 0x2000; mthd += 4) {
			if (mthd == 0 || mthd == 0x104) /* CTX_SWITCH, NOTIFY */
				continue;
			if ((cls == 1 || cls == 2) && mthd == 0x300) /* ROP, BETA */
				continue;
			if ((cls == 3 || cls == 4) && mthd == 0x304) /* CHROMA, PLANE */
				continue;
			if (cls == 5 && (mthd == 0x300 || mthd == 0x304)) /* CLIP */
				continue;
			if (cls == 6 && (mthd == 0x308 || (mthd >= 0x310 && mthd <= 0x31c))) /* PATTERN */
				continue;
			if (cls >= 8 && cls <= 0xc && mthd == 0x304) /* COLOR */
				continue;
			if (cls == 8 && mthd >= 0x400 && mthd < 0x580) /* POINT */
				continue;
			if (cls >= 9 && cls <= 0xa && mthd >= 0x400 && mthd < 0x680) /* LINE/LIN */
				continue;
			if (cls == 0xb && mthd >= 0x310 && mthd < 0x31c) /* TRI.TRIANGLE */
				continue;
			if (cls == 0xb && mthd >= 0x320 && mthd < 0x338) /* TRI.TRIANGLE32 */
				continue;
			if (cls == 0xb && mthd >= 0x400 && mthd < 0x600) /* TRI.TRIMESH, TRI.TRIMESH32, TRI.CTRIANGLE, TRI.CTRIMESH */
				continue;
			if (cls == 0xc && mthd >= 0x400 && mthd < 0x480) /* RECT */
				continue;
			if (cls == 0xd || cls == 0xe || cls == 0x1d || cls == 0x1e) { /* TEX* */
				if (mthd == 0x304) /* SUBDIVIDE */
					continue;
				int vcnt = 4;
				if ((cls & 0xf) == 0xe)
					vcnt = 9;
				int vcntd2 = (vcnt+1)/2;
				if (mthd >= 0x310 && mthd < 0x310+vcnt*4) /* VTX_POS_INT */
					continue;
				if (mthd >= 0x350 && mthd < 0x350+vcnt*4) /* VTX_POS_FRACT */
					continue;
				if (cls & 0x10 && mthd >= 0x380 && mthd < 0x380+vcntd2*4) /* VTX_BETA */
					continue;
				if (mthd >= 0x400 && mthd < 0x480) /* COLOR */
					continue;
			}
			if (cls == 0x10 && mthd >= 0x300 && mthd < 0x30c) /* BLIT */
				continue;
			if (cls == 0x11 && mthd >= 0x304 && mthd < 0x310) /* IFC setup */
				continue;
			if (cls == 0x12 && mthd >= 0x308 && mthd < 0x31c) /* BITMAP setup */
				continue;
			if (cls >= 0x11 && cls <= 0x12 && mthd >= 0x400 && mthd < 0x480) /* IFC/BITMAP data */
				continue;
			if (cls >= 0x13 && cls <= 0x14 && mthd >= 0x308 && mthd < 0x318) /* IFM/ITM */
				continue;
			if (cls == 0x13 && mthd >= 0x40 && mthd < 0x80) /* IFM data */
				continue;
			res |= check_mthd_invalid(ctx, cls, mthd);
		}
	}
	return res ? HWTEST_RES_FAIL : HWTEST_RES_PASS;
}

static int test_mthd_rop(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xff;
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x420300, val);
		exp.rop = val & 0xff;
		if (val & ~0xff) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("ROP set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_chroma_plane(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int is_plane = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, is_plane ? 0x440304 : 0x430304, val);
		uint32_t color = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		if (is_plane)
			exp.plane = color;
		else
			exp.chroma = color;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Color set to %08x switch %08x config %08x\n", val, exp.ctx_switch[0], exp.canvas_config);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_clip(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		int is_size = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.vtx_x[15], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
			insrt(orig.vtx_y[15], 16, 15, (jrand48(ctx->rand48) & 1) ? 0 : 0x7fff);
		}
		/* XXX: submitting on BLIT causes an actual blit */
		if (is_size && extr(orig.access, 12, 5) == 0x10)
			insrt(orig.access, 12, 5, 0);
		nv01_pgraph_load_state(ctx, &orig);
		nva_wr32(ctx->cnum, 0x450300 + is_size * 4, val);
		exp = orig;
		nv01_pgraph_set_clip(&exp, is_size, val);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Clip %s set to %08x\n", is_size?"size":"point", val);
			printf("Iteration %d\n", i);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_shape(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		if (jrand48(ctx->rand48) & 1)
			val &= 0xf;
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x460308, val);
		exp.pattern_config = val & 3;
		if (val > 2) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Shape set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_bitmap(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x460318 + idx * 4, val);
		exp.pattern_mono_bitmap[idx] = pgraph_expand_mono(&exp, val);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Bitmap set to %08x switch %08x\n", val, exp.ctx_switch[0]);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pattern_mono_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48) & 1;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x460310 + idx * 4, val);
		struct pgraph_color c = pgraph_expand_color(&exp, val);
		exp.pattern_mono_rgb[idx] = c.r << 20 | c.g << 10 | c.b;
		exp.pattern_mono_a[idx] = c.a;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Color set to %08x switch %08x config %08x\n", val, exp.ctx_switch[0], exp.canvas_config);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_subdivide(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int beta = jrand48(ctx->rand48) & 1;
		int quad = jrand48(ctx->rand48) & 1;
		int cls = 0xd + beta * 0x10 + quad;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		nva_wr32(ctx->cnum, 0x400304 | cls << 16, val);
		exp = orig;
		exp.subdivide = val & 0xffff00ff;
		int err = 0;
		if (val & 0xff00)
			err = 1;
		int j;
		for (j = 0; j < 8; j++) {
			if (extr(val, 4*j, 4) > 8)
				err = 1;
			if (j < 2 && extr(val, 4*j, 4) < 2)
				err = 1;
		}
		if (err) {
			exp.intr |= 1;
			exp.invalid |= 0x10;
			exp.access &= ~0x101;
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Subdivide set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_vtx_beta(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int quad = jrand48(ctx->rand48) & 1;
		int idx = nrand48(ctx->rand48) % (quad?5:2);
		int mclass = 0x1d + quad;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		nva_wr32(ctx->cnum, (0x400380 + idx * 4) | mclass << 16, val);
		exp = orig;
		uint32_t rclass = extr(exp.access, 12, 5);
		int j;
		for (j = 0; j < 2; j++) {
			int vid = idx * 2 + j;
			if (vid == 9 && (rclass & 0xf) == 0xe)
				break;
			uint32_t beta = extr(val, j*16, 16);
			beta &= 0xff80;
			beta <<= 8;
			beta |= 0x4000;
			if (beta & 1 << 23)
				beta |= 1 << 24;
			exp.vtx_beta[vid] = beta;
		}
		if (rclass == 0x1d || rclass == 0x1e)
			exp.valid[0] |= 1 << (12 + idx);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Iter %04d: VTX %d set to %08x\n", i, idx, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_solid_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int cls;
		uint32_t mthd;
		switch (nrand48(ctx->rand48)%5) {
			case 0:
				mthd = 0x304;
				cls = 8 + nrand48(ctx->rand48)%5;
				break;
			case 1:
				mthd = 0x500 | (jrand48(ctx->rand48)&0x78);
				cls = 8;
				break;
			case 2:
				mthd = 0x600 | (jrand48(ctx->rand48)&0x78);
				cls = 9 + (jrand48(ctx->rand48)&1);
				break;
			case 3:
				mthd = 0x500 | (jrand48(ctx->rand48)&0x70);
				cls = 0xb;
				break;
			case 4:
				mthd = 0x580 | (jrand48(ctx->rand48)&0x78);
				cls = 0xb;
				break;
			default:
				abort();
		}
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		nva_wr32(ctx->cnum, 0x400000 | cls << 16 | mthd, val);
		exp = orig;
		exp.misc32[0] = val;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Color [%02x:%04x] set to %08x\n", cls, mthd, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_vtx(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.access, 12, 5, 8 + nrand48(ctx->rand48) % 4);
		}
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x033033;
		if (jrand48(ctx->rand48) & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int cls = extr(exp.access, 12, 5);
		bool first, poly = false;
		uint32_t mthd;
		int fract = 0;
		bool draw = false;
		switch (nrand48(ctx->rand48) % 27) {
			case 0:
				mthd = 0x500300;
				first = true;
				break;
			case 1:
				mthd = 0x510304;
				first = true;
				break;
			case 2:
				mthd = 0x520310;
				first = true;
				break;
			case 3:
				mthd = 0x530308;
				first = true;
				break;
			case 4:
				mthd = 0x540308;
				first = true;
				break;
			case 5:
				mthd = 0x500304;
				first = false;
				break;
			case 6:
				mthd = 0x4c0400 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 7:
				mthd = 0x4b0310;
				first = true;
				break;
			case 8:
				mthd = 0x4b0504 | (jrand48(ctx->rand48) & 0x70);
				first = true;
				break;
			case 9:
				mthd = 0x490400 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 10:
				mthd = 0x4a0400 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 11: {
				int beta = jrand48(ctx->rand48) & 1;
				int idx = jrand48(ctx->rand48) & 3;
				fract = jrand48(ctx->rand48) & 1;
				mthd = (0x4d0310 + idx * 4) | beta << 20 | fract << 6;
				first = idx == 0;
				break;
			}
			case 12: {
				int beta = jrand48(ctx->rand48) & 1;
				int idx = nrand48(ctx->rand48) % 9;
				fract = jrand48(ctx->rand48) & 1;
				mthd = (0x4e0310 + idx * 4) | beta << 20 | fract << 6;
				first = idx == 0;
				break;
			}
			case 13:
				mthd = 0x480400 | (jrand48(ctx->rand48) & 0x7c);
				first = true;
				draw = true;
				break;
			case 14:
				mthd = 0x480504 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				draw = true;
				break;
			case 15:
				mthd = 0x490404 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				draw = true;
				break;
			case 16:
				mthd = 0x4a0404 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				draw = true;
				break;
			case 17:
				mthd = 0x4b0314;
				first = false;
				break;
			case 18:
				mthd = 0x4b0318;
				first = false;
				draw = true;
				break;
			case 19:
				mthd = 0x4b0508 | (jrand48(ctx->rand48) & 0x70);
				first = false;
				break;
			case 20:
				mthd = 0x4b050c | (jrand48(ctx->rand48) & 0x70);
				first = false;
				draw = true;
				break;
			case 21:
				mthd = 0x490500 | (jrand48(ctx->rand48) & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 22:
				mthd = 0x4a0500 | (jrand48(ctx->rand48) & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 23:
				mthd = 0x490604 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			case 24:
				mthd = 0x4a0604 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			case 25:
				mthd = 0x4b0400 | (jrand48(ctx->rand48) & 0x7c);
				first = false;
				draw = true;
				poly = true;
				break;
			case 26:
				mthd = 0x4b0584 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				draw = true;
				poly = true;
				break;
			default:
				abort();
		}
		nva_wr32(ctx->cnum, mthd, val);
		if (first)
			insrt(exp.xy_misc_0, 28, 4, 0);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (cls == 0x11 || cls == 0x12 || cls == 0x13)
			idx = 4;
		if (nv01_pgraph_is_tex_class(cls)) {
			idx = (mthd - 0x10) >> 2 & 0xf;
			if (idx >= 12)
				idx -= 8;
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1) != (uint32_t)fract) {
				exp.valid[0] &= ~0xffffff;
			}
		} else {
			fract = 0;
		}
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, fract);
		nv01_pgraph_bump_vtxid(&exp);
		nv01_pgraph_set_vtx(&exp, 0, idx, extrs(val, 0, 16), false);
		nv01_pgraph_set_vtx(&exp, 1, idx, extrs(val, 16, 16), false);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 0x1001 << idx;
			if (cls >= 0x09 && cls <= 0x0b) {
				if (first) {
					exp.valid[0] &= ~0xffffff;
					exp.valid[0] |= 0x011111;
				} else {
					exp.valid[0] |= 0x10010 << (idx & 3);
				}
			}
			if ((cls == 0x10 || cls == 0x0c) && first)
				exp.valid[0] |= 0x100;
		} else {
			if (cls >= 9 && cls <= 0xb) {
				exp.valid[0] |= 0x10010 << (idx & 3);
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, poly);
		// XXX
		if (draw && (cls == 0x11 || cls == 0x12 || cls == 0x13))
			continue;
		nv01_pgraph_dump_state(ctx, &real);
		if (real.status && (cls == 0x0b || cls == 0x0c)) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Point %04x set to %08x [%d]\n", mthd, val, idx);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_vtx_x32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.access, 12, 5, 9 + nrand48(ctx->rand48) % 3);
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int cls = extr(exp.access, 12, 5);
		bool first, poly = false;
		uint32_t mthd;
		switch (nrand48(ctx->rand48) % 7) {
			case 0:
				mthd = 0x480480 | (jrand48(ctx->rand48) & 0x78);
				first = true;
				break;
			case 1:
				mthd = 0x490480 | (jrand48(ctx->rand48) & 0x78);
				first = !(mthd & 8);
				break;
			case 2:
				mthd = 0x4a0480 | (jrand48(ctx->rand48) & 0x78);
				first = !(mthd & 8);
				break;
			case 3:
				mthd = 0x4b0320 + 8 * (nrand48(ctx->rand48) % 3);
				first = (mthd == 0x4b0320);
				break;
			case 4:
				mthd = 0x490580 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				poly = true;
				break;
			case 5:
				mthd = 0x4a0580 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				poly = true;
				break;
			case 6:
				mthd = 0x4b0480 | (jrand48(ctx->rand48) & 0x78);
				first = false;
				poly = true;
				break;
			default:
				abort();
		}
		nva_wr32(ctx->cnum, mthd, val);
		if (first)
			insrt(exp.xy_misc_0, 28, 4, 0);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (nv01_pgraph_is_tex_class(cls)) {
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
				exp.valid[0] &= ~0xffffff;
			}
		}
		insrt(exp.xy_misc_0, 28, 4, idx);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, 0);
		nv01_pgraph_set_vtx(&exp, 0, idx, val, true);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 1 << idx;
			if (cls >= 0x09 && cls <= 0x0b) {
				if (first) {
					exp.valid[0] &= ~0xffffff;
					exp.valid[0] |= 0x000111;
				} else {
					exp.valid[0] |= 0x10 << (idx & 3);
				}
			}
			if ((cls == 0x10 || cls == 0x0c) && first)
				exp.valid[0] |= 0x100;
		} else {
			if (cls >= 9 && cls <= 0xb) {
				if (exp.valid[0] & 0xf00f)
					exp.valid[0] &= ~0x100;
				exp.valid[0] |= 0x10 << (idx & 3);
			}
		}
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Point set to %08x [%d] %04x\n", val, idx, mthd);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_vtx_y32(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 1) {
			insrt(orig.access, 12, 5, 8 + nrand48(ctx->rand48) % 4);
		}
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x033033;
		if (jrand48(ctx->rand48) & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int cls = extr(exp.access, 12, 5);
		bool poly = false, draw = false;
		uint32_t mthd;
		switch (nrand48(ctx->rand48) % 7) {
			case 0:
				mthd = 0x480484 | (jrand48(ctx->rand48) & 0x78);
				draw = true;
				break;
			case 1:
				mthd = 0x490484 | (jrand48(ctx->rand48) & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 2:
				mthd = 0x4a0484 | (jrand48(ctx->rand48) & 0x78);
				draw = !!(mthd & 0x8);
				break;
			case 3:
				mthd = 0x4b0324 + 8 * (nrand48(ctx->rand48) % 3);
				draw = mthd == 0x4b0334;
				break;
			case 4:
				mthd = 0x490584 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				draw = true;
				break;
			case 5:
				mthd = 0x4a0584 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				draw = true;
				break;
			case 6:
				mthd = 0x4b0484 | (jrand48(ctx->rand48) & 0x78);
				poly = true;
				draw = true;
				break;
			default:
				abort();
		}
		nva_wr32(ctx->cnum, mthd, val);
		int idx = extr(exp.xy_misc_0, 28, 4);
		if (nv01_pgraph_is_tex_class(cls)) {
			if (extr(exp.xy_misc_1[0], 24, 1) && extr(exp.xy_misc_1[0], 25, 1)) {
				exp.valid[0] &= ~0xffffff;
			}
		}
		nv01_pgraph_bump_vtxid(&exp);
		insrt(exp.xy_misc_1[0], 0, 1, 0);
		insrt(exp.xy_misc_1[0], 24, 1, 1);
		insrt(exp.xy_misc_1[0], 25, 1, 0);
		nv01_pgraph_set_vtx(&exp, 1, idx, val, true);
		if (!poly) {
			if (idx <= 8)
				exp.valid[0] |= 0x1000 << idx;
			if (cls >= 0x09 && cls <= 0x0b) {
				exp.valid[0] |= 0x10000 << (idx & 3);
			}
		} else {
			if (cls >= 9 && cls <= 0xb) {
				exp.valid[0] |= 0x10000 << (idx & 3);
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, poly);
		// XXX
		if (draw && (cls == 0x11 || cls == 0x12 || cls == 0x13))
			continue;
		nv01_pgraph_dump_state(ctx, &real);
		if (real.status && cls == 0xb) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Point set to %08x [%d] %04x\n", val, idx, mthd);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_size_out(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int is_bitmap = jrand48(ctx->rand48) & 1;
		nva_wr32(ctx->cnum, 0x510308 + is_bitmap * 0x1000c, val);
		int cls = extr(exp.access, 12, 5);
		exp.vtx_x[5] = extr(val, 0, 16);
		exp.vtx_y[5] = extr(val, 16, 16);
		if (cls <= 0xb && cls >= 9)
			exp.valid[0] &= ~0xffffff;
		exp.valid[0] |= 0x020020;
		insrt(exp.xy_misc_0, 28, 4, 0);
		if (cls >= 0x11 && cls <= 0x13)
			insrt(exp.xy_misc_1[0], 0, 1, 0);
		if (cls == 0x10 || (cls >= 9 && cls <= 0xc))
			exp.valid[0] |= 0x100;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Size out set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_size_in(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (!(jrand48(ctx->rand48) & 3))
			insrt(orig.access, 12, 5, jrand48(ctx->rand48) & 1 ? 0x12 : 0x14);
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int which = 0;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0xff00ff;
		if (!(jrand48(ctx->rand48) & 3))
			val &= 0xffff000f;
		switch (nrand48(ctx->rand48) % 3) {
			case 0:
				nva_wr32(ctx->cnum, 0x51030c, val);
				which = 0;
				break;
			case 1:
				nva_wr32(ctx->cnum, 0x520318, val);
				which = 0;
				break;
			case 2:
				nva_wr32(ctx->cnum, 0x53030c, val);
				which = 1;
				break;
			default:
				abort();
		}
		int cls = extr(exp.access, 12, 5);
		exp.vtx_y[1] = 0;
		exp.vtx_x[3] = extr(val, 0, 16);
		exp.vtx_y[3] = -extr(val, 16, 16);
		if (cls >= 0x11 && cls <= 0x13)
			insrt(exp.xy_misc_1[0], 0, 1, 0);
		if (which == 0) {
			if (cls <= 0xb && cls >= 9)
				exp.valid[0] &= ~0xffffff;
			if (cls == 0x10 || (cls >= 9 && cls <= 0xc))
				exp.valid[0] |= 0x100;
		}
		exp.valid[0] |= 0x008008;
		if (cls <= 0xb && cls >= 9)
			exp.valid[0] |= 0x080080;
		exp.edgefill &= ~0x110;
		if (exp.vtx_x[3] < 0x20 && cls == 0x12)
			exp.edgefill |= 0x100;
		if (cls != 0x0d && cls != 0x1d) {
			insrt(exp.xy_misc_4[0], 28, 2, 0);
			insrt(exp.xy_misc_4[1], 28, 2, 0);
		}
		if (exp.vtx_x[3])
			exp.xy_misc_4[0] |= 2 << 28;
		if (exp.vtx_y[3])
			exp.xy_misc_4[1] |= 2 << 28;
		bool zero;
		if (cls == 0x14) {
			uint32_t pixels = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
			zero = (exp.vtx_x[3] == pixels || !exp.vtx_y[3]);
		} else {
			zero = extr(exp.xy_misc_4[0], 28, 2) == 0 ||
				 extr(exp.xy_misc_4[1], 28, 2) == 0;
		}
		insrt(exp.xy_misc_0, 12, 1, zero);
		insrt(exp.xy_misc_0, 28, 4, 0);
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Size in set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_pitch(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int is_itm = jrand48(ctx->rand48) & 1;
		nva_wr32(ctx->cnum, 0x530310 + is_itm * 0x10000, val);
		exp.vtx_x[6] = val;
		exp.valid[0] |= 0x040040;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Pitch set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_rect(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x1ff1ff;
		if (jrand48(ctx->rand48) & 1)
			orig.valid[0] |= 0x033033;
		if (jrand48(ctx->rand48) & 1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		if (jrand48(ctx->rand48) & 1) {
			orig.valid[0] &= ~0x00f00f;
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int cls = exp.access >> 12 & 0x1f;
		bool draw;
		switch (nrand48(ctx->rand48) % 3) {
			case 0:
				nva_wr32(ctx->cnum, 0x500308, val);
				draw = true;
				break;
			case 1:
				nva_wr32(ctx->cnum, 0x54030c, val);
				draw = false;
				break;
			case 2:
				nva_wr32(ctx->cnum, 0x4c0404 | (jrand48(ctx->rand48) & 0x78), val);
				draw = true;
				break;
			default:
				abort();
		}
		if (cls == 0x14) {
			exp.vtx_x[3] = extr(val, 0, 16);
			exp.vtx_y[3] = extr(val, 16, 16);
			nv01_pgraph_vtx_fixup(&exp, 0, 2, exp.vtx_x[3], 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, exp.vtx_y[3], 1, 0, 2);
			exp.valid[0] |= 0x4004;
			insrt(exp.xy_misc_0, 12, 1, 0);
			nv01_pgraph_bump_vtxid(&exp);
		} else if (cls == 0x10) {
			nv01_pgraph_vtx_fixup(&exp, 0, 2, extr(val, 0, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 2, extr(val, 16, 16), 1, 0, 2);
			nv01_pgraph_vtx_fixup(&exp, 0, 3, extr(val, 0, 16), 1, 1, 3);
			nv01_pgraph_vtx_fixup(&exp, 1, 3, extr(val, 16, 16), 1, 1, 3);
			nv01_pgraph_bump_vtxid(&exp);
			nv01_pgraph_bump_vtxid(&exp);
			exp.valid[0] |= 0x00c00c;
		} else if (cls == 0x0c || cls == 0x11 || cls == 0x12 || cls == 0x13) {
			int idx = extr(exp.xy_misc_0, 28, 4);
			nv01_pgraph_vtx_fixup(&exp, 0, idx, extr(val, 0, 16), 1, 0, idx & 3);
			nv01_pgraph_vtx_fixup(&exp, 1, idx, extr(val, 16, 16), 1, 0, idx & 3);
			nv01_pgraph_bump_vtxid(&exp);
			if (idx <= 8)
				exp.valid[0] |= 0x1001 << idx;
		} else {
			nv01_pgraph_vtx_fixup(&exp, 0, 15, extr(val, 0, 16), 1, 15, 1);
			nv01_pgraph_vtx_fixup(&exp, 1, 15, extr(val, 16, 16), 1, 15, 1);
			nv01_pgraph_bump_vtxid(&exp);
			if (cls >= 0x09 && cls <= 0x0b) {
				exp.valid[0] |= 0x080080;
			}
		}
		if (draw)
			nv01_pgraph_prep_draw(&exp, false);
		// XXX
		if (draw && (cls == 0x11 || cls == 0x12 || cls == 0x13))
			continue;
		nv01_pgraph_dump_state(ctx, &real);
		if (real.status && cls == 0x0b) {
			/* Hung PGRAPH... */
			continue;
		}
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Size set to %08x\n", val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_ifc_data(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 1000000; i++) {
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		if (jrand48(ctx->rand48) & 3)
			orig.valid[0] = 0x1ff1ff;
		if (jrand48(ctx->rand48) & 3) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		if (jrand48(ctx->rand48) & 3) {
			orig.valid[0] &= ~0x11000000;
			orig.xy_misc_1[0] &= ~0x330;
		}
		int j;
		for (j = 0; j < 6; j++) {
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_x[j] &= 0xff;
				orig.vtx_x[j] -= 0x80;
			}
			if (jrand48(ctx->rand48) & 1) {
				orig.vtx_y[j] &= 0xff;
				orig.vtx_y[j] -= 0x80;
			}
		}
		if (jrand48(ctx->rand48) & 3)
			insrt(orig.access, 12, 5, 0x11 + (jrand48(ctx->rand48) & 1));
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		uint32_t mthd;
		bool is_bitmap = false;
		switch (nrand48(ctx->rand48) % 3) {
			case 0:
				mthd = 0x510400 | (jrand48(ctx->rand48) & 0x7c);
				break;
			case 1:
				mthd = 0x520400 | (jrand48(ctx->rand48) & 0x7c);
				is_bitmap = true;
				break;
			case 2:
				mthd = 0x530040 | (jrand48(ctx->rand48) & 0x3c);
				break;
			default:
				abort();
		}
		nva_wr32(ctx->cnum, mthd, val);
		exp.misc32[0] = is_bitmap ? pgraph_expand_mono(&exp, val) : val;
		insrt(exp.xy_misc_1[0], 24, 1, 0);
		int cls = exp.access >> 12 & 0x1f;
		int steps = 4 / nv01_pgraph_cpp_in(exp.ctx_switch[0]);
		if (cls == 0x12)
			steps = 0x20;
		if (cls != 0x11 && cls != 0x12)
			goto done;
		if (exp.valid[0] & 0x11000000 && exp.ctx_switch[0] & 0x80)
			exp.intr |= 1 << 16;
		if (extr(exp.canvas_config, 24, 1))
			exp.intr |= 1 << 20;
		if (extr(exp.cliprect_ctrl, 8, 1))
			exp.intr |= 1 << 24;
		int iter;
		iter = 0;
		if (extr(exp.xy_misc_0, 12, 1)) {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			goto done;
		}
		int vidx;
		if (!(exp.xy_misc_1[0] & 1)) {
			exp.vtx_x[6] = exp.vtx_x[4] + exp.vtx_x[5];
			exp.vtx_y[6] = exp.vtx_y[4] + exp.vtx_y[5];
			insrt(exp.xy_misc_1[0], 14, 1, 0);
			insrt(exp.xy_misc_1[0], 18, 1, 0);
			insrt(exp.xy_misc_1[0], 20, 1, 0);
			if ((exp.valid[0] & 0x38038) != 0x38038) {
				exp.intr |= 1 << 16;
				if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
					exp.intr |= 1 << 12;
				goto done;
			}
			nv01_pgraph_iclip_fixup(&exp, 0, exp.vtx_x[6], 0);
			nv01_pgraph_iclip_fixup(&exp, 1, exp.vtx_y[6], 0);
			insrt(exp.xy_misc_1[0], 0, 1, 1);
			if (extr(exp.edgefill, 8, 1)) {
				/* XXX */
				continue;
			}
			insrt(exp.xy_misc_0, 28, 4, 0);
			vidx = 1;
			exp.vtx_y[2] = exp.vtx_y[3] + 1;
			nv01_pgraph_vtx_cmp(&exp, 1, 2);
			nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
			nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
			exp.vtx_x[2] = exp.vtx_x[3];
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
			if ((exp.xy_misc_4[0] & 0xc0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
			if ((exp.xy_misc_4[0] & 0x30) == 0x30)
				exp.intr |= 1 << 12;
		} else {
			if ((exp.valid[0] & 0x38038) != 0x38038)
				exp.intr |= 1 << 16;
			if ((exp.xy_misc_4[0] & 0xf0) || (exp.xy_misc_4[1] & 0xf0))
				exp.intr |= 1 << 12;
		}
restart:;
		vidx = extr(exp.xy_misc_0, 28, 1);
		if (extr(exp.edgefill, 8, 1)) {
			/* XXX */
			continue;
		}
		if (!exp.intr) {
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				nv01_pgraph_bump_vtxid(&exp);
			} else {
				insrt(exp.xy_misc_0, 28, 4, 0);
				vidx = 1;
				bool check_y = false;
				if (extr(exp.xy_misc_4[1], 28, 1)) {
					exp.vtx_y[2]++;
					nv01_pgraph_vtx_add(&exp, 1, 0, 0, exp.vtx_y[0], exp.vtx_y[1], 1);
					check_y = true;
				} else {
					exp.vtx_x[4] += exp.vtx_x[3];
					exp.vtx_y[2] = exp.vtx_y[3] + 1;
					nv01_pgraph_vtx_fixup(&exp, 1, 0, 0, 1, 4, 0);
				}
				nv01_pgraph_vtx_cmp(&exp, 1, 2);
				nv01_pgraph_vtx_fixup(&exp, 0, 0, 0, 1, 4, 0);
				if (extr(exp.xy_misc_4[0], 28, 1)) {
					nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], ~exp.vtx_x[2], 1);
					exp.vtx_x[2] += exp.vtx_x[3];
					nv01_pgraph_vtx_cmp(&exp, 0, 2);
					if (extr(exp.xy_misc_4[0], 28, 1)) {
						nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
						if ((exp.xy_misc_4[0] & 0x30) == 0x30)
							exp.intr |= 1 << 12;
						check_y = true;
					} else {
						if ((exp.xy_misc_4[0] & 0x20))
							exp.intr |= 1 << 12;
					}
					if (exp.xy_misc_4[1] & 0x10 && check_y)
						exp.intr |= 1 << 12;
					iter++;
					if (iter > 10000) {
						/* This is a hang - skip this test run.  */
						continue;
					}
					goto restart;
				}
				exp.vtx_x[2] = exp.vtx_x[3];
			}
			exp.vtx_x[2] -= steps;
			nv01_pgraph_vtx_cmp(&exp, 0, 2);
			nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[vidx ^ 1], steps, 0);
			if (extr(exp.xy_misc_4[0], 28, 1)) {
				nv01_pgraph_vtx_add(&exp, 0, vidx, vidx, exp.vtx_x[2], exp.vtx_x[vidx], 0);
			}
		} else {
			nv01_pgraph_bump_vtxid(&exp);
			if (extr(exp.xy_misc_4[0], 29, 1)) {
				exp.vtx_x[2] -= steps;
				nv01_pgraph_vtx_cmp(&exp, 0, 2);
			} else if (extr(exp.xy_misc_4[1], 28, 1)) {
				exp.vtx_y[2]++;
			}
		}
done:
		if (exp.intr)
			exp.access &= ~0x101;
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("IFC set to %08x [%d]\n", val, steps);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_mthd_bitmap_color(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 10000; i++) {
		int idx = jrand48(ctx->rand48)&1;
		uint32_t val = jrand48(ctx->rand48);
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		nva_wr32(ctx->cnum, 0x520308 + idx * 4, val);
		exp.bitmap_color[idx] = pgraph_to_a1r10g10b10(pgraph_expand_color(&exp, val));
		nv01_pgraph_dump_state(ctx, &real);
		if (nv01_pgraph_cmp_state(&orig, &exp, &real)) {
			printf("Color %d set to %08x\n", idx, val);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_simple(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (jrand48(ctx->rand48)&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[nrand48(ctx->rand48) % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[nrand48(ctx->rand48) % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (jrand48(ctx->rand48)&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		/* XXX causes interrupts */
		orig.valid[0] &= ~0x11000000;
		insrt(orig.access, 12, 5, 8);
		insrt(orig.pfb_config, 4, 3, 3);
		int x = jrand48(ctx->rand48) & 0x3ff;
		int y = jrand48(ctx->rand48) & 0xff;
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, x);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, y);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, x);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, y);
		if (jrand48(ctx->rand48)&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (jrand48(ctx->rand48) & 1) << 30; /* perturb alpha */
			if (jrand48(ctx->rand48)&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (nrand48(ctx->rand48) % 30);
			}
			orig.chroma = ckey;
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int bfmt = extr(exp.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(exp.pfb_config, 12, 1))
			bufmask = 1;
		uint32_t addr0 = nv01_pgraph_pixel_addr(&exp, x, y, 0);
		uint32_t addr1 = nv01_pgraph_pixel_addr(&exp, x, y, 1);
		nva_wr32(ctx->cnum, 0x1000000+(addr0&~3), jrand48(ctx->rand48));
		nva_wr32(ctx->cnum, 0x1000000+(addr1&~3), jrand48(ctx->rand48));
		uint32_t pixel0 = nva_rd32(ctx->cnum, 0x1000000+(addr0&~3)) >> (addr0 & 3) * 8;
		uint32_t pixel1 = nva_rd32(ctx->cnum, 0x1000000+(addr1&~3)) >> (addr1 & 3) * 8;
		pixel0 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		pixel1 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		nva_wr32(ctx->cnum, 0x480400, y << 16 | x);
		uint32_t epixel0 = pixel0, epixel1 = pixel1;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		if (bufmask & 1 && cliprect_pass)
			epixel0 = nv01_pgraph_solid_rop(&exp, x, y, pixel0);
		if (bufmask & 2 && (cliprect_pass || extr(exp.canvas_config, 4, 1)))
			epixel1 = nv01_pgraph_solid_rop(&exp, x, y, pixel1);
		exp.vtx_x[0] = x;
		exp.vtx_y[0] = y;
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x10000000;
		exp.xy_misc_1[0] &= ~0x03000001;
		exp.xy_misc_1[0] |= 0x01000000;
		nv01_pgraph_set_xym2(&exp, 0, 0, 0, 0, 0, x == 0x400 ? 8 : x ? 0 : 2);
		nv01_pgraph_set_xym2(&exp, 1, 0, 0, 0, 0, y == 0x400 ? 8 : y ? 0 : 2);
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		nv01_pgraph_dump_state(ctx, &real);
		uint32_t rpixel0 = nva_rd32(ctx->cnum, 0x1000000+(addr0&~3)) >> (addr0 & 3) * 8;
		uint32_t rpixel1 = nva_rd32(ctx->cnum, 0x1000000+(addr1&~3)) >> (addr1 & 3) * 8;
		rpixel0 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		rpixel1 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		if (!extr(exp.pfb_config, 12, 1))
			rpixel1 = epixel1;
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, epixel0 != rpixel0 || epixel1 != rpixel1)) {
			printf("Iter %05d: Point (%03x,%02x) orig %08x/%08x expected %08x/%08x real %08x/%08x source %08x canvas %08x pfb %08x ctx %08x beta %02x fmt %d\n", i, x, y, pixel0, pixel1, epixel0, epixel1, rpixel0, rpixel1, exp.misc32[0], exp.canvas_config, exp.pfb_config, exp.ctx_switch[0], exp.beta >> 23, bfmt%5);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int test_rop_blit(struct hwtest_ctx *ctx) {
	int i;
	for (i = 0; i < 100000; i++) {
		struct pgraph_state orig, exp, real;
		nv01_pgraph_gen_state(ctx, &orig);
		orig.notify &= ~0x110000;
		orig.dst_canvas_min = 0;
		orig.dst_canvas_max = 0x01000400;
		/* XXX bits 8-9 affect rendering */
		/* XXX bits 12-19 affect xy_misc_4 clip status */
		orig.xy_misc_1[0] &= 0xfff00cff;
		/* avoid invalid ops */
		orig.ctx_switch[0] &= ~0x001f;
		if (jrand48(ctx->rand48)&1) {
			int ops[] = {
				0x00, 0x0f,
				0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
				0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
				0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x17,
			};
			orig.ctx_switch[0] |= ops[nrand48(ctx->rand48) % ARRAY_SIZE(ops)];
		} else {
			/* BLEND needs more testing */
			int ops[] = { 0x18, 0x19, 0x1a, 0x1b, 0x1c };
			orig.ctx_switch[0] |= ops[nrand48(ctx->rand48) % ARRAY_SIZE(ops)];
			/* XXX Y8 blend? */
			orig.pfb_config |= 0x200;
		}
		orig.pattern_config = nrand48(ctx->rand48)%3; /* shape 3 is a rather ugly hole in Karnough map */
		if (jrand48(ctx->rand48)&1) {
			orig.xy_misc_4[0] &= ~0xf0;
			orig.xy_misc_4[1] &= ~0xf0;
		}
		orig.xy_misc_4[0] &= ~0xf000;
		orig.xy_misc_4[1] &= ~0xf000;
		orig.valid[0] &= ~0x11000000;
		orig.valid[0] |= 0xf10f;
		insrt(orig.access, 12, 5, 0x10);
		insrt(orig.pfb_config, 4, 3, 3);
		int x = jrand48(ctx->rand48) & 0x1ff;
		int y = jrand48(ctx->rand48) & 0xff;
		int sx = (jrand48(ctx->rand48) & 0x3ff) + 0x200;
		int sy = jrand48(ctx->rand48) & 0xff;
		orig.vtx_x[0] = sx;
		orig.vtx_y[0] = sy;
		orig.vtx_x[1] = x;
		orig.vtx_y[1] = y;
		orig.xy_misc_0 &= ~0xf0000000;
		orig.xy_misc_0 |= 0x20000000;
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 0, 16, x);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_min[jrand48(ctx->rand48)&1], 16, 16, y);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 0, 16, x);
		if (jrand48(ctx->rand48)&1)
			insrt(orig.cliprect_max[jrand48(ctx->rand48)&1], 16, 16, y);
		if (jrand48(ctx->rand48)&1) {
			/* it's vanishingly rare for the chroma key to match perfectly by random, so boost the odds */
			uint32_t ckey = pgraph_to_a1r10g10b10(pgraph_expand_color(&orig, orig.misc32[0]));
			ckey ^= (jrand48(ctx->rand48) & 1) << 30; /* perturb alpha */
			if (jrand48(ctx->rand48)&1) {
				/* perturb it a bit to check which bits have to match */
				ckey ^= 1 << (nrand48(ctx->rand48) % 30);
			}
			orig.chroma = ckey;
		}
		nv01_pgraph_load_state(ctx, &orig);
		exp = orig;
		int bfmt = extr(exp.ctx_switch[0], 9, 4);
		int bufmask = (bfmt / 5 + 1) & 3;
		if (!extr(exp.pfb_config, 12, 1))
			bufmask = 1;
		uint32_t addr0 = nv01_pgraph_pixel_addr(&exp, x, y, 0);
		uint32_t addr1 = nv01_pgraph_pixel_addr(&exp, x, y, 1);
		uint32_t saddr0 = nv01_pgraph_pixel_addr(&exp, sx, sy, 0);
		uint32_t saddr1 = nv01_pgraph_pixel_addr(&exp, sx, sy, 1);
		nva_wr32(ctx->cnum, 0x1000000+(addr0&~3), jrand48(ctx->rand48));
		nva_wr32(ctx->cnum, 0x1000000+(addr1&~3), jrand48(ctx->rand48));
		nva_wr32(ctx->cnum, 0x1000000+(saddr0&~3), jrand48(ctx->rand48));
		nva_wr32(ctx->cnum, 0x1000000+(saddr1&~3), jrand48(ctx->rand48));
		uint32_t pixel0 = nva_rd32(ctx->cnum, 0x1000000+(addr0&~3)) >> (addr0 & 3) * 8;
		uint32_t pixel1 = nva_rd32(ctx->cnum, 0x1000000+(addr1&~3)) >> (addr1 & 3) * 8;
		uint32_t spixel0 = nva_rd32(ctx->cnum, 0x1000000+(saddr0&~3)) >> (saddr0 & 3) * 8;
		uint32_t spixel1 = nva_rd32(ctx->cnum, 0x1000000+(saddr1&~3)) >> (saddr1 & 3) * 8;
		if (sx >= 0x400)
			spixel0 = spixel1 = 0;
		pixel0 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		pixel1 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		spixel0 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		spixel1 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		nva_wr32(ctx->cnum, 0x500308, 1 << 16 | 1);
		uint32_t epixel0 = pixel0, epixel1 = pixel1;
		if (!pgraph_cliprect_pass(&exp, sx, sy)) {
			spixel0 = 0;
			if (!extr(exp.canvas_config, 4, 1))
				spixel1 = 0;
		}
		if (!extr(exp.pfb_config, 12, 1))
			spixel1 = spixel0;
		bool cliprect_pass = pgraph_cliprect_pass(&exp, x, y);
		struct pgraph_color s = nv01_pgraph_expand_surf(&exp, extr(exp.ctx_switch[0], 13, 1) ? spixel1 : spixel0);
		if (bufmask & 1 && cliprect_pass)
			epixel0 = nv01_pgraph_rop(&exp, x, y, pixel0, s);
		if (bufmask & 2 && (cliprect_pass || extr(exp.canvas_config, 4, 1)))
			epixel1 = nv01_pgraph_rop(&exp, x, y, pixel1, s);
		nv01_pgraph_vtx_fixup(&exp, 0, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 1, 2, 1, 1, 0, 2);
		nv01_pgraph_vtx_fixup(&exp, 0, 3, 1, 1, 1, 3);
		nv01_pgraph_vtx_fixup(&exp, 1, 3, 1, 1, 1, 3);
		exp.xy_misc_0 &= ~0xf0000000;
		exp.xy_misc_0 |= 0x00000000;
		exp.valid[0] &= ~0xffffff;
		if (extr(exp.cliprect_ctrl, 8, 1)) {
			exp.intr |= 1 << 24;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		if (extr(exp.canvas_config, 24, 1)) {
			exp.intr |= 1 << 20;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		if (extr(exp.xy_misc_4[0], 4, 4) || extr(exp.xy_misc_4[1], 4, 4)) {
			exp.intr |= 1 << 12;
			exp.access &= ~0x101;
			epixel0 = pixel0;
			epixel1 = pixel1;
		}
		nv01_pgraph_dump_state(ctx, &real);
		uint32_t rpixel0 = nva_rd32(ctx->cnum, 0x1000000+(addr0&~3)) >> (addr0 & 3) * 8;
		uint32_t rpixel1 = nva_rd32(ctx->cnum, 0x1000000+(addr1&~3)) >> (addr1 & 3) * 8;
		rpixel0 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		rpixel1 &= bflmask(nv01_pgraph_cpp(exp.pfb_config)*8);
		if (!extr(exp.pfb_config, 12, 1))
			rpixel1 = epixel1;
		if (nv01_pgraph_cmp_state(&orig, &exp, &real, epixel0 != rpixel0 || epixel1 != rpixel1)) {
			printf("Iter %05d: Point (%03x,%02x) source %08x/%08x orig %08x/%08x expected %08x/%08x real %08x/%08x source %08x canvas %08x pfb %08x ctx %08x beta %02x fmt %d\n", i, x, y, spixel0, spixel1, pixel0, pixel1, epixel0, epixel1, rpixel0, rpixel1, exp.misc32[0], exp.canvas_config, exp.pfb_config, exp.ctx_switch[0], exp.beta >> 23, bfmt%5);
			return HWTEST_RES_FAIL;
		}
	}
	return HWTEST_RES_PASS;
}

static int nv01_pgraph_prep(struct hwtest_ctx *ctx) {
	if (ctx->chipset.chipset != 0x01)
		return HWTEST_RES_NA;
	if (!(nva_rd32(ctx->cnum, 0x200) & 1 << 24)) {
		printf("Mem controller not up.\n");
		return HWTEST_RES_UNPREP;
	}
	nva_wr32(ctx->cnum, 0x200, 0xffffffff);
	return HWTEST_RES_PASS;
}

static int scan_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int state_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int misc_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int simple_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int xy_mthd_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

static int rop_prep(struct hwtest_ctx *ctx) {
	return HWTEST_RES_PASS;
}

HWTEST_DEF_GROUP(scan,
	HWTEST_TEST(test_scan_access, 0),
	HWTEST_TEST(test_scan_debug, 0),
	HWTEST_TEST(test_scan_control, 0),
	HWTEST_TEST(test_scan_canvas, 0),
	HWTEST_TEST(test_scan_vtx, 0),
	HWTEST_TEST(test_scan_clip, 0),
	HWTEST_TEST(test_scan_context, 0),
	HWTEST_TEST(test_scan_vstate, 0),
)

HWTEST_DEF_GROUP(state,
	HWTEST_TEST(test_state, 0),
	HWTEST_TEST(test_soft_reset, 0),
	HWTEST_TEST(test_mmio_read, 0),
	HWTEST_TEST(test_mmio_write, 0),
	HWTEST_TEST(test_mmio_vtx_write, 0),
	HWTEST_TEST(test_mmio_iclip_write, 0),
	HWTEST_TEST(test_mmio_uclip_write, 0),
	HWTEST_TEST(test_mmio_clip_status, 0),
)

HWTEST_DEF_GROUP(misc_mthd,
	HWTEST_TEST(test_mthd_invalid, 0),
	HWTEST_TEST(test_mthd_ctx_switch, 0),
	HWTEST_TEST(test_mthd_notify, 0),
)

HWTEST_DEF_GROUP(simple_mthd,
	HWTEST_TEST(test_mthd_beta, 0),
	HWTEST_TEST(test_mthd_rop, 0),
	HWTEST_TEST(test_mthd_chroma_plane, 0),
	HWTEST_TEST(test_mthd_pattern_shape, 0),
	HWTEST_TEST(test_mthd_pattern_mono_color, 0),
	HWTEST_TEST(test_mthd_pattern_mono_bitmap, 0),
	HWTEST_TEST(test_mthd_solid_color, 0),
	HWTEST_TEST(test_mthd_subdivide, 0),
	HWTEST_TEST(test_mthd_vtx_beta, 0),
	HWTEST_TEST(test_mthd_bitmap_color, 0),
)

HWTEST_DEF_GROUP(xy_mthd,
	HWTEST_TEST(test_mthd_clip, 0),
	HWTEST_TEST(test_mthd_vtx, 0),
	HWTEST_TEST(test_mthd_vtx_x32, 0),
	HWTEST_TEST(test_mthd_vtx_y32, 0),
	HWTEST_TEST(test_mthd_ifc_size_in, 0),
	HWTEST_TEST(test_mthd_ifc_size_out, 0),
	HWTEST_TEST(test_mthd_pitch, 0),
	HWTEST_TEST(test_mthd_rect, 0),
	HWTEST_TEST(test_mthd_ifc_data, 0),
)

HWTEST_DEF_GROUP(rop,
	HWTEST_TEST(test_rop_simple, 0),
	HWTEST_TEST(test_rop_blit, 0),
)

HWTEST_DEF_GROUP(nv01_pgraph,
	HWTEST_GROUP(scan),
	HWTEST_GROUP(state),
	HWTEST_GROUP(misc_mthd),
	HWTEST_GROUP(simple_mthd),
	HWTEST_GROUP(xy_mthd),
	HWTEST_GROUP(rop),
)
