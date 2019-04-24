#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <cmath>
#include <iostream>
#include <time.h>

#define PI 3.14159265358979323846264338327950288
using namespace cv;
using namespace std;

 
/// Global variables

Mat src, src_gray, acc;
Mat dst, edges;

// Struct to store center points and pixel intensities
struct cent
{
	int bright;
	int Xbright;
	int Ybright;
};

char* window_edges = "Edges";
char* window_circles = "Detected Circles";
char* window_accumulator = "Accumulator";

// Vector to print circles
vector<Vec3f> circles;
int Xc, Yc;

// Maximum and minimum radius values
const int maxradius = 85;
const int minradius = 20;
int radius = maxradius;

// Array of structs to store center points
cent accumul[maxradius];

// Function to manipulate threshold parameters and perform Canny edge detection

void CannyThreshold(int, void*)
{
	
	int edgeThresh = 1;
	double lowThreshold = 91;
	int const max_lowThreshold = 200;
	int ratio = 3;
	int kernel_size = 3;

	// Average filter to reduce noise
	blur( src_gray, edges, Size(3,3) );

	// Canny edge detector
	Canny( edges, edges, lowThreshold, lowThreshold*ratio, kernel_size);
	
	// Copying edges to plain image with black background
	dst = Scalar::all(0);
	edges.copyTo( dst);

	// Print detected edges
	namedWindow(window_edges, CV_WINDOW_AUTOSIZE );
	imshow( window_edges, dst );
 }

void Accumulate(int, void*)
{
	
	// Accumulator created using Circular Hough Transform

	for(int i = 0;i < acc.rows;i++)
	{
		for(int j = 0;j < acc.cols;j++)
		{
			if (dst.at<uchar>(i,j) > 256/2)
			{
				for (int theta = 0; theta <= 360; theta++)			// Calculating center points for each edge point
				{
					Xc = i + (radius * cos(theta*PI/180));
					Yc = j + (radius * sin(theta*PI/180));

					if(Xc < acc.rows && Yc < acc.cols && Xc > 0 && Yc > 0)
					{
						acc.at<uchar>(Xc,Yc) = (int)acc.at<uchar>(Xc,Yc) + 1;	// Incrementing/Accumulating center point values
					}
				}
				
			}

		}
		
	}

	int bright = 0;						// Highest pixel intensity value
	int Xbright = 0;					// X-index of highest pixel intensity value
	int Ybright = 0;					// Y-index of highest pixel intensity value

	int sum = 0;						// Sum of differences between candidate center point and its neighbors + candidate center point intensity value
	int bigsum = 0;						// Largest sum of differences between candidate center point and its neighbors + candidate center point intensity value
	int brightdist = 4;					// Maximum pixel distance from candidate center point to neighbors
	
	for(int i = 0; i < acc.rows; i++)
	{
		for(int j = 0; j < acc.cols; j++)
		{
			
			for(int m = i - brightdist; m < i + brightdist; m++)
			{
				for(int n = j - brightdist; n < j + brightdist; n++)
				{
					if ((m>=0) && (m<acc.rows) && (n>=0) && (n<acc.cols))
					{
						sum += abs((int)acc.at<uchar>(i,j) - (int)acc.at<uchar>(m,n));	// Adding absolute difference between candidate center point and its neighbor to sum
					}
				}

			}
			
			sum += (int)acc.at<uchar>(i,j);						// Adding candidate center point's intensity value to sum

			if(sum > bigsum)									// Finding largest sum
			{
				accumul[radius].bright = acc.at<uchar>(i,j);	// Storing corresponding center point in array of structs
				accumul[radius].Xbright = i;
				accumul[radius].Ybright = j;
				bright = acc.at<uchar>(i,j);					// Storing corresponding center point in variables for quicker access and readability
				Xbright = i;
				Ybright = j;
				bigsum = sum;									// Updating largest sum of differences
			}
			sum = 0;											// Resetting sum of differences
		}
	}
	cout<<"Center: "<<"["<<Xbright<<","<<Ybright<<"]"<<"	Votes: "<<bright<<"		Radius:"<<radius<<endl;	// Printing accumulator center point for this radius
	
	double dist;											// Distance between two points
	double radco = 0.75;									// Radial coefficient (used to calculate minimum distance between centers)
	bool valid = true;										// Valid/Invalid center point

	Point center(Ybright, Xbright);							

	for (int i = maxradius; i > radius ; i--)				// Calculating distance between candidate center point and previously found center points
	{
		dist = sqrt(pow((double(accumul[radius].Xbright - accumul[i].Xbright)), 2) + pow((double(accumul[radius].Ybright - accumul[i].Ybright)), 2));
		if (dist < radco*i)									// If distance between candidate center point and any previous center point is less than the radial coefficient multiplied by any of the previously found radii, it is not a valid center point
		{
			valid = false;									// Set valid to false
		}

	}

	if (valid == true)										// If the center point is valid, draw the corresponding circle
	{
		
		circle( src, center, 3, Scalar(0,255,0), -1, 8, 0 );		// Draw circle center point
		circle( src, center, radius, Scalar(0,0,255), 3, 8, 0 );	// Draw circle outline
	}
	
	
	namedWindow( window_circles, CV_WINDOW_AUTOSIZE );				// Show original image with overlayed circles
	imshow( window_circles, src );

	namedWindow( window_accumulator, CV_WINDOW_AUTOSIZE );			// Show accumulator
	imshow( window_accumulator, acc );

	acc = Scalar::all(0);											// Reset accumulator to zero
}



int main( int argc, char** argv )
{
  // Load an image
  src = imread( argv[1]);

  if( !src.data )
  { return -1; }
  	
  // Create a matrix of the same type and size as src (for dst)
  dst.create( src.size(), src.type() );

  /// Convert the image to grayscale
  cvtColor( src, src_gray, CV_BGR2GRAY );
  
  /// Create a window 
  namedWindow( window_circles, CV_WINDOW_AUTOSIZE );

  CannyThreshold(0, 0);															// Detecting edges with Canny edge detector

  acc.create(src_gray.size(), src_gray.type());									// Creating an accumulator
  acc = Scalar::all(0);															// Setting accumulator to a full black image

  for(radius = maxradius; radius > minradius; radius--)							// Varying radial values
  { 
	  createTrackbar( "Radius", window_circles, &radius, 100, Accumulate );		// Creating a trackbar to show circle detection for various radial values
	  Accumulate(0,0);															// Creating an accumulator for every radial value
	  waitKey(1);																// Causing a delay to allow image in trackbar to render
  }
		
		
  waitKey(0);
  return 0;
 }