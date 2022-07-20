#pragma once
#include <ceres/ceres.h>
#include <swarm_msgs/Pose.h>
#include "BaseParamResInfo.hpp"
#include "../utils.hpp"

namespace D2Common {
class RelPoseFactor : public ceres::SizedCostFunction<6, 7> {
    Matrix3d q_sqrt_info;
    Matrix3d T_sqrt_info;
    Vector3d t_rel;
    Quaterniond q_rel;
public:
    RelPoseFactor(Swarm::Pose relative_pose, Matrix6d sqrt_info): 
        q_sqrt_info(sqrt_info.block<3,3>(3,3)), T_sqrt_info(sqrt_info.block<3,3>(0,0)) {
        t_rel = relative_pose.pos();
        q_rel = relative_pose.att();
    }

    bool Evaluate(double const *const *parameters, double *residuals, double **jacobians) const {
        Map<const Vector3d> t0(parameters[0]);
        Map<const Quaterniond> q0(parameters[0] + 3);
        Map<const Vector3d> t1(parameters[1]);
        Map<const Quaterniond> q1(parameters[1] + 3);
        Map<Vector3d> T_err(residuals);
        Map<Vector3d> theta_err(residuals + 3);
        Quaterniond q_0_inv = q0.conjugate();
        Matrix3d R_0_inv = q_0_inv.toRotationMatrix();
        Quaterniond q_01_est = q_0_inv * q1;
        Vector3d T_01_est = q_0_inv*(t1 - t0);  
        Quaterniond q_err = Utility::positify(q_rel.inverse() * q_01_est);
        theta_err = 2.0 * q_sqrt_info * q_err.vec();
        T_err = T_sqrt_info*(T_01_est - t_rel);
        if (jacobians) {
            //TODO: fill in jacobians...
            if (jacobians[0]) {
                Eigen::Map<Eigen::Matrix<double, 6, 7, Eigen::RowMajor>> jacobian_pose_0(jacobians[0]);
                jacobian_pose_0.setZero();

                jacobian_pose_0.block<3, 3>(0, 0) = -T_sqrt_info*R_0_inv;
                jacobian_pose_0.block<3, 3>(0, 3) = T_sqrt_info*Utility::skewSymmetric(R_0_inv*(t1 - t0)); 
                jacobian_pose_0.block<3, 3>(3, 3) = -q_sqrt_info*Utility::Qleft(q1.inverse() * q0).bottomRightCorner<3, 3>();
            }
            if (jacobians[1]) {
                Eigen::Map<Eigen::Matrix<double, 6, 7, Eigen::RowMajor>> jacobian_pose_1(jacobians[0]);
                jacobian_pose_1.setZero();
                jacobian_pose_1.block<3, 3>(0, 0) = T_sqrt_info*R_0_inv;
                jacobian_pose_1.block<3, 3>(3, 3) = q_sqrt_info*Utility::Qleft(q_0_inv * q1).bottomRightCorner<3, 3>();
            }

        }
        return true;
    }
};

class RelPoseFactor4D {
    Swarm::Pose relative_pose;
    Eigen::Vector3d relative_pos;
    double relative_yaw;
    Eigen::Matrix4d sqrt_inf;
    RelPoseFactor4D(const Swarm::Pose & _relative_pose, const Eigen::Matrix4d & _sqrt_inf):
        relative_pose(_relative_pose), sqrt_inf(_sqrt_inf) {
        relative_pos = relative_pose.pos();
        relative_yaw = relative_pose.yaw();
    }
    RelPoseFactor4D(const Swarm::Pose & _relative_pose, const Eigen::Matrix3d & _sqrt_inf_pos, double sqrt_info_yaw):
        relative_pose(_relative_pose) {
        relative_pos = relative_pose.pos();
        relative_yaw = relative_pose.yaw();
        sqrt_inf.setZero();
        sqrt_inf.block<3, 3>(0, 0) = _sqrt_inf_pos;
        sqrt_inf(3, 3) = sqrt_info_yaw;
    }
public:
    template<typename T>
    bool operator()(const T* const p_a_ptr, const T* const p_b_ptr, T *_residual) const {
        Eigen::Map<const Eigen::Matrix<T, 3, 1>> pos_a(p_a_ptr);
        Eigen::Map<const Eigen::Matrix<T, 3, 1>> pos_b(p_b_ptr);
        const Eigen::Matrix<T, 3, 1> _relative_pos = relative_pos.template cast<T>();
        const Eigen::Matrix<T, 4, 4> _sqrt_inf = sqrt_inf.template cast<T>();
        T relyaw_est = Utility::NormalizeAngle(p_b_ptr[3] - p_a_ptr[3]);
        Eigen::Matrix<T, 3, 1> relpos_est = Utility::yawRotMat(-p_a_ptr[3]) * (pos_b - pos_a);
        Utility::poseError4D<T>(relpos_est, relyaw_est, _relative_pos, (T)(relative_yaw) , _sqrt_inf, _residual);
        return true;
    }

    static ceres::CostFunction* Create(const Swarm::GeneralMeasurement2Drones* _loc) {
        auto loop = static_cast<const Swarm::LoopEdge*>(_loc);
        // std::cout << "Loop" << "sqrt_inf\n" << loop->get_sqrt_information_4d() << std::endl;
        return new ceres::AutoDiffCostFunction<RelPoseFactor4D, 4, 4, 4>(
            new RelPoseFactor4D(loop->relative_pose, loop->get_sqrt_information_4d()));
    }

    static ceres::CostFunction * Create(const Swarm::Pose & _relative_pose, const Eigen::Matrix3d & _sqrt_inf_pos, double sqrt_info_yaw) {
        return new ceres::AutoDiffCostFunction<RelPoseFactor4D, 4, 4, 4>(
            new RelPoseFactor4D(_relative_pose, _sqrt_inf_pos, sqrt_info_yaw));
    }
};

class RelPoseResInfo : public ResidualInfo {
public:
    FrameIdType frame_ida;
    FrameIdType frame_idb;
    bool is_4dof = false;
    RelPoseResInfo():ResidualInfo(ResidualType::RelPoseResidual) {}
    bool relavant(const std::set<FrameIdType> & frame_id) const override {
        return frame_id.find(frame_ida) != frame_id.end() || frame_id.find(frame_idb) != frame_id.end();
    }
    virtual std::vector<ParamInfo> paramsList(D2State * state) const override {
        std::vector<ParamInfo> params_list;
        if (is_4dof) {
            params_list.push_back(createFramePose4D(state, frame_ida));
            params_list.push_back(createFramePose4D(state, frame_idb));
        } else {
            params_list.push_back(createFramePose(state, frame_ida));
            params_list.push_back(createFramePose(state, frame_idb));
        }
        return params_list;
    }

    static RelPoseResInfo * create(
            ceres::CostFunction * cost_function, ceres::LossFunction * loss_function, 
            FrameIdType frame_ida, FrameIdType frame_idb, bool is_4dof=false) {
        auto * info = new RelPoseResInfo();
        info->frame_ida = frame_ida;
        info->frame_idb = frame_idb;
        info->cost_function = cost_function;
        info->loss_function = loss_function;
        info->is_4dof = is_4dof;
        return info;
    }
};
}