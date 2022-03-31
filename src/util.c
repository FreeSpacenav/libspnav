/*
This file is part of libspnav, part of the spacenav project (spacenav.sf.net)
Copyright (C) 2007-2022 John Tsiombikas <nuclear@member.fsf.org>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/
#include <string.h>
#include <math.h>
#include "spnav.h"


static void vec3_cross(float *res, const float *va, const float *vb);
static void vec3_qrot(float *res, const float *vec, const float *quat);
static void quat_mul(float *qa, const float *qb);
static void quat_invert(float *q);
static void quat_rotate(float *q, float angle, float x, float y, float z);
static void mat4_mul(float *ma, const float *mb);
static void mat4_translation(float *mat, float x, float y, float z);
static void mat4_quat(float *mat, const float *quat);


void spnav_posrot_init(struct spnav_posrot *pr)
{
	pr->pos[0] = pr->pos[1] = pr->pos[2] = 0.0f;
	pr->rot[0] = pr->rot[1] = pr->rot[2] = 0.0f;
	pr->rot[3] = 1.0f;
}

void spnav_posrot_moveobj(struct spnav_posrot *pr, const struct spnav_event_motion *ev)
{
	float len;

	pr->pos[0] += ev->x * 0.001;
	pr->pos[1] += ev->y * 0.001;
	pr->pos[2] -= ev->z * 0.001;

	if((len = sqrt(ev->rx * ev->rx + ev->ry * ev->ry + ev->rz * ev->rz)) != 0.0f) {
		float x = ev->rx / len;
		float y = ev->ry / len;
		float z = -ev->rz / len;
		quat_rotate(pr->rot, len * 0.001, x, y, z);
	}
}

void spnav_posrot_moveview(struct spnav_posrot *pr, const struct spnav_event_motion *ev)
{
	float trans[3], len;

	if((len = sqrt(ev->rx * ev->rx + ev->ry * ev->ry + ev->rz * ev->rz)) != 0.0f) {
		float x = -ev->rx / len;
		float y = -ev->ry / len;
		float z = ev->rz / len;
		quat_rotate(pr->rot, len * 0.001, x, y, z);
	}

	trans[0] = -ev->x * 0.001;
	trans[1] = -ev->y * 0.001;
	trans[2] = ev->z * 0.001;
	vec3_qrot(trans, trans, pr->rot);

	pr->pos[0] += trans[0];
	pr->pos[1] += trans[1];
	pr->pos[2] += trans[2];
}

void spnav_matrix_obj(float *mat, const struct spnav_posrot *pr)
{
	float tmp[16];
	mat4_quat(mat, pr->rot);
	mat4_translation(tmp, pr->pos[0], pr->pos[1], pr->pos[2]);
	mat4_mul(mat, tmp);
}

void spnav_matrix_view(float *mat, const struct spnav_posrot *pr)
{
	float tmp[16];
	mat4_translation(mat, pr->pos[0], pr->pos[1], pr->pos[2]);
	mat4_quat(tmp, pr->rot);
	mat4_mul(mat, tmp);
}


/* ---- vector/matrix/quaternion math operations ---- */

static void vec3_cross(float *res, const float *va, const float *vb)
{
	res[0] = va[1] * vb[2] - va[2] * vb[1];
	res[1] = va[2] * vb[0] - va[0] * vb[2];
	res[2] = va[0] * vb[1] - va[1] * vb[0];
}

static void vec3_qrot(float *res, const float *vec, const float *quat)
{
	float vq[4], inv_q[4], tmp_q[4];

	inv_q[0] = tmp_q[0] = quat[0];
	inv_q[1] = tmp_q[1] = quat[1];
	inv_q[2] = tmp_q[2] = quat[2];
	inv_q[3] = tmp_q[3] = quat[3];

	vq[0] = vec[0];
	vq[1] = vec[1];
	vq[2] = vec[2];
	vq[3] = 0.0f;

	quat_invert(inv_q);
	quat_mul(tmp_q, vq);
	quat_mul(tmp_q, inv_q);

	res[0] = tmp_q[0];
	res[1] = tmp_q[1];
	res[2] = tmp_q[2];
}

static void quat_mul(float *qa, const float *qb)
{
	float x, y, z, dot, cross[3];

	dot = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2];
	vec3_cross(cross, qb, qa);

	x = qa[3] * qb[0] + qb[3] * qa[0] + cross[0];
	y = qa[3] * qb[1] + qb[3] * qa[1] + cross[1];
	z = qa[3] * qb[2] + qb[3] * qa[2] + cross[2];
	qa[3] = qa[3] * qb[3] - dot;
	qa[0] = x;
	qa[1] = y;
	qa[2] = z;
}

static void quat_invert(float *q)
{
	float s, len_sq = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	/* conjugate */
	q[0] = -q[0];
	q[1] = -q[1];
	q[2] = -q[2];
	if(len_sq != 0.0f) {
		s = 1.0f / len_sq;
		q[0] *= s;
		q[1] *= s;
		q[2] *= s;
		q[3] *= s;
	}
}

static void quat_rotate(float *q, float angle, float x, float y, float z)
{
	float rq[4];
	float half = angle * 0.5f;
	float sin_half = sin(half);

	rq[3] = cos(half);
	rq[0] = x * sin_half;
	rq[1] = y * sin_half;
	rq[2] = z * sin_half;

	quat_mul(q, rq);
}

static void mat4_mul(float *ma, const float *mb)
{
	int i, j;
	float res[16];
	float *resptr = res;
	float *arow = ma;

	for(i=0; i<4; i++) {
		for(j=0; j<4; j++) {
			*resptr++ = arow[0] * mb[j] + arow[1] * mb[4 + j] +
				arow[2] * mb[8 + j] + arow[3] * mb[12 + j];
		}
		arow += 4;
	}
	memcpy(ma, res, sizeof res);
}

static void mat4_translation(float *mat, float x, float y, float z)
{
	static const float id[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
	memcpy(mat, id, sizeof id);
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

static void mat4_quat(float *m, const float *q)
{
	float xsq2 = 2.0f * q[0] * q[0];
	float ysq2 = 2.0f * q[1] * q[1];
	float zsq2 = 2.0f * q[2] * q[2];
	float sx = 1.0f - ysq2 - zsq2;
	float sy = 1.0f - xsq2 - zsq2;
	float sz = 1.0f - xsq2 - ysq2;

	m[3] = m[7] = m[11] = m[12] = m[13] = m[14] = 0.0f;
	m[15] = 1.0f;

	m[0] = sx;
	m[1] = 2.0f * q[0] * q[1] + 2.0f * q[3] * q[2];
	m[2] = 2.0f * q[2] * q[0] - 2.0f * q[3] * q[1];
	m[4] = 2.0f * q[0] * q[1] - 2.0f * q[3] * q[2];
	m[5] = sy;
	m[6] = 2.0f * q[1] * q[2] + 2.0f * q[3] * q[0];
	m[8] = 2.0f * q[2] * q[0] + 2.0f * q[3] * q[1];
	m[9] = 2.0f * q[1] * q[2] - 2.0f * q[3] * q[0];
	m[10] = sz;
}
