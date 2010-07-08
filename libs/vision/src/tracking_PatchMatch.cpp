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

#include <mrpt/vision.h>  // Precompiled headers

#include <mrpt/vision/tracking.h>
#include <mrpt/vision/CFeatureExtraction.h>

#include "do_opencv_includes.h"

using namespace mrpt;
using namespace mrpt::vision;
using namespace mrpt::utils;
using namespace mrpt::math;
using namespace std;

/** Ctor  */
CFeatureTracker_PatchMatch::CFeatureTracker_PatchMatch(const mrpt::utils::TParametersDouble &extraParams) :
	CGenericFeatureTracker			( extraParams )
{
}


/** Track a set of features from old_img -> new_img by patch matching over a fixed window centered at each feature's previous location.
*  Optional parameters that can be passed in "extra_params":
*		- "window_width"  (Default=15)
*		- "window_height" (Default=15)
*		- "match_method" (Default=0)
*
*  Possible values for "match_method":
*		- 0 : Normalized cross correlation
*/
void CFeatureTracker_PatchMatch::trackFeatures(
	const CImage &old_img,
	const CImage &new_img,
	vision::CFeatureList &featureList )
{
	MRPT_START
#if MRPT_HAS_OPENCV
#if MRPT_OPENCV_VERSION_NUM >= 0x211

	const unsigned int window_width = extra_params.getWithDefaultVal("window_width",15);
	const unsigned int window_height = extra_params.getWithDefaultVal("window_height",15);

	// Create a temporary gray image, if needed:
	mrpt::utils::CImage  new_img_gray;
	if (new_img.isColor())
			new_img.grayscale(new_img_gray);	// Create a new auxiliary grayscale image
	else	new_img_gray.setFromIplImageReadOnly( new_img.getAsIplImage() );  // Copy the IPLImage, but do not own the memory

	const size_t  new_img_width = new_img_gray.getWidth();
	const size_t  new_img_height = new_img_gray.getHeight();

	const size_t  patch_size = (!featureList.empty() && featureList[0]) ? featureList[0]->patch.getWidth() : 0;

	using namespace cv;

	// =======================================================================
	//                   OVERVIEW OF THE TRACKING ALGORITHM
	//
	// 1) For each existing feature from the last image, do:
	//     1.a) Compute the patch matching in a window around it
	//     1.b) If the response is good enough at the maximum, update the
	//          feature position. Otherwise, mark as "feature lost".
	// =======================================================================

	// For each old feature:
	for (size_t i=0;i<featureList.size();i++)
	{
		ASSERTDEB_(featureList[i].present())
		CFeaturePtr &feat = featureList[i];

		// Get the size of the patch so we know when we are too close to a border.
		ASSERTDEB_(feat->patch.getWidth()>1)
		ASSERTDEB_(feat->patch.getWidth()==feat->patch.getHeight())

		int x_search_ini = feat->x - window_width;
		int	y_search_ini = feat->y - window_height;
		int x_search_size = window_width*2;
		int	y_search_size = window_height*2;

		// Compute patch match:
		size_t  best_x, best_y;
		double  best_match;

//		mrpt::vision::openCV_cross_correlation(
//			new_img_gray,
//			feat->patch,
//			best_x, best_y,best_match,   // Output
//			x_search_ini, y_search_ini,
//			x_search_size,y_search_size);

		if ((x_search_ini + x_search_size  + patch_size-1)>new_img_width)
			x_search_size -= (x_search_ini + x_search_size + patch_size-1) - new_img_width;

		if ((y_search_ini + y_search_size  + patch_size-1)>new_img_height)
			y_search_size -= (y_search_ini + y_search_size  + patch_size-1) - new_img_height;

		ASSERT_( (x_search_ini + x_search_size  + patch_size-1)<=new_img_width )
		ASSERT_( (y_search_ini + y_search_size  + patch_size-1)<=new_img_height )

		IplImage *result = cvCreateImage(cvSize(x_search_size+1,y_search_size+1),IPL_DEPTH_32F, 1);

		CImage  img_region_to_search;
		new_img_gray.extract_patch(
			img_region_to_search,
			x_search_ini,   // start corner
			y_search_ini ,
			patch_size+x_search_size,  // sub-image size
			patch_size+y_search_size
			);

		// Compute cross correlation:
		cvMatchTemplate(
			img_region_to_search.getAsIplImage(),
			feat->patch.getAsIplImage(),
			result,
			CV_TM_CCORR_NORMED
			//CV_TM_CCOEFF_NORMED
			);

		// Find the max point:
		{
			double		mini;
			CvPoint		min_point,max_point;
			cvMinMaxLoc(result,&mini,&best_match,&min_point,&max_point,NULL);
			best_x = max_point.x+x_search_ini+(round(patch_size-1)>>1);
			best_y = max_point.y+y_search_ini+(round(patch_size-1)>>1);
		}

		//cout << "match: " << best_match << endl;

		if (best_match>0.90)
		{
			// Aditional checks:
			// A good, isolated match in its neighbourhood?

			// ...

			// OK: Accept it:
			feat->track_status	= status_TRACKED;
			feat->x				= best_x;
			feat->y				= best_y;
		}
		else
		{
			// No potential match!
			// Mark as "lost":
			feat->track_status = status_LOST;
		}

		// Free memory:
		cvReleaseImage( &result );

	} // end for each prev feat.

#else
	THROW_EXCEPTION("This function requires OpenCV >= 2.1.1")
#endif
#else
	THROW_EXCEPTION("MRPT has been compiled without OpenCV!")
#endif
	MRPT_END
}


