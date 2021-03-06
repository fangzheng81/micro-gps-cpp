#ifndef _MICRO_GPS_H_
#define _MICRO_GPS_H_

#include <string>
#include <algorithm>
#include <Eigen/Dense>

#include "image_dataset.h"
#include "image.h"
#include "image_func.h"

#include "flann/flann.h"

extern "C" {
  #include "inpolygon.h"
}


namespace MicroGPS {

struct LocalizationOptions {
  bool  m_save_debug_info;
  bool  m_generate_alignment_image;
  bool  m_do_match_verification;
  float m_image_scale_for_sift;
  int   m_best_knn;
  float m_confidence_thresh;  // not used
  bool  m_use_visual_words;
  bool  m_use_sift_for_verification;

  LocalizationOptions() {
    this->reset();
  }

  void reset() {
    m_save_debug_info           = true;
    m_generate_alignment_image  = true;
    m_do_match_verification     = true;
    m_image_scale_for_sift      = 0.5;
    m_best_knn                  = 9999;
    m_confidence_thresh         = 0.8f;
    m_use_visual_words          = false;
    m_use_sift_for_verification = true;
  }
};



struct LocalizationResult {
  // always compute
  Eigen::MatrixXf               m_final_estimated_pose;
  std::vector<int>              m_top_cells;
  float                         m_confidence;
  bool                          m_success_flag;
  bool                          m_can_estimate_pose;

  // optionally compute
  Eigen::MatrixXf               m_siftmatch_estimated_pose;
  float                         m_x_error; //comparing with result from siftmatch
  float                         m_y_error; 
  float                         m_angle_error;

  float                         m_cell_size;
  float                         m_peak_topleft_x;
  float                         m_peak_topleft_y;
  std::vector<Eigen::Matrix3f>  m_matched_feature_poses; 
  std::vector<Eigen::Matrix3f>  m_candidate_image_poses; 
  std::vector<Eigen::Matrix3f>  m_test_feature_poses; 
  std::string                   m_test_image_path;
  std::string                   m_closest_database_image_path;
  int                           m_closest_database_image_idx;

  LocalizationResult() {
    this->reset();
  }


  void reset() {
    // always compute
    m_final_estimated_pose          = Eigen::MatrixXf::Identity(3,3);
    m_top_cells.resize(0);
    m_confidence                    = 0.0f;

    // optional
    m_success_flag                  = false;
    m_can_estimate_pose             = false;
    m_siftmatch_estimated_pose      = Eigen::MatrixXf::Identity(3,3);
    m_x_error                       = -9999.9f;
    m_y_error                       = -9999.9f;
    m_angle_error                   = -9999.9f;
    m_cell_size                     = 0.0f;
    m_peak_topleft_x                = -9999.9f;
    m_peak_topleft_y                = -9999.9f;
    m_matched_feature_poses.resize(0);
    m_candidate_image_poses.resize(0);
    m_test_feature_poses.resize(0);
    m_test_image_path               = "";
    m_closest_database_image_path   = "";
    m_closest_database_image_idx    = -1;
  }


  void printToFile(FILE*& fp) {
    fprintf(fp, "----- Result -----\n");
    fprintf(fp, "Estimated Pose: %f %f %f %f %f %f %f %f %f\n", m_final_estimated_pose(0, 0), m_final_estimated_pose(0, 1), m_final_estimated_pose(0, 2),
                                                                m_final_estimated_pose(1, 0), m_final_estimated_pose(1, 1), m_final_estimated_pose(1, 2),
                                                                m_final_estimated_pose(2, 0), m_final_estimated_pose(2, 1), m_final_estimated_pose(2, 2));
    fprintf(fp, "Success: %d\n", m_success_flag);
    fprintf(fp, "Top cells: ");
    for(size_t i =0; i < m_top_cells.size(); i++) {
      fprintf(fp, "%d ", m_top_cells[i]);
    }
    fprintf(fp, "\n");

    fprintf(fp, "SIFTMatch Estimated Pose: %f %f %f %f %f %f %f %f %f\n", m_siftmatch_estimated_pose(0, 0), m_siftmatch_estimated_pose(0, 1), m_siftmatch_estimated_pose(0, 2),
                                                                          m_siftmatch_estimated_pose(1, 0), m_siftmatch_estimated_pose(1, 1), m_siftmatch_estimated_pose(1, 2),
                                                                          m_siftmatch_estimated_pose(2, 0), m_siftmatch_estimated_pose(2, 1), m_siftmatch_estimated_pose(2, 2));

    fprintf(fp, "x_error = %f, y_error = %f, angle_error = %f\n", m_x_error, m_y_error, m_angle_error);


    fprintf(fp, "----- Debug info -----\n");
    fprintf(fp, "Test image path: %s\n",              m_test_image_path.c_str());
    fprintf(fp, "Closest database image path: %s\n",  m_closest_database_image_path.c_str());
    fprintf(fp, "Grid step: %f\n",                    m_cell_size);
    fprintf(fp, "Peak top-left x: %f\n",              m_peak_topleft_x);
    fprintf(fp, "Peak top-left y: %f\n",              m_peak_topleft_y);
  }


};


struct LocalizationTiming {
  float m_sift_extraction;
  float m_dimension_reduction;
  float m_knn_search;
  float m_candidate_image_pose;
  float m_voting;
  float m_ransac;
  float m_total;
  
  LocalizationTiming() {
    this->reset();
  }
  
  void reset() {
    m_sift_extraction       = 0.0f;
    m_dimension_reduction   = 0.0f;
    m_knn_search            = 0.0f;
    m_candidate_image_pose  = 0.0f;
    m_voting                = 0.0f;
    m_ransac                = 0.0f;
    m_total                 = 0.0f;
  }


  void printToFile(FILE*& fp) {
    fprintf(fp, "----- Timing info -----\n");
    fprintf(fp, "Total: %f ms\n",                   m_total);
    fprintf(fp, "Feature Extraction: %f ms\n",      m_sift_extraction);
    fprintf(fp, "NN Search: %f ms\n",               m_knn_search);
    fprintf(fp, "Compute Candidate Poses: %f ms\n", m_candidate_image_pose);
    fprintf(fp, "Voting: %f ms\n",                  m_voting);
    fprintf(fp, "RANSAC: %f ms\n",                  m_ransac);
  }

};





class Localization {
public:
  Localization();
  ~Localization();

  void          setVotingCellSize(const float cell_size);
  void          setNumScaleGroups(const int num_scale_groups);
  void          loadImageDataset(MicroGPS::ImageDataset* image_dataset);

  void          computePCABasis();
  void          dimensionReductionPCA(const int num_dimensions_to_keep);
  void          preprocessDatabaseImages(const int num_samples_per_image, 
                                         const float image_scale_for_sift,
                                         const bool use_top_n = false);
  void          removeDuplicatedFeatures();
  void          removeDuplicatedFeatures2();

  void          buildSearchIndexMultiScales();
  void          searchNearestNeighborsMultiScales(MicroGPS::Image* work_image, 
                                                  std::vector<int>& nn_index, 
                                                  int best_knn = 9999);

  void          savePCABasis(const char* path);
  void          loadPCABasis(const char* path);
  void          saveFeatures(const char* path);
  void          loadFeatures(const char* path);

  // visual vocabs
  void          loadVisualWords(const char* path);
  void          dimensionReductionPCAVisualWords();
  void          buildVisualWordsSearchIndex();
  void          findNearestVisualWords(flann::Matrix<float>& flann_query,
                                       std::vector<int>& vw_id);
  void          fillVisualWordCells();
  void          saveVisualWordCells(const char* path);
  void          loadVisualWordCells(const char* path);
  void          searchNearestNeighborsByVisualWords(MicroGPS::Image* work_image,
                                                    std::vector<int>& src_index,
                                                    std::vector<int>& des_index);
  

  void          locateUseVW(MicroGPS::Image* work_image, 
                            LocalizationOptions* options,
                            LocalizationResult* results,
                            LocalizationTiming* timing,
                            MicroGPS::Image*& alignment_image);

  void          locateGlobalNN(MicroGPS::Image* work_image, 
                              LocalizationOptions* options,
                              LocalizationResult* results,
                              LocalizationTiming* timing,
                              MicroGPS::Image*& alignment_image);

  void          locate(MicroGPS::Image* work_image, 
                      LocalizationOptions* options,
                      LocalizationResult* results,
                      LocalizationTiming* timing,
                      MicroGPS::Image*& alignment_image);


  int           getClosestDatabaseImage (Eigen::Matrix3f pose_estimated,
                                         size_t im_width,
                                         size_t im_height);

  int           getClosestDatabaseImage2 (Eigen::Matrix3f pose_estimated,
                                          size_t im_width,
                                          size_t im_height);

  void          verifyAndGenerateAlignmentImage (MicroGPS::Image* work_image, 
                                                 Eigen::Matrix3f pose_estimated,
                                                 LocalizationOptions* options,
                                                 LocalizationResult* results,
                                                 LocalizationTiming* timing,
                                                 MicroGPS::Image*& alignment_image);


private:
  // image dataset
  MicroGPS::ImageDataset*                   m_image_dataset;
  int                                       m_image_width;
  int                                       m_image_height;
  std::vector<MicroGPS::Image*>             m_database_images;

  // image dataset
  Eigen::MatrixXf                           m_PCA_basis;  
  Eigen::MatrixXf                           m_features;
  Eigen::MatrixXf                           m_features_short;
  std::vector<Eigen::Matrix3f>              m_feature_poses;
  std::vector<int>                          m_feature_image_indices;
  std::vector<float>                        m_feature_scales;
  Eigen::MatrixXf                           m_feature_local_locations;
  float*                                    m_feature_poses_x;
  float*                                    m_feature_poses_y;
  int                                       m_dimensions_to_keep;


  // FLANN data
  int                                       m_num_scale_groups;
  std::vector<flann::Matrix<float> >        m_features_short_flann_multi_scales;
  std::vector<flann::Index<L2<float> >* >   m_flann_kdtree_multi_scales;
  std::vector<float>                        m_bounds_multi_scales;  
  std::vector<std::vector<int> >            m_global_index_multi_scales;


  // voting 
  std::vector<int>                          m_voting_grid;
  float                                     m_grid_step;
  float                                     m_grid_min_x;
  float                                     m_grid_min_y;
  int                                       m_grid_width;
  int                                       m_grid_height;

  // world limits
  float                                     m_world_min_x;
  float                                     m_world_min_y;
  float                                     m_world_max_x;
  float                                     m_world_max_y;

  // visual vocabs
  std::vector<std::vector<int> >            m_feature_vw_id;
  Eigen::MatrixXf                           m_visual_words;
  flann::Matrix<float>                      m_flann_visual_words_data;
  flann::Index<L2<float> >*                 m_flann_visual_words_kdtree;





};

}

#endif
