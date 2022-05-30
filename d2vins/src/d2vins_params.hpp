#pragma once
#include <Eigen/Eigen>
#include <ros/ros.h>
#include <swarm_msgs/Pose.h>
#include <ceres/ceres.h>

#define UNIT_SPHERE_ERROR
using namespace Eigen;

namespace D2VINS {

struct D2VINSConfig {
    //Inputs
    std::string imu_topic;

    //Sensor config
    double acc_n = 0.1;
    double gyr_n = 0.05;
    double acc_w = 0.002;
    double gyr_w = 0.0004;
    double focal_length = 460.0;

    //Sensor frequency
    double IMU_FREQ = 400.0;
    int camera_num = 1; // number of cameras;
    int frame_step = 3; //step of frame to use in backend.
    
    //Sliding window
    int min_solve_frames = 9;
    int max_sld_win_size = 10;
    int landmark_estimate_tracks = 4; //thres for landmark to tracking

    //Initialization
    enum InitialMethod {
        INIT_POSE_IMU,
        INIT_POSE_PNP
    };
    int pnp_min_inliers = 8;
    int pnp_iteratives = 100;
    int init_imu_num = 10;
    InitialMethod init_method = INIT_POSE_PNP;
    double depth_estimate_baseline = 0.05;
    double tri_max_err = 0.1;
    
    //Estimation
    bool estimate_td = false;
    bool estimate_extrinsic = false;
    double solver_time = 0.04;
    enum {
        LM_INV_DEP,
        LM_POS
    } landmark_param = LM_INV_DEP;
    bool always_fixed_first_pose = false;
    double estimator_timer_freq = 1000.0;
    int warn_pending_frames = 10;
    enum {
        SINGLE_DRONE_MODE, //Not accept remote frame
        SOLVE_ALL_MODE, //Each drone solve all the information
        DISTRIBUTED_CAMERA_CONSENUS //Distributed camera consensus
    } estimation_mode = SOLVE_ALL_MODE;

    //Fuse depth
    bool fuse_dep = true;
    double min_inv_dep = 1e-1; //10 meter away
    double depth_sqrt_inf = 20.0;
    double max_depth_to_fuse = 5.;
    double min_depth_to_fuse = 0.3;
    ceres::Solver::Options options;

    //Outlier rejection
    int perform_outlier_rejection_num = 50;
    double landmark_outlier_threshold = 10.0;

    //Margin config
    bool margin_sparse_solver = true;
    bool enable_marginalization = true;
    int remove_base_when_margin_remote = 2;

    //Safety
    int min_measurements_per_keyframe = 10;
    double max_imu_time_err = 0.0025;

    //Multi-drone
    int self_id = 0;

    //Debug
    bool debug_print_states = false;
    bool debug_print_sldwin = false;
    std::string output_folder;
    bool enable_perf_output = false;
    bool debug_write_margin_matrix = false;

    bool verbose = true;

    //Initialial states
    std::vector<Swarm::Pose> camera_extrinsics;
    double td_initial = 0.0;

    void init(const std::string & config_file);
};

extern D2VINSConfig * params;
void initParams(ros::NodeHandle & nh);
}