/* +---------------------------------------------------------------------------+
   |          The Mobile Robot Programming Toolkit (MRPT) C++ library          |
   |                                                                           |
   |                   http://mrpt.sourceforge.net/                            |
   |                                                                           |
   |   Copyright (C) 2005-2010  University of Malaga                           |
   |                                                                           |
   |    This software was written by the Machine Perception and Intelligent    |
   |      Robotics Lab, University of Malaga (Spain).                          |
   |    Contact: Jose-Luis Blanco  <jlblanco@ctima.uma.es>                     |
   |                                                                           |
   |  This file is part of the MRPT project.                                   |
   |                                                                           |
   |     MRPT is free software: you can redistribute it and/or modify          |
   |     it under the terms of the GNU General Public License as published by  |
   |     the Free Software Foundation, either version 3 of the License, or     |
   |     (at your option) any later version.                                   |
   |                                                                           |
   |   MRPT is distributed in the hope that it will be useful,                 |
   |     but WITHOUT ANY WARRANTY; without even the implied warranty of        |
   |     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         |
   |     GNU General Public License for more details.                          |
   |                                                                           |
   |     You should have received a copy of the GNU General Public License     |
   |     along with MRPT.  If not, see <http://www.gnu.org/licenses/>.         |
   |                                                                           |
   +---------------------------------------------------------------------------+ */
#ifndef mrpt_CKinect_H
#define mrpt_CKinect_H

#include <mrpt/hwdrivers/CGenericSensor.h>
#include <mrpt/slam/CObservation3DRangeScan.h>
#include <mrpt/slam/CObservationIMU.h>

#include <mrpt/gui/CDisplayWindow.h>

#include <mrpt/hwdrivers/link_pragmas.h>

// MRPT implements a common interface to Kinect disregarding the
//  actual underlying library. These macros are defined for internal
//  MRPT usage to know which library to use:
#if MRPT_HAS_KINECT
#	if defined(_MSC_VER)
#		define MRPT_KINECT_WITH_CLNUI        1
#		define MRPT_KINECT_WITH_LIBFREENECT  0
#	else
#		define MRPT_KINECT_WITH_CLNUI        0
#		define MRPT_KINECT_WITH_LIBFREENECT  1
#	endif
#else
#		define MRPT_KINECT_WITH_CLNUI        0
#		define MRPT_KINECT_WITH_LIBFREENECT  0
#endif

// Depth of Kinect ranges:
#if MRPT_KINECT_WITH_LIBFREENECT
#	define MRPT_KINECT_DEPTH_10BIT
#	define KINECT_RANGES_TABLE_LEN    1024
#	define KINECT_RANGES_TABLE_MASK   0x03FF
#elif MRPT_KINECT_WITH_CLNUI
#	define MRPT_KINECT_DEPTH_11BIT
#	define KINECT_RANGES_TABLE_LEN    2048
#	define KINECT_RANGES_TABLE_MASK   0x07FF
#endif


namespace mrpt
{
	namespace hwdrivers
	{
		/** A class for grabing "range images", intensity images and other information from an Xbox Kinect sensor.
		  *
		  *  <h2>Configuration and usage:</h2> <hr>
		  * Data is returned as observations of type mrpt::slam::CObservation3DRangeScan (and mrpt::slam::CObservationIMU for accelerometers data). 
		  *  See those classes for documentation on their fields.
		  *
		  * As with any other CGenericSensor class, the normal sequence of methods to be called is:
		  *   - CGenericSensor::loadConfig() - Or calls to the individual setXXX() to configure the sensor parameters.
		  *   - CKinect::initialize() - to start the communication with the sensor.
		  *   - call CKinect::getNextObservation() for getting the data.
		  *
		  * <h2>Calibration parameters</h2><hr>
		  *  For an accurate transformation of depth images to 3D points, you'll have to calibrate your Kinect, and supply
		  *   the following <b>threee pieces of information</b> (default calibration data will be used otherwise, but they'll be not optimal for all sensors!):
		  *    - Camera parameters for the RGB camera. See CKinect::setCameraParamsIntensity()
		  *    - Camera parameters for the depth camera. See CKinect::setCameraParamsDepth()
		  *    - The 3D relative pose of the two cameras. See CKinect::setRelativePoseIntensityWrtDepth()
		  *
		  * <h2>Some general comments</h2><hr>
		  *		- Depth is grabbed in 10bit depth, and a range N it's converted to meters as: range(m) = 0.1236 * tan(N/2842.5 + 1.1863)
		  *		- This sensor can be also used from within rawlog-grabber to grab datasets within a robot with more sensors.
		  *		- There is no built-in threading support, so if you use this class manually (not with-in rawlog-grabber),
		  *			the ideal would be to create a thread and continuously request data from that thread (see mrpt::system::createThread ).
		  *		- There is a built-in support for an optional preview of the data on a window, so you don't need to even worry on creating a window to show them.
		  *		- This class relies on an embedded version of libfreenect (you don't have to install it in your system). Thanks guys for the great job!
		  *
		  * <h2>Converting to 3D point cloud </h2><hr>
		  *   You can convert the 3D observation into a 3D point cloud with this piece of code:
		  *
		  * \code
		  * mrpt::slam::CObservation3DRangeScan  obs3D;
		  * mrpt::slam::CColouredPointsMap       pntsMap;
		  * pntsMap.colorScheme.scheme = CColouredPointsMap::cmFromIntensityImage;
		  * pntsMap.loadFromRangeScan(obs3D);
		  * \endcode
		  *
		  *   Then the point cloud mrpt::slam::CColouredPointsMap can be converted into an OpenGL object for
		  *    rendering with mrpt::slam::CMetricMap::getAs3DObject() or alternatively with:
		  *
		  *  \code
		  *    mrpt::opengl::CPointCloudColouredPtr gl_points = mrpt::opengl::CPointCloudColoured::Create();
		  *    gl_points->loadFromPointsMap(&pntsMap);
		  *  \endcode
		  *
		  *
		  * <h2>Raw depth to range conversion</h2><hr>
		  *  At construction, this class builds an internal array for converting raw 10 or 11bit depths into ranges in meters.
		  *   Users can read that array or modify it (if you have a better calibration, for example) by calling CKinect::getRawDepth2RangeConversion().
		  *   If you replace it, remember to set the first and last entries (index 0 and KINECT_RANGES_TABLE_LEN-1) to zero, to indicate that those are invalid ranges.
		  *
		  *  <table width="100%" >
		  *  <tr>
		  *  <td align="center" >
		  *   <img src="kinect_depth2range_10bit.png" > <br>
		  *    R(d) = k3 * tan(d/k2 + k1); <br>
		  *    k1 = 1.1863,  k2 = 2842.5, k3 = 0.1236 <br>
		  *  </td>
		  *  <td align="center" >
		  *  </td>
		  *  </tr>
		  *  </table>
		  *
		  *
		  * <h2>Platform-specific comments</h2><hr>
		  *   For more details, refer to <a href="http://openkinect.org/wiki/Main_Page" >libfreenect</a> documentation:
		  *		- Linux: You'll need root privileges to access Kinect. Or, install <code> MRPT/scripts/51-kinect.rules </code> in <code>/etc/udev/rules.d/</code> to allow access to all users.
		  *		- Windows: Install CL NUI SDK: http://codelaboratories.com/nui
		  *		- MacOS: (write me!)
		  *
		  *
		  * <h2>Format of parameters for loading from a .ini file</h2><hr>
		  *
		  *  \code
		  *  PARAMETERS IN THE ".INI"-LIKE CONFIGURATION STRINGS:
		  * -------------------------------------------------------
		  *   [supplied_section_name]
		  *    sensorLabel     = KINECT       // A text description
		  *    preview_window  = false        // Show a window with a preview of the grabbed data in real-time
		  *
		  *    device_number   = 0           // Device index to open (0:first Kinect, 1:second Kinect,...)
		  *    
		  *    grab_image      = true        // Grab the RGB image channel? (Default=true)
		  *    grab_depth      = true        // Grab the depth channel? (Default=true)
		  *    grab_3D_points  = true        // Grab the 3D point cloud? (Default=true) If disabled, points can be generated later on.
		  *    grab_IMU        = true        // Grab the accelerometers? (Default=true)
		  *
		  *    // Calibration matrix of the RGB camera:
		  *    rgb_cx        = 328.9427     // (cx,cy): Optical center, pixels
		  *    rgb_cy        = 267.4807
		  *    rgb_fx        = 529.2151     // (fx,fy): Focal distance, pixels
		  *    rgb_fy        = 525.5639
		  *
		  *    // Calibration matrix of the Depth camera:
		  *    d_cx          = 339.3078     // (cx,cy): Optical center, pixels
		  *    d_cy          = 242.7391
		  *    d_fx          = 594.2143     // (fx,fy): Focal distance, pixels
		  *    d_fy          = 591.0405
		  *
		  *    // The relative pose of the RGB camera wrt the depth camera.
		  *    //  (See mrpt::slam::CObservation3DRangeScan for a 3D diagram of this pose)
		  *    relativePoseIntensityWRTDepth  =  [0 -0.02 0 -90 0 -90]   //  [x(m) y(m) z(m) yaw(deg) pitch(deg) roll(deg)]
		  *
		  *    pose_x=0	// Camera position in the robot (meters)
		  *    pose_y=0
		  *    pose_z=0
		  *    pose_yaw=0	// Angles in degrees
		  *    pose_pitch=0
		  *    pose_roll=0
		  *
		  *  \endcode
		  *
		  *  More references to read:
		  *		- http://openkinect.org/wiki/Imaging_Information
		  *		- http://nicolas.burrus.name/index.php/Research/KinectCalibration
		  */
		class HWDRIVERS_IMPEXP  CKinect : public mrpt::hwdrivers::CGenericSensor
		{
			DEFINE_GENERIC_SENSOR(CKinect)

		public:
			typedef float TDepth2RangeArray[KINECT_RANGES_TABLE_LEN]; //!< A typedef for an array that converts raw depth to ranges in meters.


			CKinect();	 //!< Default ctor
			~CKinect();	 //!< Default ctor

			/** Initializes the 3D camera - should be invoked after calling loadConfig() or setting the different parameters with the set*() methods.
			  *  \exception This method must throw an exception with a descriptive message if some critical error is found.
			  */
			virtual void initialize();

			/** To be called  at a high rate (>XX Hz), this method populates the internal buffer of received observations.
			  *  This method is mainly intended for usage within rawlog-grabber or similar programs.
			  *  For an alternative, see getNextObservation()
			  *  \exception This method must throw an exception with a descriptive message if some critical error is found.
			  * \sa getNextObservation
			  */
			virtual void doProcess();

			/** The main data retrieving function, to be called after calling loadConfig() and initialize().
			  *  \param out_obs The output retrieved observation (only if there_is_obs=true).
			  *  \param there_is_obs If set to false, there was no new observation.
			  *  \param hardware_error True on hardware/comms error.
			  *
			  * \sa doProcess
			  */
			void getNextObservation( 
				mrpt::slam::CObservation3DRangeScan &out_obs, 
				bool &there_is_obs, 
				bool &hardware_error );

			/** \overload 
			  * \note This method also grabs data from the accelerometers, returning them in out_obs_imu
			  */
			void getNextObservation(
				mrpt::slam::CObservation3DRangeScan &out_obs, 
				mrpt::slam::CObservationIMU         &out_obs_imu,
				bool &there_is_obs, 
				bool &hardware_error );

			/**  Set the path where to save off-rawlog image files (this class DOES take into account this path).
			  *  An  empty string (the default value at construction) means to save images embedded in the rawlog, instead of on separate files.
			  * \exception std::exception If the directory doesn't exists and cannot be created.
			  */
			virtual void setPathForExternalImages( const std::string &directory );


			/** @name Sensor parameters (alternative to \a loadConfig ) and manual control
			    @{ */

			/** Try to open the camera (set all the parameters first!) - users may also call initialize(), which in turn calls this.
			  *  Raises an exception upon error.
			  * \exception std::exception A textual description of the error.
			  */
			void open();

			bool isOpen() const; //!< Whether there is a working connection to the sensor

			/** Close the conection to the sensor (not need to call it manually unless desired for some reason,
			  * since it's called at destructor) */
			void close();

			/** Set the sensor index to open (if there're several sensors attached to the computer); default=0 -> the first one. */
			inline void setDeviceIndexToOpen(int index) { m_user_device_number=index; }
			inline int getDeviceIndexToOpen() const { return m_user_device_number; }

			/** Change tilt angle \note Sensor must be open first. */
			void setTiltAngleDegrees(double angle);
			double getTiltAngleDegrees();

			/** Default: disabled */
			inline void enablePreviewRGB(bool enable=true) { m_preview_window = enable; }
			inline void disablePreviewRGB() { m_preview_window = false; }
			inline bool isPreviewRGBEnabled() const { return m_preview_window; }

			/** If preview is enabled, show only one image out of N (default: 1=show all) */
			inline void setPreviewDecimation(size_t decimation_factor ) { m_preview_window_decimation = decimation_factor; }
			inline size_t getPreviewDecimation() const { return m_preview_window_decimation; }

			/** Get the maximum range (meters) that can be read in the observation field "rangeImage" */
			inline double getMaxRange() const { return m_maxRange; }

			/** Get the row count in the camera images, loaded automatically upon camera open(). */
			inline size_t getRowCount() const { return m_cameraParamsRGB.nrows; }
			/** Get the col count in the camera images, loaded automatically upon camera open(). */
			inline size_t getColCount() const { return m_cameraParamsRGB.ncols; }

			/** Get a const reference to the depth camera calibration parameters */
			inline const mrpt::utils::TCamera  & getCameraParamsIntensity() const { return m_cameraParamsRGB; }
			inline void setCameraParamsIntensity(const mrpt::utils::TCamera  &p) { m_cameraParamsRGB=p; }

			/** Get a const reference to the depth camera calibration parameters */
			inline const mrpt::utils::TCamera  & getCameraParamsDepth() const { return m_cameraParamsDepth; }
			inline void setCameraParamsDepth(const mrpt::utils::TCamera  &p) { m_cameraParamsDepth=p; }

			/** Set the pose of the intensity camera wrt the depth camera \sa See mrpt::slam::CObservation3DRangeScan for a 3D diagram of this pose */
			inline void setRelativePoseIntensityWrtDepth(const mrpt::poses::CPose3D &p) { m_relativePoseIntensityWRTDepth=p; }
			inline const mrpt::poses::CPose3D &getRelativePoseIntensityWrtDepth() const { return m_relativePoseIntensityWRTDepth; }

			/** Get a reference to the array that convert raw depth values (10 or 11 bit) into ranges in meters, so it can be read or replaced by the user.
			  *  If you replace it, remember to set the first and last entries (index 0 and KINECT_RANGES_TABLE_LEN-1) to zero, to indicate that those are invalid ranges.
			  */
			inline       TDepth2RangeArray & getRawDepth2RangeConversion()       { return m_range2meters; }
			inline const TDepth2RangeArray & getRawDepth2RangeConversion() const { return m_range2meters; }

			/** Enable/disable the grabbing of the RGB channel */
			inline void enableGrabRGB(bool enable=true) { m_grab_image=enable; }
			inline bool isGrabRGBEnabled() const { return m_grab_image; }

			/** Enable/disable the grabbing of the depth channel */
			inline void enableGrabDepth(bool enable=true) { m_grab_depth=enable; }
			inline bool isGrabDepthEnabled() const { return m_grab_depth; }

			/** Enable/disable the grabbing of the inertial data */
			inline void enableGrabAccelerometers(bool enable=true) { m_grab_IMU=enable; }
			inline bool isGrabAccelerometersEnabled() const { return m_grab_IMU; }

			/** Enable/disable the grabbing of the 3D point clouds */
			inline void enableGrab3DPoints(bool enable=true) { m_grab_3D_points=enable; }
			inline bool isGrab3DPointsEnabled() const { return m_grab_3D_points; }

			/** @} */

		protected:
			/** Loads specific configuration for the device from a given source of configuration parameters, for example, an ".ini" file, loading from the section "[iniSection]" (see utils::CConfigFileBase and derived classes)
			  *  \exception This method must throw an exception with a descriptive message if some critical parameter is missing or has an invalid value.
			  */
			virtual void  loadConfig_sensorSpecific(
				const mrpt::utils::CConfigFileBase &configSource,
				const std::string			&section );

			mrpt::poses::CPose3D 	m_sensorPoseOnRobot;

			bool		m_preview_window; //!< Show preview window while grabbing
			size_t 		m_preview_window_decimation; //!< If preview is enabled, only show 1 out of N images.
			size_t      m_preview_decim_counter_range, m_preview_decim_counter_rgb;
			mrpt::gui::CDisplayWindowPtr  m_win_range, m_win_int;

#if MRPT_KINECT_WITH_LIBFREENECT
			void *m_f_ctx;  //!< The "freenect_context", or NULL if closed
			void *m_f_dev;  //!< The "freenect_device", or NULL if closed
#endif

#if MRPT_KINECT_WITH_CLNUI
			void *m_clnui_cam;   //!< The "CLNUICamera" or NULL if closed
			void *m_clnui_motor; //!< The "CLNUIMotor" or NULL if closed
#endif

			mrpt::utils::TCamera  	m_cameraParamsRGB;  //!< Params for the RGB camera
			mrpt::utils::TCamera  	m_cameraParamsDepth;  //!< Params for the Depth camera
			mrpt::poses::CPose3D    m_relativePoseIntensityWRTDepth; //!< See mrpt::slam::CObservation3DRangeScan for a diagram of this pose

			double  m_maxRange; //!< Sensor max range (meters)

			int  m_user_device_number; //!< Number of device to open (0:first,...)

			bool  m_grab_image, m_grab_depth, m_grab_3D_points, m_grab_IMU ; //!< Default: all true

		private:
			std::vector<uint8_t> m_buf_depth, m_buf_rgb; //!< Temporary buffers for image grabbing.
			TDepth2RangeArray   m_range2meters; //!< The table raw depth -> range in meters

			void calculate_range2meters(); //!< Compute m_range2meters at construction

		};	// End of class

	} // End of NS
} // End of NS


#endif