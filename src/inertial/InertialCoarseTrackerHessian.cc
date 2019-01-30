using namespace std;

#include "inertial/InertialCoarseTrackerHessian.h"
#include "inertial/InertialFrameFrameHessian.h"
#include "inertial/InertialFrameHessian.h"

namespace ldso {
	namespace inertial {

		InertialCoarseTrackerHessian::InertialCoarseTrackerHessian()
		{
			Vec25 s;
			s << SCALE_XI_TRANS, SCALE_XI_TRANS, SCALE_XI_TRANS, SCALE_XI_ROT, SCALE_XI_ROT, SCALE_XI_ROT, SCALE_VI_ROT, SCALE_VI_ROT, SCALE_VI_ROT, SCALE_VI_S, SCALE_VI_TRANS, SCALE_VI_TRANS, SCALE_VI_TRANS, SCALE_VI_ROT, SCALE_VI_ROT, SCALE_VI_ROT, SCALE_VI_V, SCALE_VI_V, SCALE_VI_V, SCALE_VI_B, SCALE_VI_B, SCALE_VI_B, SCALE_VI_B, SCALE_VI_B, SCALE_VI_B;
			S = s.asDiagonal();
		}

		void InertialCoarseTrackerHessian::compute(double visualWeight, SE3 T_id, SE3 T_ji) {
			Mat1515 J_i = Mat1515::Zero();
			Mat1515 J_j = Mat1515::Zero();
			Vec15 r_pr = Vec15::Zero();
			Vec6 r_co = Vec6::Zero();
			Mat625 J_co = Mat625::Zero();

			energy = 0;
			H_I.setZero();


			InertialFrameFrameHessian::computeResidual(r_pr, preIntegration, Tw_i.translation(), Tw_j.translation(), Tw_i.so3().inverse(), Tw_j.so3().inverse(), Tw_j.so3(), v_i, v_j, bg_i, bg_j, ba_i, ba_j);
			InertialFrameFrameHessian::computeJacobian(J_i, J_j, preIntegration, Tw_i.translation(), Tw_j.translation(), Tw_i.so3().inverse(), Tw_j.so3().inverse(), Tw_j.so3(), v_i, v_j, bg_i, bg_j, ba_i, ba_j);

			SE3 Tcd = T_ji * T_id;

			InertialFrameHessian::computeResidual(r_co, scale, Tw_j.so3(), Tw_j.so3().inverse(), T_bc.so3().inverse(), T_bc.so3(), Tcd.so3(), Tcd.so3().inverse(), R_wd.inverse(), R_wd, Tw_j.translation(), T_bc.inverse().translation(), Tcd.inverse());
			InertialFrameHessian::computeJacobian(J_co, scale, Tw_j.so3(), Tw_j.so3().inverse(), T_bc.so3().inverse(), T_bc.so3(), Tcd.so3(), Tcd.so3().inverse(), R_wd.inverse(), R_wd, Tw_j.translation(), T_bc.inverse().translation(), Tcd.inverse());

			J_co = J_co * S;

			W.setZero();
			W.block<9, 9>(0, 0) = preIntegration->Sigma_ij;
			W.block<6, 6>(9, 9) = preIntegration->Sigma_bd * preIntegration->dt_ij;
			W = W.inverse();

			w.setZero();
			w.block<3, 1>(0, 0) = Vec3(setting_vi_lambda_rot * setting_vi_lambda_rot, setting_vi_lambda_rot* setting_vi_lambda_rot, setting_vi_lambda_rot*setting_vi_lambda_rot);
			w.block<3, 1>(3, 0) = Vec3(setting_vi_lambda_trans*setting_vi_lambda_trans, setting_vi_lambda_trans*setting_vi_lambda_trans, setting_vi_lambda_trans*setting_vi_lambda_trans);

			if (fix_i)
			{
				J_j = J_j * S.block<15, 15>(10, 10);
				Hbb_inv = MatXX::Zero(15, 15);
				bb = VecX::Zero(15);
				Hab = VecX::Zero(8, 15);

				Mat1515 Hj;

				Hj = J_j.transpose() * visualWeight * W * J_j;

				if (Hj.determinant() < 1e-8)
					Hbb_inv = (Hj + VecX::Constant(Hj.cols(), 10e-8).asDiagonal().toDenseMatrix()).inverse();
				else
					Hbb_inv.noalias() = Hj.inverse();

				bb = -J_j.transpose() * visualWeight * W * r_pr;
				Hab.block<6, 15>(0, 15) = J_co.block<6, 6>(0, 0).transpose() * visualWeight * w.asDiagonal() *  J_co.block<6, 15>(0, 10);
			}
			else
			{
				J_i = J_i * S.block<15, 15>(10, 10);
				J_j = J_j * S.block<15, 15>(10, 10);
				Hbb_inv = MatXX::Zero(30, 30);
				bb = VecX::Zero(30);
				Hab = VecX::Zero(8, 30);

				Mat1515 Hi;
				Mat1515 Hj;

				Hi = J_i.transpose() * visualWeight * W * J_i;
				Hj = J_j.transpose() * visualWeight * W * J_j;

				if (Hi.determinant() < 1e-8)
					Hbb_inv.block<15, 15>(0, 0) = (Hi + VecX::Constant(Hi.cols(), 10e-8).asDiagonal().toDenseMatrix()).inverse();
				else
					Hbb_inv.block<15, 15>(0, 0) = Hi.inverse();

				if (Hj.determinant() < 1e-8)
					Hbb_inv.block<15, 15>(15, 15) = (Hj + VecX::Constant(Hj.cols(), 10e-8).asDiagonal().toDenseMatrix()).inverse();
				else
					Hbb_inv.block<15, 15>(15, 15) = Hj.inverse();

				bb.block<15, 1>(0, 0) = -J_i.transpose() * visualWeight * W * r_pr;
				bb.block<15, 1>(15, 0) = -J_j.transpose() * visualWeight * W * r_pr;
				Hab.block<6, 15>(0, 0) = J_co.block<6, 6>(0, 0).transpose() * visualWeight * w.asDiagonal() *  J_co.block<6, 15>(0, 10);
			}

			H_I.block<6, 6>(0, 0) = J_co.block<6, 6>(0, 0).transpose() * visualWeight * w.asDiagonal() *  J_co.block<6, 6>(0, 0);
			b_I.block<6, 1>(0, 0) = -J_co.block<6, 6>(0, 0).transpose() * visualWeight * w.asDiagonal() * r_co;

			MatXX HabHbbinv;
			HabHbbinv.noalias() = Hab * Hbb_inv;

			H_I_sc.noalias() = HabHbbinv * Hab.transpose();
			b_I_sc.noalias() = HabHbbinv * bb;

			energy += r_pr.transpose() * visualWeight * W * r_pr;
			energy += r_co.transpose() * visualWeight * w.asDiagonal() * r_co;

		}

		void InertialCoarseTrackerHessian::setValues(std::vector <shared_ptr<internal::FrameHessian>> &frameHessians, shared_ptr<inertial::InertialHessian> Hinertial)
		{
			assert(frameHessians.size() > 0);
			shared_ptr<internal::FrameHessian> fh = frameHessians.back();

			v_i = fh->inertialFrameHessian->W_v_B_PRE;
			v_j = v_i;

			Tw_i = fh->inertialFrameHessian->T_WB_PRE;
			Tw_j = Tw_i;

			bg_i = fh->inertialFrameHessian->db_g_PRE;
			bg_j = bg_i;

			ba_i = fh->inertialFrameHessian->db_g_PRE;
			ba_j = ba_i;

			scale = Hinertial->scale_PRE;
			T_bc = Hinertial->T_BC;
			R_wd = Hinertial->R_WD_PRE;
			fix_i = true;
		}

		void InertialCoarseTrackerHessian::update(Vec8 x) {
			VecX xb;

			xb = S.block<15, 15>(10, 10)*(Hbb_inv * (bb - Hab.transpose() * x));


			if (!fix_i)
			{
				Tw_i = SE3::exp(xb.block<6, 1>(0, 0)) * Tw_i;
				v_i += xb.block<3, 1>(6, 0);
				bg_i += xb.block<3, 1>(9, 0);
				ba_i += xb.block<3, 1>(12, 0);
				Tw_j = SE3::exp(xb.block<6, 1>(0 + 15, 0)) * Tw_j;
				v_j += xb.block<3, 1>(6 + 15, 0);
				bg_j += xb.block<3, 1>(9 + 15, 0);
				ba_j += xb.block<3, 1>(12 + 15, 0);
			}
			else
			{
				Tw_j = SE3::exp(xb.block<6, 1>(0, 0)) * Tw_j;
				v_j += xb.block<3, 1>(6, 0);
				bg_j += xb.block<3, 1>(9, 0);
				ba_j += xb.block<3, 1>(12, 0);
			}
		}
		void InertialCoarseTrackerHessian::backup() {
			x_backup_i = Vec15::Zero();
			x_backup_j = Vec15::Zero();

			x_backup_i.block<6, 1>(0, 0) = Tw_i.log();
			x_backup_j.block<6, 1>(0, 0) = Tw_j.log();

			x_backup_i.block<3, 1>(6, 0) = v_i;
			x_backup_j.block<3, 1>(6, 0) = v_j;

			x_backup_i.block<3, 1>(9, 0) = bg_i;
			x_backup_j.block<3, 1>(9, 0) = bg_j;

			x_backup_i.block<3, 1>(12, 0) = ba_i;
			x_backup_j.block<3, 1>(12, 0) = ba_j;
		}

		void InertialCoarseTrackerHessian::reset() {
			Tw_i = SE3::exp(x_backup_i.block<6, 1>(0, 0));
			Tw_j = SE3::exp(x_backup_j.block<6, 1>(0, 0));

			v_i = x_backup_i.block<3, 1>(6, 0);
			v_j = x_backup_j.block<3, 1>(6, 0);

			bg_i = x_backup_i.block<3, 1>(9, 0);
			bg_j = x_backup_j.block<3, 1>(9, 0);

			ba_i = x_backup_i.block<3, 1>(12, 0);
			ba_j = x_backup_j.block<3, 1>(12, 0);
		}
	}
}