/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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

/** \author Ken Anderson */

#include <trajectory_processing/iterative_smoother.h>
#include <moveit_msgs/JointLimits.h>
#include <ros/console.h>

namespace trajectory_processing
{

const double DEFAULT_VEL_MAX=1.0;
const double DEFAULT_ACCEL_MAX=1.0;
const double ROUNDING_THRESHOLD = 0.01;

IterativeParabolicSmoother::IterativeParabolicSmoother(unsigned int max_iterations,
                                                       double max_time_change_per_it)
: max_iterations_(max_iterations),
  max_time_change_per_it_(max_time_change_per_it)
{}

IterativeParabolicSmoother::~IterativeParabolicSmoother()
{}

void IterativeParabolicSmoother::printPoint(const trajectory_msgs::JointTrajectoryPoint& point, unsigned int i) const
{
  ROS_DEBUG(  " time   [%i]= %f",i,point.time_from_start.toSec());
  if(point.positions.size() >= 7 )
  {
    ROS_DEBUG(" pos_   [%i]= %f %f %f %f %f %f %f",i,
              point.positions[0],point.positions[1],point.positions[2],point.positions[3],point.positions[4],point.positions[5],point.positions[6]);
  }
  if(point.velocities.size() >= 7 )
  {
    ROS_DEBUG("  vel_  [%i]= %f %f %f %f %f %f %f",i,
              point.velocities[0],point.velocities[1],point.velocities[2],point.velocities[3],point.velocities[4],point.velocities[5],point.velocities[6]);
  }
  if(point.accelerations.size() >= 7 )
  {
    ROS_DEBUG("   acc_ [%i]= %f %f %f %f %f %f %f",i,
              point.accelerations[0],point.accelerations[1],point.accelerations[2],point.accelerations[3],point.accelerations[4],point.accelerations[5],point.accelerations[6]);
  }
}

void IterativeParabolicSmoother::printStats(const trajectory_msgs::JointTrajectory& trajectory,
                                            const std::vector<moveit_msgs::JointLimits>& limits) const
{
  ROS_DEBUG("jointNames= %s %s %s %s %s %s %s",
    limits[0].joint_name.c_str(),limits[1].joint_name.c_str(),limits[2].joint_name.c_str(),
    limits[3].joint_name.c_str(),limits[4].joint_name.c_str(),limits[5].joint_name.c_str(),
    limits[6].joint_name.c_str());
  ROS_DEBUG("maxVelocities= %f %f %f %f %f %f %f",
    limits[0].max_velocity,limits[1].max_velocity,limits[2].max_velocity,
    limits[3].max_velocity,limits[4].max_velocity,limits[5].max_velocity,
    limits[6].max_velocity);
  ROS_DEBUG("maxAccelerations= %f %f %f %f %f %f %f",
    limits[0].max_acceleration,limits[1].max_acceleration,limits[2].max_acceleration,
    limits[3].max_acceleration,limits[4].max_acceleration,limits[5].max_acceleration,
    limits[6].max_acceleration);
  // for every point in time:
  for (unsigned int i=0; i<trajectory.points.size(); ++i)
  {
    const trajectory_msgs::JointTrajectoryPoint& point = trajectory.points[i];
    printPoint(point, i);
  }
}

// Applies velocity
void IterativeParabolicSmoother::applyVelocityConstraints(trajectory_msgs::JointTrajectory& trajectory, 
                                                          const std::vector<moveit_msgs::JointLimits>& limits,
                                                          std::vector<double> &time_diff) const
{

  // aleeper: This function computes the minimum time needed for each trajectory
  //          point by (forward_difference/ v_max), and recording the minimum time in
  //          the time_diff vector, which will be used later.
  const unsigned int num_points = trajectory.points.size();
  const unsigned int num_joints = trajectory.joint_names.size();

  // Initial values
  for (unsigned int i=0; i<num_points; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point = trajectory.points[i];
    point.velocities.resize(num_joints);
    point.accelerations.resize(num_joints);
  }

  for (unsigned int i=0; i<num_points-1; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point1 = trajectory.points[i];
    trajectory_msgs::JointTrajectoryPoint& point2 = trajectory.points[i+1];

    // Get velocity min/max
    for (unsigned int j=0; j<num_joints; ++j)
    {
      double v_max = 1.0;

      if( limits[j].has_velocity_limits )
      {
        v_max = limits[j].max_velocity;
      }

      const double dq1 = point1.positions[j];
      const double dq2 = point2.positions[j];
      const double t_min = std::abs(dq2-dq1) / v_max;

      if( t_min > time_diff[i] )
      {
        time_diff[i] = t_min;
      }
    }
  }
}

// Iteratively expand dt1 interval by a constant factor until within acceleration constraint
// In the future we may want to solve to quadratic equation to get the exact timing interval.
// To do this, use the CubicTrajectory::quadSolve() function in cubic_trajectory.h
double IterativeParabolicSmoother::findT1( const double dq1, 
                                           const double dq2, 
                                           double dt1, 
                                           const double dt2, 
                                           const double a_max) const
{
  const double mult_factor = 1.01;
  double v1 = (dq1)/dt1;
  double v2 = (dq2)/dt2;
  double a = 2*(v2-v1)/(dt1+dt2);

  while( std::abs( a ) > a_max )
  {
    v1 = (dq1)/dt1;
    v2 = (dq2)/dt2;
    a = 2*(v2-v1)/(dt1+dt2);
    dt1 *= mult_factor;
  }

  return dt1;
}

double IterativeParabolicSmoother::findT2(const double dq1, 
                                          const double dq2, 
                                          const double dt1, 
                                          double dt2, 
                                          const double a_max) const
{
  const double mult_factor = 1.01;
  double v1 = (dq1)/dt1;
  double v2 = (dq2)/dt2;
  double a = 2*(v2-v1)/(dt1+dt2);

  while( std::abs( a ) > a_max )
  {
    v1 = (dq1)/dt1;
    v2 = (dq2)/dt2;
    a = 2*(v2-v1)/(dt1+dt2);
    dt2 *= mult_factor;
  }

  return dt2;
}

// Takes the time differences, and updates the values in the trajectory.
void updateTrajectory(trajectory_msgs::JointTrajectory& trajectory, const std::vector<double>& time_diff )
{
  double time_sum = 0.2;
  unsigned int num_joints = trajectory.joint_names.size();
  const unsigned int num_points = trajectory.points.size();

  // Error check
  if(time_diff.size() < 1)
		return;

  // Times
  trajectory.points[0].time_from_start = ros::Duration(.2);
  for (unsigned int i=1; i<num_points; ++i)
  {
    time_sum += time_diff[i-1];
    trajectory.points[i].time_from_start = ros::Duration(time_sum);
  }

  // Return if there is only one point in the trajectory!
  if(trajectory.points.size() <= 1) return;

  // Velocities
/*
  for (unsigned int j=0; j<num_joints; ++j)
  {
    trajectory.points[num_points-1].velocities[j] = 0.0;
  }
  for (unsigned int i=0; i<num_points-1; ++i)
  {
    trajectory_msgs::JointTrajectoryPoint& point1 = trajectory.points[i];
    trajectory_msgs::JointTrajectoryPoint& point2 = trajectory.points[i+1];
    for (unsigned int j=0; j<num_joints; ++j)
    {
      const double dq1 = point1.positions[j];
      const double dq2 = point2.positions[j];
      const double & dt1 = time_diff[i];
      const double v1 = (dq2-dq1)/(dt1);
      point1.velocities[j]= v1;
    }
  }
*/
  // Accelerations
  for (unsigned int i=0; i<num_points; ++i)
  {
    for (unsigned int j=0; j<num_joints; ++j)
    {
      double q1;
      double q2;
      double q3;
      double dt1;
      double dt2;

      if(i==0)
      {	// First point
        q1 = trajectory.points[i+1].positions[j];
        q2 = trajectory.points[i].positions[j];
        q3 = trajectory.points[i+1].positions[j];
        dt1 = time_diff[i];
        dt2 = time_diff[i];
      }
      else if(i < num_points-1)
      { // middle points
        q1 = trajectory.points[i-1].positions[j];
        q2 = trajectory.points[i].positions[j];
        q3 = trajectory.points[i+1].positions[j];
        dt1 = time_diff[i-1];
        dt2 = time_diff[i];
      }
      else
      { // last point
        q1 = trajectory.points[i-1].positions[j];
        q2 = trajectory.points[i].positions[j];
        q3 = trajectory.points[i-1].positions[j];
        dt1 = time_diff[i-1];
        dt2 = time_diff[i-1];
      }

      double v1, v2, a;

      if(dt1 == 0.0 || dt2 == 0.0) {
        v1 = 0.0;
        v2 = 0.0;
        a = 0.0;
      } else {
        v1 = (q2-q1)/dt1;
        v2 = (q3-q2)/dt2;
        a = 2*(v2-v1)/(dt1+dt2);
      }
      trajectory.points[i].velocities[j] = (v2+v1)/2;
      trajectory.points[i].accelerations[j] = a;
    }
  }
}


// Applies Acceleration constraints
void IterativeParabolicSmoother::applyAccelerationConstraints(const trajectory_msgs::JointTrajectory& trajectory, 
                                                              const std::vector<moveit_msgs::JointLimits>& limits,
                                                              std::vector<double> & time_diff) const
{
  const unsigned int num_points = trajectory.points.size();
  const unsigned int num_joints = trajectory.joint_names.size();
  int num_updates = 0;
  int iteration= 0;
  bool backwards = false;
  double q1;
  double q2;
  double q3;
  double dt1;
  double dt2;
  double v1;
  double v2;
  double a;

  do
  {
    num_updates = 0;
    iteration++;

    // In this case we iterate through the joints on the outer loop.
    // This is so that any time interval increases have a chance to get propogated through the trajectory
    for (unsigned int j=0; j<num_joints; ++j)
    {
      // Loop forwards, then backwards
      for( int count=0; count<2; count++)
      {
        ROS_DEBUG("applyAcceleration: Iteration %i backwards=%i joint=%i", iteration, backwards, j);
        //updateTrajectory(trajectory, time_diff);
        //printStats(trajectory);

        for (unsigned int i=0; i<num_points-1; ++i)
        {
          unsigned int index = i;
          if(backwards)
          {
            index = (num_points-1)-i;
          }

          // Get acceleration limits
          double a_max = 1.0;
          if( limits[j].has_acceleration_limits )
          {
            a_max = limits[j].max_acceleration;
          }

          if(index==0)
          {	// First point
            q1 = trajectory.points[index+1].positions[j];
            q2 = trajectory.points[index].positions[j];
            q3 = trajectory.points[index+1].positions[j];
            dt1 = time_diff[index];
            dt2 = time_diff[index];
            assert(!backwards);
          }
          else if(index < num_points-1)
          { // middle points
            q1 = trajectory.points[index-1].positions[j];
            q2 = trajectory.points[index].positions[j];
            q3 = trajectory.points[index+1].positions[j];
            dt1 = time_diff[index-1];
            dt2 = time_diff[index];
          }
          else
          { // last point - careful, there are only numpoints-1 time intervals
            q1 = trajectory.points[index-1].positions[j];
            q2 = trajectory.points[index].positions[j];
            q3 = trajectory.points[index-1].positions[j];
            dt1 = time_diff[index-1];
            dt2 = time_diff[index-1];
            assert(backwards);
          }

          if(dt1 == 0.0 || dt2 == 0.0) {
            v1 = 0.0;
            v2 = 0.0;
            a = 0.0;
          } else {
            v1 = (q2-q1)/dt1;
            v2 = (q3-q2)/dt2;
            a = 2*(v2-v1)/(dt1+dt2);
          }

          if( std::abs( a ) > a_max + ROUNDING_THRESHOLD )
          {
            if(!backwards)
            {
              dt2 = std::min( dt2+max_time_change_per_it_, findT2( q2-q1, q3-q2, dt1, dt2, a_max) );
              time_diff[index] = dt2;
            }
            else
            {
              dt1 = std::min( dt1+max_time_change_per_it_, findT1( q2-q1, q3-q2, dt1, dt2, a_max) );
              time_diff[index-1] = dt1;
            }
            num_updates++;

            if(dt1 == 0.0 || dt2 == 0.0) {
              v1 = 0.0;
              v2 = 0.0;
              a = 0.0;
            } else {
              v1 = (q2-q1)/dt1;
              v2 = (q3-q2)/dt2;
              a = 2*(v2-v1)/(dt1+dt2);
            }
          }
        }
        backwards = !backwards;
      }
    }
    ROS_DEBUG("applyAcceleration: num_updates=%i", num_updates);
  } while(num_updates > 0 && iteration < max_iterations_);
}

bool IterativeParabolicSmoother::smooth(const trajectory_msgs::JointTrajectory& trajectory_in,
                                        trajectory_msgs::JointTrajectory& trajectory_out,
                                        const std::vector<moveit_msgs::JointLimits>& limits) const
{
  bool success = true;
  trajectory_out = trajectory_in;	//copy
  const unsigned int num_points = trajectory_out.points.size();
  std::vector<double> time_diff(num_points,0.0);	// the time difference between adjacent points

  applyVelocityConstraints(trajectory_out, limits, time_diff);
  applyAccelerationConstraints(trajectory_out, limits, time_diff);

  ROS_DEBUG("Velocity & Acceleration-Constrained Trajectory");
  updateTrajectory(trajectory_out, time_diff);
  printStats(trajectory_out, limits);

  return success;
}

}
