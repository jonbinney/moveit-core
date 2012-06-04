/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2012, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/*------------------------------------------------------*/
/*   DO NOT INCLUDE THIS FILE DIRECTLY                  */
/*------------------------------------------------------*/

/** \brief A fixed joint */
class FixedJointModel : public JointModel
{
  friend class KinematicModel;
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  
  FixedJointModel(const std::string &name);
  
  virtual void getDefaultValues(std::vector<double> &values, const Bounds &other_bounds) const;    
  virtual void getRandomValues(random_numbers::RandomNumberGenerator &rng, std::vector<double> &values, const Bounds &other_bounds) const;
  virtual void getRandomValuesNearBy(random_numbers::RandomNumberGenerator &rng, std::vector<double> &values, const Bounds &other_bounds,
                                     const std::vector<double> &near, const double distance) const;
  virtual void enforceBounds(std::vector<double> &values, const Bounds &other_bounds) const;
  virtual bool satisfiesBounds(const std::vector<double> &values, const Bounds &other_bounds) const;
  
  virtual unsigned int getStateSpaceDimension(void) const;   
  virtual double getMaximumExtent(void) const;
  virtual double distance(const std::vector<double> &values1, const std::vector<double> &values2) const;    
  virtual void interpolate(const std::vector<double> &from, const std::vector<double> &to, const double t, std::vector<double> &state) const;
  virtual void computeTransform(const std::vector<double>& joint_values, Eigen::Affine3d &transf) const;
  virtual void computeJointStateValues(const Eigen::Affine3d& trans, std::vector<double>& joint_values) const;
  virtual void updateTransform(const std::vector<double>& joint_values, Eigen::Affine3d &transf) const;
};