/*
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <mach/clk-provider.h>
#include <mach/clock-generic.h>

/* ==================== Mux clock ==================== */

static int parent_to_src_sel(struct mux_clk *mux, struct clk *p)
{
	int i;

	for (i = 0; i < mux->num_parents; i++) {
		if (mux->parents[i].src == p)
			return mux->parents[i].sel;
	}

	return -EINVAL;
}

static int mux_set_parent(struct clk *c, struct clk *p)
{
	struct mux_clk *mux = to_mux_clk(c);
	int sel = parent_to_src_sel(mux, p);
	struct clk *old_parent;
	int rc = 0, i;
	unsigned long flags;

	if (sel < 0 && mux->rec_set_par) {
		for (i = 0; i < mux->num_parents; i++) {
			rc = clk_set_parent(mux->parents[i].src, p);
			if (!rc) {
				sel = mux->parents[i].sel;
				/*
				 * This is necessary to ensure prepare/enable
				 * counts get propagated correctly.
				 */
				p = mux->parents[i].src;
				break;
			}
		}
	}

	if (sel < 0)
		return sel;

	rc = __clk_pre_reparent(c, p, &flags);
	if (rc)
		goto out;

	rc = mux->ops->set_mux_sel(mux, sel);
	if (rc)
		goto set_fail;

	old_parent = c->parent;
	c->parent = p;
	c->rate = clk_get_rate(p);
	__clk_post_reparent(c, old_parent, &flags);

	return 0;

set_fail:
	__clk_post_reparent(c, p, &flags);
out:
	return rc;
}

static long mux_round_rate(struct clk *c, unsigned long rate)
{
	struct mux_clk *mux = to_mux_clk(c);
	int i;
	unsigned long prate, rrate = 0;

	for (i = 0; i < mux->num_parents; i++) {
		prate = clk_round_rate(mux->parents[i].src, rate);
		if (is_better_rate(rate, rrate, prate))
			rrate = prate;
	}
	if (!rrate)
		return -EINVAL;

	return rrate;
}

static int mux_set_rate(struct clk *c, unsigned long rate)
{
	struct mux_clk *mux = to_mux_clk(c);
	struct clk *new_parent = NULL;
	int rc = 0, i;
	unsigned long new_par_curr_rate;
	unsigned long flags;

	for (i = 0; i < mux->num_parents; i++) {
		if (clk_round_rate(mux->parents[i].src, rate) == rate) {
			new_parent = mux->parents[i].src;
			break;
		}
	}
	if (new_parent == NULL)
		return -EINVAL;

	/*
	 * Switch to safe parent since the old and new parent might be the
	 * same and the parent might temporarily turn off while switching
	 * rates.
	 */
	if (mux->safe_sel >= 0) {
		/*
		 * Some mux implementations might switch to/from a low power
		 * parent as part of their disable/enable ops. Grab the
		 * enable lock to avoid racing with these implementations.
		 */
		spin_lock_irqsave(&c->lock, flags);
		rc = mux->ops->set_mux_sel(mux, mux->safe_sel);
		spin_unlock_irqrestore(&c->lock, flags);
	}
	if (rc)
		return rc;

	new_par_curr_rate = clk_get_rate(new_parent);
	rc = clk_set_rate(new_parent, rate);
	if (rc)
		goto set_rate_fail;

	rc = mux_set_parent(c, new_parent);
	if (rc)
		goto set_par_fail;

	return 0;

set_par_fail:
	clk_set_rate(new_parent, new_par_curr_rate);
set_rate_fail:
	WARN(mux->ops->set_mux_sel(mux, parent_to_src_sel(mux, c->parent)),
		"Set rate failed for %s. Also in bad state!\n", c->dbg_name);
	return rc;
}

static int mux_enable(struct clk *c)
{
	struct mux_clk *mux = to_mux_clk(c);
	if (mux->ops->enable)
		return mux->ops->enable(mux);
	return 0;
}

static void mux_disable(struct clk *c)
{
	struct mux_clk *mux = to_mux_clk(c);
	if (mux->ops->disable)
		return mux->ops->disable(mux);
}

static struct clk *mux_get_parent(struct clk *c)
{
	struct mux_clk *mux = to_mux_clk(c);
	int sel = mux->ops->get_mux_sel(mux);
	int i;

	for (i = 0; i < mux->num_parents; i++) {
		if (mux->parents[i].sel == sel)
			return mux->parents[i].src;
	}

	/* Unfamiliar parent. */
	return NULL;
}

static enum handoff mux_handoff(struct clk *c)
{
	struct mux_clk *mux = to_mux_clk(c);

	c->rate = clk_get_rate(c->parent);
	mux->safe_sel = parent_to_src_sel(mux, mux->safe_parent);

	if (mux->en_mask && mux->ops && mux->ops->is_enabled)
		return mux->ops->is_enabled(mux)
			? HANDOFF_ENABLED_CLK
			: HANDOFF_DISABLED_CLK;

	/*
	 * If this function returns 'enabled' even when the clock downstream
	 * of this clock is disabled, then handoff code will unnecessarily
	 * enable the current parent of this clock. If this function always
	 * returns 'disabled' and a clock downstream is on, the clock handoff
	 * code will bump up the ref count for this clock and its current
	 * parent as necessary. So, clocks without an actual HW gate can
	 * always return disabled.
	 */
	return HANDOFF_DISABLED_CLK;
}

struct clk_ops clk_ops_gen_mux = {
	.enable = mux_enable,
	.disable = mux_disable,
	.set_parent = mux_set_parent,
	.round_rate = mux_round_rate,
	.set_rate = mux_set_rate,
	.handoff = mux_handoff,
	.get_parent = mux_get_parent,
};

/* ==================== Divider clock ==================== */

static long __div_round_rate(struct div_data *data, unsigned long rate,
	struct clk *parent, unsigned int *best_div)
{
	unsigned int div, min_div, max_div, rrate_div = 1;
	unsigned long p_rrate, rrate = 0;

	rate = max(rate, 1UL);

	min_div = max(data->min_div, 1U);
	max_div = min(data->max_div, (unsigned int) (ULONG_MAX / rate));

	for (div = min_div; div <= max_div; div++) {
		p_rrate = clk_round_rate(parent, rate * div);
		if (IS_ERR_VALUE(p_rrate))
			break;

		p_rrate /= div;

		if (is_better_rate(rate, rrate, p_rrate)) {
			rrate = p_rrate;
			rrate_div = div;
		}

		/*
		 * Trying higher dividers is only going to ask the parent for
		 * a higher rate. If it can't even output a rate higher than
		 * the one we request for this divider, the parent is not
		 * going to be able to output an even higher rate required
		 * for a higher divider. So, stop trying higher dividers.
		 */
		if (p_rrate < rate)
			break;

		if (rrate <= rate + data->rate_margin)
			break;
	}

	if (!rrate)
		return -EINVAL;
	if (best_div)
		*best_div = rrate_div;

	return rrate;
}

static long div_round_rate(struct clk *c, unsigned long rate)
{
	struct div_clk *d = to_div_clk(c);

	return __div_round_rate(&d->data, rate, c->parent, NULL);
}

static int div_set_rate(struct clk *c, unsigned long rate)
{
	struct div_clk *d = to_div_clk(c);
	int div = 0, rc = 0;
	long rrate, old_prate;
	struct div_data *data = &d->data;

	rrate = __div_round_rate(data, rate, c->parent, &div);
	if (rrate != rate)
		return -EINVAL;

	/*
	 * For fixed divider clock we don't want to return an error if the
	 * requested rate matches the achievable rate. So, don't check for
	 * !d->ops and return an error. __div_round_rate() ensures div ==
	 * d->div if !d->ops.
	 */
	if (div > data->div)
		rc = d->ops->set_div(d, div);
	if (rc)
		return rc;

	old_prate = clk_get_rate(c->parent);
	rc = clk_set_rate(c->parent, rate * div);
	if (rc)
		goto set_rate_fail;

	if (div < data->div)
		rc = d->ops->set_div(d, div);
	if (rc)
		goto div_dec_fail;

	data->div = div;

	return 0;

div_dec_fail:
	WARN(clk_set_rate(c->parent, old_prate),
		"Set rate failed for %s. Also in bad state!\n", c->dbg_name);
set_rate_fail:
	if (div > data->div)
		WARN(d->ops->set_div(d, data->div),
			"Set rate failed for %s. Also in bad state!\n",
			c->dbg_name);
	return rc;
}

static int div_enable(struct clk *c)
{
	struct div_clk *d = to_div_clk(c);
	if (d->ops && d->ops->enable)
		return d->ops->enable(d);
	return 0;
}

static void div_disable(struct clk *c)
{
	struct div_clk *d = to_div_clk(c);
	if (d->ops && d->ops->disable)
		return d->ops->disable(d);
}

static enum handoff div_handoff(struct clk *c)
{
	struct div_clk *d = to_div_clk(c);
	unsigned int div = d->data.div;

	if (d->ops && d->ops->get_div)
		div = max(d->ops->get_div(d), 1);
	div = max(div, 1U);
	c->rate = clk_get_rate(c->parent) / div;

	if (!d->ops || !d->ops->set_div)
		d->data.min_div = d->data.max_div = div;
	d->data.div = div;

	if (d->en_mask && d->ops && d->ops->is_enabled)
		return d->ops->is_enabled(d)
			? HANDOFF_ENABLED_CLK
			: HANDOFF_DISABLED_CLK;

	/*
	 * If this function returns 'enabled' even when the clock downstream
	 * of this clock is disabled, then handoff code will unnecessarily
	 * enable the current parent of this clock. If this function always
	 * returns 'disabled' and a clock downstream is on, the clock handoff
	 * code will bump up the ref count for this clock and its current
	 * parent as necessary. So, clocks without an actual HW gate can
	 * always return disabled.
	 */
	return HANDOFF_DISABLED_CLK;
}

struct clk_ops clk_ops_div = {
	.enable = div_enable,
	.disable = div_disable,
	.round_rate = div_round_rate,
	.set_rate = div_set_rate,
	.handoff = div_handoff,
};

static long __slave_div_round_rate(struct clk *c, unsigned long rate,
					int *best_div)
{
	struct div_clk *d = to_div_clk(c);
	unsigned int div, min_div, max_div;
	long p_rate;

	rate = max(rate, 1UL);

	min_div = d->data.min_div;
	max_div = d->data.max_div;

	p_rate = clk_get_rate(c->parent);
	div = p_rate / rate;
	div = max(div, min_div);
	div = min(div, max_div);
	if (best_div)
		*best_div = div;

	return p_rate / div;
}

static long slave_div_round_rate(struct clk *c, unsigned long rate)
{
	return __slave_div_round_rate(c, rate, NULL);
}

static int slave_div_set_rate(struct clk *c, unsigned long rate)
{
	struct div_clk *d = to_div_clk(c);
	int div, rc = 0;
	long rrate;

	rrate = __slave_div_round_rate(c, rate, &div);
	if (rrate != rate)
		return -EINVAL;

	if (div == d->data.div)
		return 0;

	/*
	 * For fixed divider clock we don't want to return an error if the
	 * requested rate matches the achievable rate. So, don't check for
	 * !d->ops and return an error. __slave_div_round_rate() ensures
	 * div == d->data.div if !d->ops.
	 */
	rc = d->ops->set_div(d, div);
	if (rc)
		return rc;

	d->data.div = div;

	return 0;
}

static unsigned long slave_div_get_rate(struct clk *c)
{
	struct div_clk *d = to_div_clk(c);
	if (!d->data.div)
		return 0;
	return clk_get_rate(c->parent) / d->data.div;
}

struct clk_ops clk_ops_slave_div = {
	.enable = div_enable,
	.disable = div_disable,
	.round_rate = slave_div_round_rate,
	.set_rate = slave_div_set_rate,
	.get_rate = slave_div_get_rate,
	.handoff = div_handoff,
};


/**
 * External clock
 * Some clock controllers have input clock signal that come from outside the
 * clock controller. That input clock signal might then be used as a source for
 * several clocks inside the clock controller. This external clock
 * implementation models this input clock signal by just passing on the requests
 * to the clock's parent, the original external clock source. The driver for the
 * clock controller should clk_get() the original external clock in the probe
 * function and set is as a parent to this external clock..
 */

static long ext_round_rate(struct clk *c, unsigned long rate)
{
	return clk_round_rate(c->parent, rate);
}

static int ext_set_rate(struct clk *c, unsigned long rate)
{
	return clk_set_rate(c->parent, rate);
}

static unsigned long ext_get_rate(struct clk *c)
{
	return clk_get_rate(c->parent);
}

static int ext_set_parent(struct clk *c, struct clk *p)
{
	return clk_set_parent(c->parent, p);
}

static enum handoff ext_handoff(struct clk *c)
{
	c->rate = clk_get_rate(c->parent);
	/* Similar reasoning applied in div_handoff, see comment there. */
	return HANDOFF_DISABLED_CLK;
}

struct clk_ops clk_ops_ext = {
	.handoff = ext_handoff,
	.round_rate = ext_round_rate,
	.set_rate = ext_set_rate,
	.get_rate = ext_get_rate,
	.set_parent = ext_set_parent,
};

