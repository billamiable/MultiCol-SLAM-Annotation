/**
* This file is part of MultiCol-SLAM
*
* Copyright (C) 2015-2016 Steffen Urban <urbste at googlemail.com>
* For more information see <https://github.com/urbste/MultiCol-SLAM>
*
* MultiCol-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* MultiCol-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with MultiCol-SLAM . If not, see <http://www.gnu.org/licenses/>.
*/

#include "g2o_MultiCol_vertices_edges.h"
#include "cConverter.h"

namespace MultiColSLAM
{
// super-important to understand the difference
// edge, that projects a point to a multi camera system
// vertex 0 : t -> Mt, which is the MCS pose
// vertex 1 : i -> 3D point
// vertex 2 : c -> Mc, transformation from MCS to camera
// vertex 3 : c -> interior orientation
void EdgeProjectXYZ2MCS::computeError()
{

	// get all vertices
	// we need 4:
	// 1. the cameras interior orientation that maps our point (camera params for forward/backward projection)
	// 2. the world point
	// 3. the relative orientation of the camera in the MCS frame
	// 4. the absolute orientation of the MCS (pose for body frame)
	const VertexMt_cayley* Mt = static_cast<const VertexMt_cayley*>(_vertices[0]);
	const VertexPointXYZ* pt3 = static_cast<const VertexPointXYZ*>(_vertices[1]);
	const VertexMc_cayley* Mc = static_cast<const VertexMc_cayley*>(_vertices[2]);
	const VertexOmniCameraParameters* camera = static_cast<const VertexOmniCameraParameters*>(_vertices[3]);

	// pose of camera in worldframe
	// cayley2hom - 6x1 minimal homogeneous transformation vector to homogeneous 4x4 transformation matrix
	// Mc is not the absoulte pose for camera, it's the relative transform between each camera and body frame
	cv::Matx44d Mct = cayley2hom(Mt->estimate())*cayley2hom(Mc->estimate());
	cv::Matx44d cMct_inv = cConverter::invMat(Mct);

	// calculate the rotated point in the MCS frame
	cv::Vec4d pt3_rot = cMct_inv * cv::Vec4d(pt3->estimate()(0), pt3->estimate()(1), pt3->estimate()(2), 1.0);
	// project the point to the image
	//cCamModelGeneral_ camModelTemp;
	//camModelTemp.fromVector(camera->estimate());
	double u = 0.0; double v = 0.0;
	// already in the camera coordinate, just camera model projection
	camera->camModel.WorldToImg(pt3_rot(0), pt3_rot(1), pt3_rot(2), u, v);
	// the final form is straightforward, still the form of reprojection error
	_error(0) = _measurement(0) - u;
	_error(1) = _measurement(1) - v;
}

// seems to be not that complicated, but it's clear that camera model is involved!
// TODO see if we can eliminate camera model (even extrinsic matrix) in the formulation
void EdgeProjectXYZ2MCS::linearizeOplus()
{
	const VertexMt_cayley* Mt = static_cast<const VertexMt_cayley*>(_vertices[0]);
	const VertexPointXYZ* pt3 = static_cast<const VertexPointXYZ*>(_vertices[1]);
	const VertexMc_cayley* Mc = static_cast<const VertexMc_cayley*>(_vertices[2]);
	const VertexOmniCameraParameters* camera = static_cast<const VertexOmniCameraParameters*>(_vertices[3]);
	
	//mcsJacs(const cv::Point3_<T>& pt3, const cv::Matx<T, 6, 1>& M_t,
	//	const cv::Matx<T, 6, 1>& M_c, cCamModelGeneral<T>& camModel, cv::Matx<T, 2, 32>& jacs)
	cv::Matx<double, 2, 32> jacs;

	mcsJacs1(pt3->estimate(), 
		Mt->estimate(), 
		Mc->estimate(),
		camera->estimate(), jacs);
	//cerr << camera->estimate() << endl;
	_jacobianOplus[0].resize(2, 6);
	_jacobianOplus[1].resize(2, 3);
	_jacobianOplus[2].resize(2, 6);
	_jacobianOplus[3].resize(2, 12+5);
	

	//cerr << jacs << endl;
	// fill jacs for points
	if (!pt3->fixed())
	{
		for (int i = 0; i < 3; ++i) 
		{
			_jacobianOplus[1](0, i) = -jacs(0, i);
			_jacobianOplus[1](1, i) = -jacs(1, i);
		}
	}

	// fill jacs for Mc
	if (!Mc->fixed())
	{
		for (int i = 0; i < 6; ++i)
		{
			_jacobianOplus[2](0, i) = -jacs(0, i + 26);
			_jacobianOplus[2](1, i) = -jacs(1, i + 26);
		}
	}
	if (!Mt->fixed())
	{
		
		// fill jacs for Mt
		for (int i = 0; i < 6; ++i) 
		{
			_jacobianOplus[0](0, i) = -jacs(0, i + 3);
			_jacobianOplus[0](1, i) = -jacs(1, i + 3);
		}
	}
	if (!camera->fixed())
	{
		// fill jacs for Mt
		for (int i = 0; i < 12 + 5; ++i)
		{
			_jacobianOplus[3](0, i) = -jacs(0, i + 9);
			_jacobianOplus[3](1, i) = -jacs(1, i + 9);
		}
	}
		
}

// TODO OMG! interesting, all the defined part is here!
//      BUT all BA optimizations use the same edge!
//      Maybe we cannot use this? but maybe can..
void mcsJacs1(const cv::Vec3d& pt3,
	const cv::Matx61d& M_t,
	const cv::Matx61d& M_c,
	const Eigen::Matrix<double, 12 + 5, 1>& camModelData,
	cv::Matx<double, 2, 32>& jacs)
{
	// output
	jacs = cv::Matx<double, 2, 32>::zeros();
	// 3D point
	const double X = pt3(0);
	const double Y = pt3(1);
	const double Z = pt3(2);

	// exterior orientation of the mcs    
	const double r1 = M_t(0, 0);
	const double r2 = M_t(1, 0);
	const double r3 = M_t(2, 0);
	const double t1 = M_t(3, 0);
	const double t2 = M_t(4, 0);
	const double t3 = M_t(5, 0);

	// relative orientation in the mcs frame
	const double r_rel1 = M_c(0, 0);
	const double r_rel2 = M_c(1, 0);
	const double r_rel3 = M_c(2, 0);
	const double t_rel1 = M_c(3, 0);
	const double t_rel2 = M_c(4, 0);
	const double t_rel3 = M_c(5, 0);

	// TODO major parts of the following lines are related to camera model only
	// interior orientation
	const double c = camModelData(0, 0);
	const double d = camModelData(1, 0);
	const double e = camModelData(2, 0);

	// interior orientation inverse poly  
	const double a1 = camModelData(16, 0);
	const double a2 = camModelData(15, 0);
	const double a3 = camModelData(14, 0);
	const double a4 = camModelData(13, 0);
	const double a5 = camModelData(12, 0);
	const double a6 = camModelData(11, 0);
	const double a7 = camModelData(10, 0);
	const double a8 = camModelData(9, 0);
	const double a9 = camModelData(8, 0);
	const double a10 = camModelData(7, 0);
	const double a11 = camModelData(6, 0);
	const double a12 = camModelData(5, 0);
	//cerr << camModelData << endl;
	const double  t5 = r_rel1*r_rel1;
	const double  t6 = r_rel2*r_rel2;
	const double  t7 = r_rel3*r_rel3;
	const double  t8 = r1*r1;
	const double  t9 = r2*r2;
	const double  t10 = r3*r3;
	const double  t11 = t5 + t6 + t7 + 1.0;
	const double  t12 = 1.0 / t11;
	const double  t13 = t8 + t9 + t10 + 1.0;
	const double  t14 = 1.0 / t13;
	const double  t19 = r_rel1*r_rel3;
	const double  t15 = r_rel2 - t19;
	const double  t16 = t5 - t6 - t7 + 1.0;
	const double  t17 = r_rel1*r_rel2;
	const double  t18 = r_rel3 + t17;
	const double  t20 = r1*r2;
	const double  t22 = t8 - t9 + t10 - 1.0;
	const double  t23 = r3 + t20;
	const double  t24 = r2*r3;
	const double  t25 = r1 + t24;
	const double  t29 = t8 + t9 - t10 - 1.0;
	const double  t30 = r1*r3;
	const double  t32 = r3 - t20;
	const double  t33 = t8 - t9 - t10 + 1.0;
	const double t34 = r2 + t30;
	const double  t35 = r2*t3*2.0;
	const double  t36 = r3*t2*2.0;
	const double  t37 = t1*t8;
	const double  t38 = t1*t9;
	const double  t39 = t1*t10;
	const double  t40 = r1*r2*t2*2.0;
	const double  t41 = r1*r3*t3*2.0;
	const double  t42 = t1 - t35 + t36 + t37 - t38 - t39 + t40 + t41;
	const double  t43 = r1*t2*2.0;
	const double  t44 = r2*t1*2.0;
	const double  t45 = t3*t8;
	const double  t46 = t3*t9;
	const double  t47 = t3*t10;
	const double  t48 = r1*r3*t1*2.0;
	const double  t49 = r2*r3*t2*2.0;
	const double  t50 = t3 - t43 + t44 - t45 - t46 + t47 + t48 + t49;
	const double  t51 = r1*t3*2.0;
	const double  t52 = r3*t1*2.0;
	const double  t53 = t2*t8;
	const double  t54 = t2*t9;
	const double  t55 = t2*t10;
	const double  t56 = r1*r2*t1*2.0;
	const double  t57 = r2*r3*t3*2.0;
	const double  t58 = t2 + t51 - t52 - t53 + t54 - t55 + t56 + t57;
	const double  t64 = t12*t14*t16*t33;
	const double  t65 = t12*t14*t15*t34*4.0;
	const double  t66 = t12*t14*t18*t32*4.0;
	const double  t67 = -t64 + t65 + t66;
	const double  t68 = r_rel2*t_rel3*2.0;
	const double  t69 = r_rel3*t_rel2*2.0;
	const double  t70 = t5*t_rel1;
	const double  t71 = t6*t_rel1;
	const double  t72 = t7*t_rel1;
	const double  t73 = r_rel1*r_rel2*t_rel2*2.0;
	const double  t74 = r_rel1*r_rel3*t_rel3*2.0;
	const double  t75 = -t68 + t69 + t70 - t71 - t72 + t73 + t74 + t_rel1;
	const double  t76 = t12*t75;
	const double  t77 = t12*t14*t18*t22*2.0;
	const double  t78 = t12*t14*t16*t23*2.0;
	const double  t80 = t12*t14*t15*t29*2.0;
	const double  t82 = t12*t14*t18*t25*4.0;
	const double  t83 = X*t67;
	const double  t84 = t12*t14*t16*t42;
	const double  t85 = t12*t14*t18*t58*2.0;
	const double  t86 = t12*t14*t15*t50*2.0;
	const double  t21 = t76 + t83 + t84 + t85 - t86 - Y*(-t77 + t78 + t12*t14*t15*(r1 - r2*r3)*4.0) - Z*(t80 + t82 - t12*t14*t16*(r2 - r1*r3)*2.0);
	const double  t26 = t5 - t6 + t7 - 1.0;
	const double t27 = r_rel2*r_rel3;
	const double t28 = r_rel1 + t27;
	const double t31 = r_rel3 - t17;
	const double t60 = t12*t14*t26*t32*2.0;
	const double t61 = t12*t14*t31*t33*2.0;
	const double t62 = t12*t14*t28*t34*4.0;
	const double t63 = t60 - t61 + t62;
	const double t79 = r1 - t24;
	const double t81 = r2 - t30;
	const double t88 = t12*t14*t22*t26;
	const double t89 = t12*t14*t23*t31*4.0;
	const double t90 = t12*t14*t28*t79*4.0;
	const double t91 = -t88 + t89 + t90;
	const double t92 = Y*t91;
	const double t93 = t12*t14*t25*t26*2.0;
	const double t94 = t12*t14*t28*t29*2.0;
	const double t95 = t12*t14*t31*t81*4.0;
	const double t96 = t93 + t94 - t95;
	const double t97 = Z*t96;
	const double t98 = r_rel1*t_rel3*2.0;
	const double t99 = r_rel3*t_rel1*2.0;
	const double t100 = t5*t_rel2;
	const double t101 = t6*t_rel2;
	const double t102 = t7*t_rel2;
	const double t103 = r_rel1*r_rel2*t_rel1*2.0;
	const double t104 = r_rel2*r_rel3*t_rel3*2.0;
	const double t105 = t98 - t99 - t100 + t101 - t102 + t103 + t104 + t_rel2;
	const double t106 = t12*t105;
	const double t107 = X*t63;
	const double t108 = t12*t14*t31*t42*2.0;
	const double t109 = t12*t14*t28*t50*2.0;
	const double t110 = t12*t14*t26*t58;
	const double t59 = t92 + t97 + t106 - t107 - t108 + t109 - t110;
	const double t119 = t12*t14*t15*t79*4.0;
	const double t120 = -t77 + t78 + t119;
	const double t121 = Y*t120;
	const double t122 = t12*t14*t16*t81*2.0;
	const double t123 = t80 + t82 - t122;
	const double t124 = Z*t123;
	const double t87 = t76 + t83 + t84 + t85 - t86 - t121 - t124;
	const double t111 = t59*t59;
	const double t112 = t5 + t6 - t7 - 1.0;
	const double t113 = r_rel1 - t27;
	const double t114 = r_rel2 + t19;
	const double t125 = t87*t87;
	const double t126 = t111 + t125;
	const double t127 = 1.0 / sqrt(t126);
	const double t128 = t12*t14*t79*t112*2.0;
	const double t129 = t12*t14*t22*t113*2.0;
	const double t130 = t12*t14*t23*t114*4.0;
	const double t131 = t128 + t129 + t130;
	const double t132 = Y*t131;
	const double t133 = t12*t14*t29*t112;
	const double t134 = t12*t14*t25*t113*4.0;
	const double t135 = t12*t14*t81*t114*4.0;
	const double t136 = -t133 + t134 + t135;
	const double t137 = Z*t136;
	const double t138 = r_rel1*t_rel2*2.0;
	const double t139 = r_rel2*t_rel1*2.0;
	const double t140 = t5*t_rel3;
	const double t141 = t6*t_rel3;
	const double t142 = t7*t_rel3;
	const double t143 = r_rel1*r_rel3*t_rel1*2.0;
	const double t144 = r_rel2*r_rel3*t_rel2*2.0;
	const double t145 = -t138 + t139 - t140 - t141 + t142 + t143 + t144 + t_rel3;
	const double t146 = t12*t145;
	const double t147 = t12*t14*t34*t112*2.0;
	const double t148 = t12*t14*t33*t114*2.0;
	const double t149 = t12*t14*t32*t113*4.0;
	const double t150 = -t147 + t148 + t149;
	const double t151 = X*t150;
	const double t152 = t12*t14*t42*t114*2.0;
	const double t153 = t12*t14*t58*t113*2.0;
	const double t154 = t12*t14*t50*t112;
	const double t155 = t132 - t137 - t146 + t151 - t152 + t153 + t154;
	const double t156 = t127*t155;
	const double t115 = atan(t156);
	const double t116 = t115*t115;
	const double t117 = t116*t116;
	const double t118 = t117*t117;
	const double  t157 = 1.0 / t126;
	const double  t158 = t155*t155;
	const double  t159 = t157*t158;
	const double  t160 = t159 + 1.0;
	const double  t161 = 1.0 / t160;
	const double  t162 = t127*t150;
	const double  t163 = 1.0 / pow(t126, 3.0 / 2.0);
	const double  t164 = t59*t63*2.0;
	const double  t167 = t67*t87*2.0;
	const double  t177 = t164 - t167;
	const double  t165 = t155*t163*t177*(1.0 / 2.0);
	const double  t166 = t162 + t165;
	const double  t168 = d*t59;
	const double  t169 = c*t87;
	const double  t170 = t168 + t169;
	const double  t171 = a2*t116*t118;
	const double  t172 = a4*t118;
	const double  t173 = a6*t116*t117;
	const double  t174 = a8*t117;
	const double  t175 = a10*t116;
	const double  t178 = a1*t115*t116*t118;
	const double  t179 = a3*t115*t118;
	const double  t180 = a5*t115*t116*t117;
	const double  t181 = a7*t115*t117;
	const double  t182 = a9*t115*t116;
	const double  t183 = a11*t115;
	const double  t176 = a12 + t171 + t172 + t173 + t174 + t175 - t178 - t179 - t180 - t181 - t182 - t183;
	const double  t184 = t127*t131;
	const double t185 = t59*t91*2.0;
	const double t188 = t87*t120*2.0;
	const double t186 = t185 - t188;
	const double t189 = t155*t163*t186*(1.0 / 2.0);
	const double t187 = t184 - t189;
	const double t190 = t127*t136;
	const double t191 = t59*t96*2.0;
	const double t194 = t87*t123*2.0;
	const double t192 = t155*t163*(t191 - t194)*(1.0 / 2.0);
	const double t193 = t190 + t192;
	const double t195 = 1.0 / (t13*t13);
	const double t196 = r1*t1*2.0;
	const double t197 = r2*t2*2.0;
	const double t198 = r3*t3*2.0;
	const double t199 = t196 + t197 + t198;
	const double t200 = t2*2.0;
	const double t201 = t51 - t52 + t200;
	const double t202 = t3*2.0;
	const double t203 = -t43 + t44 + t202;
	const double t204 = r2*t12*t14*t18*4.0;
	const double t205 = r1*t12*t14*t16*2.0;
	const double t206 = r1*t12*t15*t34*t195*8.0;
	const double t207 = r1*t12*t18*t32*t195*8.0;
	const double t265 = r3*t12*t14*t15*4.0;
	const double t266 = r1*t12*t16*t33*t195*2.0;
	const double t208 = t204 + t205 + t206 + t207 - t265 - t266;
	const double t209 = X*t208;
	const double t210 = t12*t14*t15*4.0;
	const double t211 = r2*t12*t14*t16*2.0;
	const double t212 = r1*t12*t18*t22*t195*4.0;
	const double t267 = r1*t12*t14*t18*4.0;
	const double t268 = r1*t12*t15*t79*t195*8.0;
	const double t269 = r1*t12*t16*t23*t195*4.0;
	const double t213 = t210 + t211 + t212 - t267 - t268 - t269;
	const double t214 = Y*t213;
	const double t215 = t12*t14*t18*4.0;
	const double t216 = r1*t12*t14*t15*4.0;
	const double t217 = r3*t12*t14*t16*2.0;
	const double t218 = r1*t12*t16*t81*t195*4.0;
	const double t270 = r1*t12*t18*t25*t195*8.0;
	const double t271 = r1*t12*t15*t29*t195*4.0;
	const double t219 = t215 + t216 + t217 + t218 - t270 - t271;
	const double t220 = Z*t219;
	const double t221 = r1*t12*t18*t58*t195*4.0;
	const double t222 = r1*t12*t16*t42*t195*2.0;
	const double t272 = t12*t14*t16*t199;
	const double t273 = t12*t14*t18*t203*2.0;
	const double t274 = t12*t14*t15*t201*2.0;
	const double t275 = r1*t12*t15*t50*t195*4.0;
	const double t223 = t209 + t214 + t220 + t221 + t222 - t272 - t273 - t274 - t275;
	const double t224 = t12*t14*t28*4.0;
	const double t225 = r2*t12*t14*t31*4.0;
	const double t226 = r1*t12*t22*t26*t195*2.0;
	const double t276 = r1*t12*t14*t26*2.0;
	const double t277 = r1*t12*t23*t31*t195*8.0;
	const double t278 = r1*t12*t28*t79*t195*8.0;
	const double t227 = t224 + t225 + t226 - t276 - t277 - t278;
	const double t228 = Y*t227;
	const double t229 = t12*t14*t26*2.0;
	const double t230 = r1*t12*t14*t28*4.0;
	const double t231 = r3*t12*t14*t31*4.0;
	const double t232 = r1*t12*t31*t81*t195*8.0;
	const double t279 = r1*t12*t25*t26*t195*4.0;
	const double t280 = r1*t12*t28*t29*t195*4.0;
	const double t233 = t229 + t230 + t231 + t232 - t279 - t280;
	const double t234 = Z*t233;
	const double  t235 = r1*t12*t14*t31*4.0;
	const double  t236 = r2*t12*t14*t26*2.0;
	const double  t237 = r1*t12*t28*t34*t195*8.0;
	const double  t238 = r1*t12*t26*t32*t195*4.0;
	const double  t281 = r3*t12*t14*t28*4.0;
	const double  t282 = r1*t12*t31*t33*t195*4.0;
	const double  t239 = t235 + t236 + t237 + t238 - t281 - t282;
	const double  t240 = X*t239;
	const double  t241 = r1*t12*t31*t42*t195*4.0;
	const double  t242 = r1*t12*t26*t58*t195*2.0;
	const double  t283 = t12*t14*t31*t199*2.0;
	const double  t284 = t12*t14*t28*t201*2.0;
	const double  t285 = t12*t14*t26*t203;
	const double  t286 = r1*t12*t28*t50*t195*4.0;
	const double  t243 = t228 + t234 + t240 + t241 + t242 - t283 - t284 - t285 - t286;
	const double  t244 = t12*t14*t112*2.0;
	const double  t245 = r1*t12*t14*t113*4.0;
	const double  t246 = r2*t12*t14*t114*4.0;
	const double  t290 = r1*t12*t23*t114*t195*8.0;
	const double  t291 = r1*t12*t79*t112*t195*4.0;
	const double  t292 = r1*t12*t22*t113*t195*4.0;
	const double  t247 = t244 + t245 + t246 - t290 - t291 - t292;
	const double  t248 = r3*t12*t14*t114*4.0;
	const double  t249 = r1*t12*t14*t112*2.0;
	const double  t250 = r1*t12*t25*t113*t195*8.0;
	const double  t251 = r1*t12*t81*t114*t195*8.0;
	const double  t294 = t12*t14*t113*4.0;
	const double  t295 = r1*t12*t29*t112*t195*2.0;
	const double  t252 = t248 + t249 + t250 + t251 - t294 - t295;
	const double  t253 = r2*t12*t14*t113*4.0;
	const double  t254 = r3*t12*t14*t112*2.0;
	const double  t255 = r1*t12*t32*t113*t195*8.0;
	const double  t256 = r1*t12*t33*t114*t195*4.0;
	const double  t297 = r1*t12*t14*t114*4.0;
	const double  t298 = r1*t12*t34*t112*t195*4.0;
	const double  t257 = t253 + t254 + t255 + t256 - t297 - t298;
	const double  t258 = X*t257;
	const double  t259 = t12*t14*t114*t199*2.0;
	const double  t260 = t12*t14*t112*t201;
	const double  t261 = r1*t12*t58*t113*t195*4.0;
	const double  t262 = r1*t12*t50*t112*t195*2.0;
	const double  t293 = Y*t247;
	const double  t296 = Z*t252;
	const double  t299 = t12*t14*t113*t203*2.0;
	const double  t300 = r1*t12*t42*t114*t195*4.0;
	const double  t263 = t258 + t259 + t260 + t261 + t262 - t293 - t296 - t299 - t300;
	const double  t264 = t127*t263;
	const double  t287 = t59*t243*2.0;
	const double  t301 = t87*t223*2.0;
	const double  t288 = t155*t163*(t287 - t301)*(1.0 / 2.0);
	const double  t289 = t264 + t288;
	const double  t302 = t1*2.0;
	const double  t303 = -t35 + t36 + t302;
	const double  t304 = r2*t12*t16*t33*t195*2.0;
	const double  t350 = r2*t12*t15*t34*t195*8.0;
	const double  t351 = r2*t12*t18*t32*t195*8.0;
	const double  t305 = t210 + t211 - t267 + t304 - t350 - t351;
	const double  t306 = r2*t12*t18*t22*t195*4.0;
	const double  t353 = r2*t12*t15*t79*t195*8.0;
	const double  t354 = r2*t12*t16*t23*t195*4.0;
	const double  t307 = t204 + t205 - t265 + t306 - t353 - t354;
	const double  t308 = Y*t307;
	const double  t309 = t12*t14*t16*2.0;
	const double  t310 = r2*t12*t18*t25*t195*8.0;
	const double  t311 = r2*t12*t15*t29*t195*4.0;
	const double  t355 = r2*t12*t14*t15*4.0;
	const double  t356 = r3*t12*t14*t18*4.0;
	const double  t357 = r2*t12*t16*t81*t195*4.0;
	const double  t312 = t309 + t310 + t311 - t355 - t356 - t357;
	const double  t313 = t12*t14*t16*t203;
	const double  t314 = t12*t14*t15*t303*2.0;
	const double  t315 = r2*t12*t18*t58*t195*4.0;
	const double  t316 = r2*t12*t16*t42*t195*2.0;
	const double  t352 = X*t305;
	const double  t358 = Z*t312;
	const double  t359 = t12*t14*t18*t199*2.0;
	const double  t360 = r2*t12*t15*t50*t195*4.0;
	const double  t317 = t308 + t313 + t314 + t315 + t316 - t352 - t358 - t359 - t360;
	const double t318 = t12*t14*t31*4.0;
	const double t319 = r2*t12*t25*t26*t195*4.0;
	const double t320 = r2*t12*t28*t29*t195*4.0;
	const double t361 = r2*t12*t14*t28*4.0;
	const double t362 = r3*t12*t14*t26*2.0;
	const double t363 = r2*t12*t31*t81*t195*8.0;
	const double t321 = t318 + t319 + t320 - t361 - t362 - t363;
	const double t322 = r2*t12*t31*t33*t195*4.0;
	const double t365 = r2*t12*t28*t34*t195*8.0;
	const double t366 = r2*t12*t26*t32*t195*4.0;
	const double t323 = t224 + t225 - t276 + t322 - t365 - t366;
	const double t324 = r2*t12*t22*t26*t195*2.0;
	const double t368 = r2*t12*t23*t31*t195*8.0;
	const double t369 = r2*t12*t28*t79*t195*8.0;
	const double t325 = t235 + t236 - t281 + t324 - t368 - t369;
	const double t326 = Y*t325;
	const double t327 = t12*t14*t31*t203*2.0;
	const double t328 = t12*t14*t28*t303*2.0;
	const double t329 = r2*t12*t31*t42*t195*4.0;
	const double t330 = r2*t12*t26*t58*t195*2.0;
	const double t364 = Z*t321;
	const double t367 = X*t323;
	const double t370 = t12*t14*t26*t199;
	const double t371 = r2*t12*t28*t50*t195*4.0;
	const double t331 = t326 + t327 + t328 + t329 + t330 - t364 - t367 - t370 - t371;
	const double t332 = t12*t14*t114*4.0;
	const double t333 = r3*t12*t14*t113*4.0;
	const double t334 = r2*t12*t29*t112*t195*2.0;
	const double t375 = r2*t12*t14*t112*2.0;
	const double t376 = r2*t12*t25*t113*t195*8.0;
	const double t377 = r2*t12*t81*t114*t195*8.0;
	const double t335 = t332 + t333 + t334 - t375 - t376 - t377;
	const double t336 = Z*t335;
	const double t337 = r2*t12*t32*t113*t195*8.0;
	const double t338 = r2*t12*t33*t114*t195*4.0;
	const double t378 = r2*t12*t34*t112*t195*4.0;
	const double t339 = t244 + t245 + t246 + t337 + t338 - t378;
	const double t340 = X*t339;
	const double t341 = r2*t12*t23*t114*t195*8.0;
	const double t342 = r2*t12*t79*t112*t195*4.0;
	const double t343 = r2*t12*t22*t113*t195*4.0;
	const double t344 = t253 + t254 - t297 + t341 + t342 + t343;
	const double t345 = Y*t344;
	const double t346 = r2*t12*t58*t113*t195*4.0;
	const double t347 = r2*t12*t50*t112*t195*2.0;
	const double t379 = t12*t14*t113*t199*2.0;
	const double t380 = t12*t14*t114*t203*2.0;
	const double t381 = t12*t14*t112*t303;
	const double t382 = r2*t12*t42*t114*t195*4.0;
	const double t348 = t336 + t340 + t345 + t346 + t347 - t379 - t380 - t381 - t382;
	const double t349 = t127*t348;
	const double t372 = t59*t331*2.0;
	const double t383 = t87*t317*2.0;
	const double t373 = t155*t163*(t372 - t383)*(1.0 / 2.0);
	const double t374 = t349 + t373;
	const double t384 = r3*t12*t16*t33*t195*2.0;
	const double t427 = r3*t12*t15*t34*t195*8.0;
	const double t428 = r3*t12*t18*t32*t195*8.0;
	const double t385 = t215 + t216 + t217 + t384 - t427 - t428;
	const double t386 = r3*t12*t16*t81*t195*4.0;
	const double t430 = r3*t12*t18*t25*t195*8.0;
	const double t431 = r3*t12*t15*t29*t195*4.0;
	const double t387 = t204 + t205 - t265 + t386 - t430 - t431;
	const double  t388 = Z*t387;
	const double  t389 = r3*t12*t15*t79*t195*8.0;
	const double  t390 = r3*t12*t16*t23*t195*4.0;
	const double  t432 = r3*t12*t18*t22*t195*4.0;
	const double  t391 = -t309 + t355 + t356 + t389 + t390 - t432;
	const double  t392 = t12*t14*t15*t199*2.0;
	const double  t393 = t12*t14*t18*t303*2.0;
	const double  t394 = r3*t12*t18*t58*t195*4.0;
	const double  t395 = r3*t12*t16*t42*t195*2.0;
	const double t429 = X*t385;
	const double t433 = Y*t391;
	const double t434 = t12*t14*t16*t201;
	const double t435 = r3*t12*t15*t50*t195*4.0;
	const double t396 = t388 + t392 + t393 + t394 + t395 - t429 - t433 - t434 - t435;
	const double t397 = r3*t12*t23*t31*t195*8.0;
	const double t398 = r3*t12*t28*t79*t195*8.0;
	const double t436 = r3*t12*t22*t26*t195*2.0;
	const double t399 = -t318 + t361 + t362 + t397 + t398 - t436;
	const double t400 = r3*t12*t31*t33*t195*4.0;
	const double t438 = r3*t12*t28*t34*t195*8.0;
	const double t439 = r3*t12*t26*t32*t195*4.0;
	const double t401 = t229 + t230 + t231 + t400 - t438 - t439;
	const double t402 = r3*t12*t31*t81*t195*8.0;
	const double t441 = r3*t12*t25*t26*t195*4.0;
	const double t442 = r3*t12*t28*t29*t195*4.0;
	const double t403 = t235 + t236 - t281 + t402 - t441 - t442;
	const double t404 = Z*t403;
	const double t405 = t12*t14*t28*t199*2.0;
	const double t406 = t12*t14*t26*t303;
	const double t407 = r3*t12*t31*t42*t195*4.0;
	const double t408 = r3*t12*t26*t58*t195*2.0;
	const double t437 = Y*t399;
	const double t440 = X*t401;
	const double t443 = t12*t14*t31*t201*2.0;
	const double t444 = r3*t12*t28*t50*t195*4.0;
	const double t409 = t404 + t405 + t406 + t407 + t408 - t437 - t440 - t443 - t444;
	const double t410 = r3*t12*t23*t114*t195*8.0;
	const double t411 = r3*t12*t79*t112*t195*4.0;
	const double t412 = r3*t12*t22*t113*t195*4.0;
	const double t413 = -t332 - t333 + t375 + t410 + t411 + t412;
	const double t414 = Y*t413;
	const double t415 = r3*t12*t32*t113*t195*8.0;
	const double t416 = r3*t12*t33*t114*t195*4.0;
	const double t448 = r3*t12*t34*t112*t195*4.0;
	const double t417 = t248 + t249 - t294 + t415 + t416 - t448;
	const double t418 = X*t417;
	const double t419 = r3*t12*t29*t112*t195*2.0;
	const double t449 = r3*t12*t25*t113*t195*8.0;
	const double t450 = r3*t12*t81*t114*t195*8.0;
	const double t420 = Z*(t253 + t254 - t297 + t419 - t449 - t450);
	const double t421 = t12*t14*t114*t201*2.0;
	const double t422 = t12*t14*t113*t303*2.0;
	const double t423 = r3*t12*t58*t113*t195*4.0;
	const double t424 = r3*t12*t50*t112*t195*2.0;
	const double t451 = t12*t14*t112*t199;
	const double t452 = r3*t12*t42*t114*t195*4.0;
	const double t425 = t414 + t418 + t420 + t421 + t422 + t423 + t424 - t451 - t452;
	const double t426 = t127*t425;
	const double t445 = t59*t409*2.0;
	const double t453 = t87*t396*2.0;
	const double t446 = t155*t163*(t445 - t453)*(1.0 / 2.0);
	const double t447 = t426 + t446;
	const double t454 = r3*2.0;
	const double t459 = r1*r2*2.0;
	const double t455 = t454 - t459;
	const double t456 = r2*2.0;
	const double t457 = r1*r3*2.0;
	const double t458 = t456 + t457;
	const double t460 = t12*t14*t18*t455*2.0;
	const double t461 = t12*t14*t15*t458*2.0;
	const double t462 = -t64 + t460 + t461;
	const double t463 = t12*t14*t28*t458*2.0;
	const double t464 = t12*t14*t26*t455;
	const double t465 = -t61 + t463 + t464;
	const double t466 = t12*t14*t113*t455*2.0;
	const double t472 = t12*t14*t112*t458;
	const double t467 = t148 + t466 - t472;
	const double t468 = t127*t467;
	const double t469 = t59*t465*2.0;
	const double t473 = t87*t462*2.0;
	const double t470 = t155*t163*(t469 - t473)*(1.0 / 2.0);
	const double t471 = t468 + t470;
	const double t474 = r1*2.0;
	const double t477 = r2*r3*2.0;
	const double t475 = t474 - t477;
	const double t476 = t454 + t459;
	const double t478 = t12*t14*t114*t476*2.0;
	const double t479 = t12*t14*t112*t475;
	const double t480 = t129 + t478 + t479;
	const double t481 = t127*t480;
	const double t482 = t12*t14*t28*t475*2.0;
	const double t483 = t12*t14*t31*t476*2.0;
	const double t484 = -t88 + t482 + t483;
	const double t485 = t59*t484*2.0;
	const double t486 = t12*t14*t15*t475*2.0;
	const double t487 = t12*t14*t16*t476;
	const double t488 = -t77 + t486 + t487;
	const double t491 = t87*t488*2.0;
	const double t489 = t485 - t491;
	const double t492 = t155*t163*t489*(1.0 / 2.0);
	const double t490 = t481 - t492;
	const double t493 = t456 - t457;
	const double t494 = t474 + t477;
	const double t495 = t12*t14*t114*t493*2.0;
	const double t496 = t12*t14*t113*t494*2.0;
	const double t497 = -t133 + t495 + t496;
	const double t498 = t127*t497;
	const double t499 = t12*t14*t26*t494;
	const double t506 = t12*t14*t31*t493*2.0;
	const double t500 = t94 + t499 - t506;
	const double t501 = t12*t14*t18*t494*2.0;
	const double t508 = t12*t14*t16*t493;
	const double t502 = t80 + t501 - t508;
	const double t507 = t59*t500*2.0;
	const double t509 = t87*t502*2.0;
	const double t503 = t507 - t509;
	const double t504 = t155*t163*t503*(1.0 / 2.0);
	const double t505 = t498 + t504;
	const double t510 = 1.0 / (t11*t11);
	const double t511 = t12*t14*t22*2.0;
	const double t512 = r_rel1*t12*t14*t79*4.0;
	const double t513 = r_rel3*t12*t14*t23*4.0;
	const double t589 = r_rel1*t14*t23*t114*t510*8.0;
	const double t590 = r_rel1*t14*t79*t112*t510*4.0;
	const double t591 = r_rel1*t14*t22*t113*t510*4.0;
	const double t514 = t511 + t512 + t513 - t589 - t590 - t591;
	const double t515 = Y*t514;
	const double t516 = t12*t14*t25*4.0;
	const double t517 = r_rel3*t12*t14*t81*4.0;
	const double t518 = r_rel1*t14*t29*t112*t510*2.0;
	const double t592 = r_rel1*t12*t14*t29*2.0;
	const double t593 = r_rel1*t14*t25*t113*t510*8.0;
	const double t594 = r_rel1*t14*t81*t114*t510*8.0;
	const double t519 = t516 + t517 + t518 - t592 - t593 - t594;
	const double t520 = t_rel2*2.0;
	const double t521 = t98 - t99 + t520;
	const double t522 = t12*t521;
	const double t523 = t12*t14*t32*4.0;
	const double t524 = r_rel3*t12*t14*t33*2.0;
	const double t525 = r_rel1*t14*t34*t112*t510*4.0;
	const double t596 = r_rel1*t12*t14*t34*4.0;
	const double t597 = r_rel1*t14*t32*t113*t510*8.0;
	const double t598 = r_rel1*t14*t33*t114*t510*4.0;
	const double t526 = t523 + t524 + t525 - t596 - t597 - t598;
	const double t527 = X*t526;
	const double t528 = t12*t14*t58*2.0;
	const double t529 = r_rel1*t145*t510*2.0;
	const double t530 = r_rel1*t12*t14*t50*2.0;
	const double t531 = r_rel1*t14*t42*t114*t510*4.0;
	const double t595 = Z*t519;
	const double t599 = r_rel3*t12*t14*t42*2.0;
	const double t600 = r_rel1*t14*t58*t113*t510*4.0;
	const double t601 = r_rel1*t14*t50*t112*t510*2.0;
	const double t532 = t515 + t522 + t527 + t528 + t529 + t530 + t531 - t595 - t599 - t600 - t601;
	const double t533 = t127*t532;
	const double t534 = r_rel2*t12*t14*t23*4.0;
	const double t535 = r_rel1*t12*t14*t22*2.0;
	const double t536 = r_rel1*t14*t23*t31*t510*8.0;
	const double t537 = r_rel1*t14*t28*t79*t510*8.0;
	const double t602 = t12*t14*t79*4.0;
	const double t603 = r_rel1*t14*t22*t26*t510*2.0;
	const double t538 = t534 + t535 + t536 + t537 - t602 - t603;
	const double t539 = t12*t14*t29*2.0;
	const double t540 = r_rel1*t12*t14*t25*4.0;
	const double t541 = r_rel2*t12*t14*t81*4.0;
	const double t542 = r_rel1*t14*t31*t81*t510*8.0;
	const double t605 = r_rel1*t14*t25*t26*t510*4.0;
	const double t606 = r_rel1*t14*t28*t29*t510*4.0;
	const double t543 = t539 + t540 + t541 + t542 - t605 - t606;
	const double t544 = Z*t543;
	const double t545 = t_rel3*2.0;
	const double t546 = -t138 + t139 + t545;
	const double t547 = t12*t546;
	const double t548 = t12*t14*t34*4.0;
	const double t549 = r_rel1*t12*t14*t32*4.0;
	const double t550 = r_rel2*t12*t14*t33*2.0;
	const double t551 = r_rel1*t14*t31*t33*t510*4.0;
	const double t607 = r_rel1*t14*t28*t34*t510*8.0;
	const double t608 = r_rel1*t14*t26*t32*t510*4.0;
	const double t552 = t548 + t549 + t550 + t551 - t607 - t608;
	const double t553 = t12*t14*t50*2.0;
	const double t554 = r_rel2*t12*t14*t42*2.0;
	const double t555 = r_rel1*t14*t31*t42*t510*4.0;
	const double t556 = r_rel1*t14*t26*t58*t510*2.0;
	const double t604 = Y*t538;
	const double t609 = X*t552;
	const double t610 = r_rel1*t105*t510*2.0;
	const double t611 = r_rel1*t12*t14*t58*2.0;
	const double t612 = r_rel1*t14*t28*t50*t510*4.0;
	const double t557 = t544 + t547 + t553 + t554 + t555 + t556 - t604 - t609 - t610 - t611 - t612;
	const double t558 = t59*t557*2.0;
	const double t559 = r_rel3*t12*t14*t34*4.0;
	const double t560 = r_rel1*t12*t14*t33*2.0;
	const double t561 = r_rel1*t14*t15*t34*t510*8.0;
	const double t562 = r_rel1*t14*t18*t32*t510*8.0;
	const double t613 = r_rel2*t12*t14*t32*4.0;
	const double t614 = r_rel1*t14*t16*t33*t510*2.0;
	const double t563 = t559 + t560 + t561 + t562 - t613 - t614;
	const double t564 = r_rel1*t_rel1*2.0;
	const double t565 = r_rel2*t_rel2*2.0;
	const double t566 = r_rel3*t_rel3*2.0;
	const double t567 = t564 + t565 + t566;
	const double t568 = t12*t567;
	const double t569 = r_rel3*t12*t14*t79*4.0;
	const double t570 = r_rel2*t12*t14*t22*2.0;
	const double t571 = r_rel1*t14*t15*t79*t510*8.0;
	const double t572 = r_rel1*t14*t16*t23*t510*4.0;
	const double t616 = r_rel1*t12*t14*t23*4.0;
	const double t617 = r_rel1*t14*t18*t22*t510*4.0;
	const double t573 = t569 + t570 + t571 + t572 - t616 - t617;
	const double t574 = Y*t573;
	const double t575 = r_rel1*t12*t14*t81*4.0;
	const double t576 = r_rel3*t12*t14*t29*2.0;
	const double t577 = r_rel1*t14*t18*t25*t510*8.0;
	const double t578 = r_rel1*t14*t15*t29*t510*4.0;
	const double t618 = r_rel2*t12*t14*t25*4.0;
	const double t619 = r_rel1*t14*t16*t81*t510*4.0;
	const double t579 = t575 + t576 + t577 + t578 - t618 - t619;
	const double t580 = Z*t579;
	const double t581 = r_rel1*t12*t14*t42*2.0;
	const double t582 = r_rel2*t12*t14*t58*2.0;
	const double t583 = r_rel3*t12*t14*t50*2.0;
	const double t584 = r_rel1*t14*t15*t50*t510*4.0;
	const double t615 = X*t563;
	const double t620 = r_rel1*t75*t510*2.0;
	const double t621 = r_rel1*t14*t18*t58*t510*4.0;
	const double t622 = r_rel1*t14*t16*t42*t510*2.0;
	const double t585 = t568 + t574 + t580 + t581 + t582 + t583 + t584 - t615 - t620 - t621 - t622;
	const double t586 = t87*t585*2.0;
	const double t587 = t558 + t586;
	const double t623 = t155*t163*t587*(1.0 / 2.0);
	const double t588 = t533 - t623;
	const double t624 = r_rel3*t12*t14*t22*2.0;
	const double t625 = r_rel2*t14*t23*t114*t510*8.0;
	const double t626 = r_rel2*t14*t79*t112*t510*4.0;
	const double t627 = r_rel2*t14*t22*t113*t510*4.0;
	const double t675 = t12*t14*t23*4.0;
	const double t676 = r_rel2*t12*t14*t79*4.0;
	const double t628 = t624 + t625 + t626 + t627 - t675 - t676;
	const double t629 = Y*t628;
	const double t630 = r_rel3*t12*t14*t25*4.0;
	const double t631 = r_rel2*t12*t14*t29*2.0;
	const double t632 = r_rel2*t14*t25*t113*t510*8.0;
	const double t633 = r_rel2*t14*t81*t114*t510*8.0;
	const double t677 = t12*t14*t81*4.0;
	const double t678 = r_rel2*t14*t29*t112*t510*2.0;
	const double t634 = t630 + t631 + t632 + t633 - t677 - t678;
	const double t635 = t_rel1*2.0;
	const double t636 = -t68 + t69 + t635;
	const double t637 = t12*t636;
	const double t638 = r_rel2*t12*t14*t34*4.0;
	const double t639 = r_rel3*t12*t14*t32*4.0;
	const double t640 = r_rel2*t14*t32*t113*t510*8.0;
	const double t641 = r_rel2*t14*t33*t114*t510*4.0;
	const double t680 = t12*t14*t33*2.0;
	const double t681 = r_rel2*t14*t34*t112*t510*4.0;
	const double t642 = t638 + t639 + t640 + t641 - t680 - t681;
	const double t643 = X*t642;
	const double t644 = t12*t14*t42*2.0;
	const double t645 = r_rel3*t12*t14*t58*2.0;
	const double t646 = r_rel2*t14*t58*t113*t510*4.0;
	const double t647 = r_rel2*t14*t50*t112*t510*2.0;
	const double t679 = Z*t634;
	const double t682 = r_rel2*t145*t510*2.0;
	const double t683 = r_rel2*t12*t14*t50*2.0;
	const double t684 = r_rel2*t14*t42*t114*t510*4.0;
	const double t648 = t629 + t637 + t643 + t644 + t645 + t646 + t647 - t679 - t682 - t683 - t684;
	const double t649 = t127*t648;
	const double t650 = r_rel2*t14*t16*t33*t510*2.0;
	const double t685 = r_rel2*t14*t15*t34*t510*8.0;
	const double t686 = r_rel2*t14*t18*t32*t510*8.0;
	const double t651 = t548 + t549 + t550 + t650 - t685 - t686;
	const double t652 = r_rel2*t14*t15*t79*t510*8.0;
	const double t653 = r_rel2*t14*t16*t23*t510*4.0;
	const double t688 = r_rel2*t14*t18*t22*t510*4.0;
	const double t654 = t534 + t535 - t602 + t652 + t653 - t688;
	const double t655 = r_rel2*t14*t16*t81*t510*4.0;
	const double t690 = r_rel2*t14*t18*t25*t510*8.0;
	const double t691 = r_rel2*t14*t15*t29*t510*4.0;
	const double t656 = t539 + t540 + t541 + t655 - t690 - t691;
	const double t657 = Z*t656;
	const double t658 = r_rel2*t75*t510*2.0;
	const double t659 = r_rel2*t14*t18*t58*t510*4.0;
	const double t660 = r_rel2*t14*t16*t42*t510*2.0;
	const double t687 = X*t651;
	const double t689 = Y*t654;
	const double t692 = r_rel2*t14*t15*t50*t510*4.0;
	const double t661 = t547 + t553 + t554 - t611 + t657 + t658 + t659 + t660 - t687 - t689 - t692;
	const double t662 = t87*t661*2.0;
	const double t663 = r_rel2*t14*t31*t33*t510*4.0;
	const double t693 = r_rel2*t14*t28*t34*t510*8.0;
	const double t694 = r_rel2*t14*t26*t32*t510*4.0;
	const double t664 = t559 + t560 - t613 + t663 - t693 - t694;
	const double t665 = r_rel2*t14*t22*t26*t510*2.0;
	const double t696 = r_rel2*t14*t23*t31*t510*8.0;
	const double t697 = r_rel2*t14*t28*t79*t510*8.0;
	const double t666 = Y*(t569 + t570 - t616 + t665 - t696 - t697);
	const double t667 = r_rel2*t14*t31*t81*t510*8.0;
	const double t698 = r_rel2*t14*t25*t26*t510*4.0;
	const double t699 = r_rel2*t14*t28*t29*t510*4.0;
	const double t668 = t575 + t576 - t618 + t667 - t698 - t699;
	const double t669 = Z*t668;
	const double t670 = r_rel2*t14*t31*t42*t510*4.0;
	const double t671 = r_rel2*t14*t26*t58*t510*2.0;
	const double t695 = X*t664;
	const double t700 = r_rel2*t105*t510*2.0;
	const double t701 = r_rel2*t14*t28*t50*t510*4.0;
	const double t672 = t568 + t581 + t582 + t583 + t666 + t669 + t670 + t671 - t695 - t700 - t701;
	const double t702 = t59*t672*2.0;
	const double t673 = t662 - t702;
	const double t703 = t155*t163*t673*(1.0 / 2.0);
	const double t674 = t649 - t703;
	const double t704 = r_rel3*t14*t34*t112*t510*4.0;
	const double t743 = r_rel3*t14*t32*t113*t510*8.0;
	const double t744 = r_rel3*t14*t33*t114*t510*4.0;
	const double t705 = t559 + t560 - t613 + t704 - t743 - t744;
	const double t706 = r_rel3*t14*t23*t114*t510*8.0;
	const double t707 = r_rel3*t14*t79*t112*t510*4.0;
	const double t708 = r_rel3*t14*t22*t113*t510*4.0;
	const double t709 = t569 + t570 - t616 + t706 + t707 + t708;
	const double t710 = Y*t709;
	const double t711 = r_rel3*t14*t29*t112*t510*2.0;
	const double t746 = r_rel3*t14*t25*t113*t510*8.0;
	const double t747 = r_rel3*t14*t81*t114*t510*8.0;
	const double t712 = t575 + t576 - t618 + t711 - t746 - t747;
	const double t713 = Z*t712;
	const double t714 = r_rel3*t14*t58*t113*t510*4.0;
	const double t715 = r_rel3*t14*t50*t112*t510*2.0;
	const double t745 = X*t705;
	const double t748 = r_rel3*t145*t510*2.0;
	const double t749 = r_rel3*t14*t42*t114*t510*4.0;
	const double t716 = t568 + t581 + t582 + t583 + t710 + t713 + t714 + t715 - t745 - t748 - t749;
	const double t717 = t127*t716;
	const double t718 = r_rel3*t14*t16*t33*t510*2.0;
	const double t750 = r_rel3*t14*t15*t34*t510*8.0;
	const double t751 = r_rel3*t14*t18*t32*t510*8.0;
	const double t719 = t523 + t524 - t596 + t718 - t750 - t751;
	const double t720 = X*t719;
	const double t721 = r_rel3*t14*t15*t79*t510*8.0;
	const double t722 = r_rel3*t14*t16*t23*t510*4.0;
	const double t752 = r_rel3*t14*t18*t22*t510*4.0;
	const double t723 = t511 + t512 + t513 + t721 + t722 - t752;
	const double t724 = Y*t723;
	const double t725 = r_rel3*t14*t16*t81*t510*4.0;
	const double t753 = r_rel3*t14*t18*t25*t510*8.0;
	const double t754 = r_rel3*t14*t15*t29*t510*4.0;
	const double t726 = t516 + t517 - t592 + t725 - t753 - t754;
	const double t727 = r_rel3*t14*t15*t50*t510*4.0;
	const double t755 = Z*t726;
	const double t756 = r_rel3*t75*t510*2.0;
	const double t757 = r_rel3*t14*t18*t58*t510*4.0;
	const double t758 = r_rel3*t14*t16*t42*t510*2.0;
	const double t728 = t522 + t528 + t530 - t599 + t720 + t724 + t727 - t755 - t756 - t757 - t758;
	const double t729 = t87*t728*2.0;
	const double t730 = r_rel3*t14*t23*t31*t510*8.0;
	const double t731 = r_rel3*t14*t28*t79*t510*8.0;
	const double t759 = r_rel3*t14*t22*t26*t510*2.0;
	const double t879 = t624 - t675 - t676 + t730 + t731 - t759;
	const double t732 = Y*t879;
	const double t733 = r_rel3*t14*t31*t81*t510*8.0;
	const double t760 = r_rel3*t14*t25*t26*t510*4.0;
	const double t761 = r_rel3*t14*t28*t29*t510*4.0;
	const double t734 = t630 + t631 - t677 + t733 - t760 - t761;
	const double t735 = r_rel3*t14*t31*t33*t510*4.0;
	const double t763 = r_rel3*t14*t28*t34*t510*8.0;
	const double t764 = r_rel3*t14*t26*t32*t510*4.0;
	const double t880 = t638 + t639 - t680 + t735 - t763 - t764;
	const double t736 = X*t880;
	const double t737 = r_rel3*t105*t510*2.0;
	const double t738 = r_rel3*t14*t28*t50*t510*4.0;
	const double t762 = Z*t734;
	const double t765 = r_rel3*t14*t31*t42*t510*4.0;
	const double t766 = r_rel3*t14*t26*t58*t510*2.0;
	const double t739 = t637 + t644 + t645 - t683 + t732 + t736 + t737 + t738 - t762 - t765 - t766;
	const double t767 = t59*t739*2.0;
	const double t740 = t729 - t767;
	const double t741 = t155*t163*t740*(1.0 / 2.0);
	const double t742 = t717 + t741;
	const double t768 = t12*t16*t87*2.0;
	const double t769 = r_rel3*2.0;
	const double t778 = r_rel1*r_rel2*2.0;
	const double t770 = t769 - t778;
	const double t779 = t12*t59*t770*2.0;
	const double t771 = t768 - t779;
	const double t772 = t155*t163*t771*(1.0 / 2.0);
	const double t773 = r_rel2*2.0;
	const double t774 = r_rel1*r_rel3*2.0;
	const double t775 = t773 + t774;
	const double t776 = t12*t127*t775;
	const double t777 = t772 + t776;
	const double t780 = t769 + t778;
	const double t781 = t12*t26*t59*2.0;
	const double t787 = t12*t87*t780*2.0;
	const double t782 = t781 - t787;
	const double t783 = t155*t163*t782*(1.0 / 2.0);
	const double t784 = r_rel1*2.0;
	const double t788 = r_rel2*r_rel3*2.0;
	const double t785 = t784 - t788;
	const double t786 = t12*t127*t785;
	const double t789 = t783 + t786;
	const double t790 = t773 - t774;
	const double t791 = t784 + t788;
	const double t792 = t12*t59*t791*2.0;
	const double t796 = t12*t87*t790*2.0;
	const double t793 = t792 - t796;
	const double t794 = t12*t112*t127;
	const double t797 = t155*t163*t793*(1.0 / 2.0);
	const double t795 = t794 - t797;
	const double t798 = t155*t163*(t164 - t167)*(1.0 / 2.0);
	const double t799 = t162 + t798;
	const double t800 = e*t87;
	const double t801 = t92 + t97 + t106 - t107 - t108 + t109 - t110 + t800;
	const double t802 = a11*t161*t187;
	const double t803 = a1*t116*t118*t161*t187*1.1E1;
	const double t804 = a3*t118*t161*t187*9.0;
	const double t805 = a5*t116*t117*t161*t187*7.0;
	const double t806 = a7*t117*t161*t187*5.0;
	const double t807 = a9*t116*t161*t187*3.0;
	const double t808 = t802 + t803 + t804 + t805 + t806 + t807 - a10*t115*t161*t187*2.0 - a2*t115*t118*t161*t187*1.0E1 - a6*t115*t117*t161*t187*6.0 - a8*t115*t116*t161*t187*4.0 - a4*t115*t116*t117*t161*t187*8.0;
	const double t809 = a11*t161*t193;
	const double t810 = a1*t116*t118*t161*t193*1.1E1;
	const double t811 = a3*t118*t161*t193*9.0;
	const double t812 = a5*t116*t117*t161*t193*7.0;
	const double t813 = a7*t117*t161*t193*5.0;
	const double t814 = a9*t116*t161*t193*3.0;
	const double t815 = t809 + t810 + t811 + t812 + t813 + t814 - a10*t115*t161*t193*2.0 - a2*t115*t118*t161*t193*1.0E1 - a6*t115*t117*t161*t193*6.0 - a8*t115*t116*t161*t193*4.0 - a4*t115*t116*t117*t161*t193*8.0;
	const double t816 = a11*t161*t289;
	const double t817 = a1*t116*t118*t161*t289*1.1E1;
	const double t818 = a3*t118*t161*t289*9.0;
	const double t819 = a5*t116*t117*t161*t289*7.0;
	const double t820 = a7*t117*t161*t289*5.0;
	const double t821 = a9*t116*t161*t289*3.0;
	const double t822 = t816 + t817 + t818 + t819 + t820 + t821 - a10*t115*t161*t289*2.0 - a2*t115*t118*t161*t289*1.0E1 - a6*t115*t117*t161*t289*6.0 - a8*t115*t116*t161*t289*4.0 - a4*t115*t116*t117*t161*t289*8.0;
	const double t823 = a11*t161*t374;
	const double t824 = a1*t116*t118*t161*t374*1.1E1;
	const double t825 = a3*t118*t161*t374*9.0;
	const double t826 = a5*t116*t117*t161*t374*7.0;
	const double t827 = a7*t117*t161*t374*5.0;
	const double t828 = a9*t116*t161*t374*3.0;
	const double t829 = t823 + t824 + t825 + t826 + t827 + t828 - a10*t115*t161*t374*2.0 - a2*t115*t118*t161*t374*1.0E1 - a6*t115*t117*t161*t374*6.0 - a8*t115*t116*t161*t374*4.0 - a4*t115*t116*t117*t161*t374*8.0;
	const double t830 = a11*t161*t447;
	const double t831 = a1*t116*t118*t161*t447*1.1E1;
	const double t832 = a3*t118*t161*t447*9.0;
	const double t833 = a5*t116*t117*t161*t447*7.0;
	const double t834 = a7*t117*t161*t447*5.0;
	const double t835 = a9*t116*t161*t447*3.0;
	const double t836 = t830 + t831 + t832 + t833 + t834 + t835 - a10*t115*t161*t447*2.0 - a2*t115*t118*t161*t447*1.0E1 - a6*t115*t117*t161*t447*6.0 - a8*t115*t116*t161*t447*4.0 - a4*t115*t116*t117*t161*t447*8.0;
	const double t837 = a11*t161*t471;
	const double t838 = a1*t116*t118*t161*t471*1.1E1;
	const double t839 = a3*t118*t161*t471*9.0;
	const double t840 = a5*t116*t117*t161*t471*7.0;
	const double t841 = a7*t117*t161*t471*5.0;
	const double t842 = a9*t116*t161*t471*3.0;
	const double t843 = t837 + t838 + t839 + t840 + t841 + t842 - a10*t115*t161*t471*2.0 - a2*t115*t118*t161*t471*1.0E1 - a6*t115*t117*t161*t471*6.0 - a8*t115*t116*t161*t471*4.0 - a4*t115*t116*t117*t161*t471*8.0;
	const double t844 = a11*t161*t490;
	const double t845 = a1*t116*t118*t161*t490*1.1E1;
	const double t846 = a3*t118*t161*t490*9.0;
	const double t847 = a5*t116*t117*t161*t490*7.0;
	const double t848 = a7*t117*t161*t490*5.0;
	const double t849 = a9*t116*t161*t490*3.0;
	const double t850 = t844 + t845 + t846 + t847 + t848 + t849 - a10*t115*t161*t490*2.0 - a2*t115*t118*t161*t490*1.0E1 - a6*t115*t117*t161*t490*6.0 - a8*t115*t116*t161*t490*4.0 - a4*t115*t116*t117*t161*t490*8.0;
	const double t851 = a11*t161*t505;
	const double t852 = a1*t116*t118*t161*t505*1.1E1;
	const double t853 = a3*t118*t161*t505*9.0;
	const double t854 = a5*t116*t117*t161*t505*7.0;
	const double t855 = a7*t117*t161*t505*5.0;
	const double t856 = a9*t116*t161*t505*3.0;
	const double t857 = t851 + t852 + t853 + t854 + t855 + t856 - a10*t115*t161*t505*2.0 - a2*t115*t118*t161*t505*1.0E1 - a6*t115*t117*t161*t505*6.0 - a8*t115*t116*t161*t505*4.0 - a4*t115*t116*t117*t161*t505*8.0;
	const double t858 = a11*t161*t588;
	const double t859 = a1*t116*t118*t161*t588*1.1E1;
	const double t860 = a3*t118*t161*t588*9.0;
	const double t861 = a5*t116*t117*t161*t588*7.0;
	const double t862 = a7*t117*t161*t588*5.0;
	const double t863 = a9*t116*t161*t588*3.0;
	const double t864 = t858 + t859 + t860 + t861 + t862 + t863 - a10*t115*t161*t588*2.0 - a2*t115*t118*t161*t588*1.0E1 - a6*t115*t117*t161*t588*6.0 - a8*t115*t116*t161*t588*4.0 - a4*t115*t116*t117*t161*t588*8.0;
	const double t865 = a11*t161*t674;
	const double t866 = a1*t116*t118*t161*t674*1.1E1;
	const double t867 = a3*t118*t161*t674*9.0;
	const double t868 = a5*t116*t117*t161*t674*7.0;
	const double t869 = a7*t117*t161*t674*5.0;
	const double t870 = a9*t116*t161*t674*3.0;
	const double t871 = t865 + t866 + t867 + t868 + t869 + t870 - a10*t115*t161*t674*2.0 - a2*t115*t118*t161*t674*1.0E1 - a6*t115*t117*t161*t674*6.0 - a8*t115*t116*t161*t674*4.0 - a4*t115*t116*t117*t161*t674*8.0;
	const double t872 = a11*t161*t742;
	const double t873 = a1*t116*t118*t161*t742*1.1E1;
	const double t874 = a3*t118*t161*t742*9.0;
	const double t875 = a5*t116*t117*t161*t742*7.0;
	const double t876 = a7*t117*t161*t742*5.0;
	const double t877 = a9*t116*t161*t742*3.0;
	const double t878 = t872 + t873 + t874 + t875 + t876 + t877 - a10*t115*t161*t742*2.0 - a2*t115*t118*t161*t742*1.0E1 - a6*t115*t117*t161*t742*6.0 - a8*t115*t116*t161*t742*4.0 - a4*t115*t116*t117*t161*t742*8.0;
	const double t881 = a11*t161*t777;
	const double t882 = a1*t116*t118*t161*t777*1.1E1;
	const double t883 = a3*t118*t161*t777*9.0;
	const double t884 = a5*t116*t117*t161*t777*7.0;
	const double t885 = a7*t117*t161*t777*5.0;
	const double t886 = a9*t116*t161*t777*3.0;
	const double t887 = t881 + t882 + t883 + t884 + t885 + t886 - a10*t115*t161*t777*2.0 - a2*t115*t118*t161*t777*1.0E1 - a6*t115*t117*t161*t777*6.0 - a8*t115*t116*t161*t777*4.0 - a4*t115*t116*t117*t161*t777*8.0;
	const double t888 = a11*t161*t789;
	const double t889 = a1*t116*t118*t161*t789*1.1E1;
	const double t890 = a3*t118*t161*t789*9.0;
	const double t891 = a5*t116*t117*t161*t789*7.0;
	const double t892 = a7*t117*t161*t789*5.0;
	const double t893 = a9*t116*t161*t789*3.0;
	const double t894 = a11*t161*t795;
	const double t895 = a1*t116*t118*t161*t795*1.1E1;
	const double t896 = a3*t118*t161*t795*9.0;
	const double t897 = a5*t116*t117*t161*t795*7.0;
	const double t898 = a7*t117*t161*t795*5.0;
	const double t899 = a9*t116*t161*t795*3.0;
	const double t900 = t894 + t895 + t896 + t897 + t898 + t899 - a10*t115*t161*t795*2.0 - a2*t115*t118*t161*t795*1.0E1 - a6*t115*t117*t161*t795*6.0 - a8*t115*t116*t161*t795*4.0 - a4*t115*t116*t117*t161*t795*8.0;
 
	jacs(0, 0) = t127*t170*(a11*t161*t166 + a3*t118*t161*t166*9.0 + a7*t117*t161*t166*5.0 + a9*t116*t161*t166*3.0 - a10*t115*t161*t166*2.0 + a1*t116*t118*t161*t166*1.1E1 - a2*t115*t118*t161*t166*1.0E1 + a5*t116*t117*t161*t166*7.0 - a6*t115*t117*t161*t166*6.0 - a8*t115*t116*t161*t166*4.0 - a4*t115*t116*t117*t161*t166*8.0) - t176*1.0 / sqrt(t111 + t21*t21)*(c*t67 - d*t63) - t163*t170*t176*t177*(1.0 / 2.0);
	jacs(0, 1) = t127*t176*(c*t120 - d*t91) + t127*t170*t808 + t163*t170*t176*(t185 - t188)*(1.0 / 2.0);
	jacs(0, 2) = t127*t176*(c*t123 - d*t96) - t127*t170*t815 + t163*t170*t176*(t191 - t194)*(1.0 / 2.0);
	jacs(0, 3) = t127*t176*(c*t223 - d*t243) - t127*t170*t822 + t163*t170*t176*(t287 - t301)*(1.0 / 2.0);
	jacs(0, 4) = t127*t176*(c*t317 - d*t331) - t127*t170*t829 + t163*t170*t176*(t372 - t383)*(1.0 / 2.0);
	jacs(0, 5) = t127*t176*(c*t396 - d*t409) - t127*t170*t836 + t163*t170*t176*(t445 - t453)*(1.0 / 2.0);
	jacs(0, 6) = t127*t176*(c*t462 - d*t465) - t127*t170*t843 + t163*t170*t176*(t469 - t473)*(1.0 / 2.0);
	jacs(0, 7) = -t127*t176*(c*t488 - d*t484) - t127*t170*t850 - t163*t170*t176*t489*(1.0 / 2.0);
	jacs(0, 8) = -t127*t176*(c*t502 - d*t500) + t127*t170*t857 - t163*t170*t176*t503*(1.0 / 2.0);
	jacs(0, 9) = -t87*t127*t176;
	jacs(0, 10) = -t59*t127*t176;
	jacs(0, 11) = 0.0;
	jacs(0, 12) = 1.0;
	jacs(0, 13) = 0.0;
	jacs(0, 14) = t115*t116*t118*t127*t170;
	jacs(0, 15) = -t116*t118*t127*t170;
	jacs(0, 16) = t115*t118*t127*t170;
	jacs(0, 17) = -t118*t127*t170;
	jacs(0, 18) = t115*t116*t117*t127*t170;
	jacs(0, 19) = -t116*t117*t127*t170;
	jacs(0, 20) = t115*t117*t127*t170;
	jacs(0, 21) = -t117*t127*t170;
	jacs(0, 22) = t115*t116*t127*t170;
	jacs(0, 23) = -t116*t127*t170;
	jacs(0, 24) = t115*t127*t170;
	jacs(0, 25) = -t127*t170;
	jacs(0, 26) = -t127*t176*(c*t585 + d*t557) + t127*t170*t864 + t163*t170*t176*t587*(1.0 / 2.0);
	jacs(0, 27) = t127*t176*(c*t661 - d*t672) - t127*t170*t871 - t163*t170*t176*t673*(1.0 / 2.0);
	jacs(0, 28) = -t127*t176*(c*t728 - d*t739) - t127*t170*t878 + t163*t170*t176*t740*(1.0 / 2.0);
	jacs(0, 29) = -t127*t170*t887 - t127*t176*(c*t12*t16 - d*t12*t770) + t163*t170*t176*t771*(1.0 / 2.0);
	jacs(0, 30) = t127*t170*(t888 + t889 + t890 + t891 + t892 + t893 - a10*t115*t161*t789*2.0 - a2*t115*t118*t161*(t783 + t786)*1.0E1 - a6*t115*t117*t161*(t783 + t786)*6.0 - a8*t115*t116*t161*(t783 + t786)*4.0 - a4*t115*t116*t117*t161*(t783 + t786)*8.0) - t127*t176*(c*t12*t780 - d*t12*t26) - t163*t170*t176*t782*(1.0 / 2.0);
	jacs(0, 31) = t127*t170*t900 + t127*t176*(c*t12*t790 - d*t12*t791) + t163*t170*t176*(t792 - t796)*(1.0 / 2.0);

	jacs(1, 0) = t127*t801*(a11*t161*t799 + a3*t118*t161*t799*9.0 + a7*t117*t161*t799*5.0 + a9*t116*t161*t799*3.0 - a10*t115*t161*t799*2.0 + a1*t116*t118*t161*t799*1.1E1 - a2*t115*t118*t161*t799*1.0E1 + a5*t116*t117*t161*t799*7.0 - a6*t115*t117*t161*t799*6.0 - a8*t115*t116*t161*t799*4.0 - a4*t115*t116*t117*t161*t799*8.0) + t127*t176*(t60 - t61 + t62 - e*t67) - t163*t176*t177*t801*(1.0 / 2.0);
	jacs(1, 1) = t127*t801*t808 + t127*t176*(t88 - t89 - t90 + e*t120) + t163*t176*t801*(t185 - t188)*(1.0 / 2.0);
	jacs(1, 2) = -t127*t176*(t93 + t94 - t95 - e*t123) - t127*t801*t815 + t163*t176*t801*(t191 - t194)*(1.0 / 2.0);
	jacs(1, 3) = -t127*t176*(t228 + t234 + t240 + t241 + t242 - t283 - t284 - t285 - t286 - e*t223) - t127*t801*t822 + t163*t176*t801*(t287 - t301)*(1.0 / 2.0);
	jacs(1, 4) = -t127*t176*(t326 + t327 + t328 + t329 + t330 - t364 - t367 - t370 - t371 - e*t317) - t127*t801*t829 + t163*t176*t801*(t372 - t383)*(1.0 / 2.0);
	jacs(1, 5) = -t127*t176*(t404 + t405 + t406 + t407 + t408 - t437 - t440 - t443 - t444 - e*t396) - t127*t801*t836 + t163*t176*t801*(t445 - t453)*(1.0 / 2.0);
	jacs(1, 6) = -t127*t801*t843 + t127*t176*(t61 - t463 - t464 + e*t462) + t163*t176*t801*(t469 - t473)*(1.0 / 2.0);
	jacs(1, 7) = -t127*t801*t850 - t127*t176*(t88 - t482 - t483 + e*t488) - t163*t176*t489*t801*(1.0 / 2.0);
	jacs(1, 8) = t127*t176*(t94 + t499 - t506 - e*t502) + t127*t801*t857 - t163*t176*t503*t801*(1.0 / 2.0);
	jacs(1, 9) = 0.0;
	jacs(1, 10) = 0.0;
	jacs(1, 11) = -t87*t127*t176;
	jacs(1, 12) = 0.0;
	jacs(1, 13) = 1.0;
	jacs(1, 14) = t115*t116*t118*t127*t801;
	jacs(1, 15) = -t116*t118*t127*t801;
	jacs(1, 16) = t115*t118*t127*t801;
	jacs(1, 17) = -t118*t127*t801;
	jacs(1, 18) = t115*t116*t117*t127*t801;
	jacs(1, 19) = -t116*t117*t127*t801;
	jacs(1, 20) = t115*t117*t127*t801;
	jacs(1, 21) = -t117*t127*t801;
	jacs(1, 22) = t115*t116*t127*t801;
	jacs(1, 23) = -t116*t127*t801;
	jacs(1, 24) = t115*t127*t801;
	jacs(1, 25) = -t127*t801;
	jacs(1, 26) = -t127*t176*(t544 + t547 + t553 + t554 + t555 + t556 - t604 - t609 - t610 - t611 - t612 + e*t585) + t127*t801*t864 + t163*t176*t587*t801*(1.0 / 2.0);
	jacs(1, 27) = -t127*t801*t871 - t127*t176*(t568 + t581 + t582 + t583 + t666 + t669 + t670 + t671 - t695 - t700 - t701 - e*t661) - t163*t176*t673*t801*(1.0 / 2.0);
	jacs(1, 28) = t127*t176*(t637 + t644 + t645 - t683 + t732 + t736 + t737 + t738 - t765 - t766 - e*t728 - Z*(t630 + t631 - t677 + t733 - t760 - t761)) - t127*t801*t878 + t163*t176*t740*t801*(1.0 / 2.0);
	jacs(1, 29) = t127*t176*(t12*t770 - e*t12*t16) - t127*t801*t887 + t163*t176*t771*t801*(1.0 / 2.0);
	jacs(1, 30) = t127*t801*(t888 + t889 + t890 + t891 + t892 + t893 - a10*t115*t161*(t783 + t786)*2.0 - a2*t115*t118*t161*(t783 + t786)*1.0E1 - a6*t115*t117*t161*(t783 + t786)*6.0 - a8*t115*t116*t161*(t783 + t786)*4.0 - a4*t115*t116*t117*t161*(t783 + t786)*8.0) + t127*t176*(t12*t26 - e*t12*t780) - t163*t176*t782*t801*(1.0 / 2.0);
	jacs(1, 31) = -t127*t176*(t12*t791 - e*t12*t790) + t127*t801*t900 + t163*t176*t801*(t792 - t796)*(1.0 / 2.0);

}
}