/*
 *  ocv_freenect.cpp
 *  OpenCVTries1
 *
 *  Created by Roy Shilkrot on 11/19/10.
 *
 */

#define LIBUSB_DEBUG 5

#include "ocv_freenect.h"

#include <math.h>

pthread_t ocv_thread;
volatile int die = 0;

int g_argc;
char **g_argv;

int window;

pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

//Mat depthMat(Size(640,480),CV_16UC1),
//	rgbMat(Size(640,480),CV_8UC3,Scalar(0));


freenect_device *f_dev;
int freenect_angle = 0;
int freenect_led;

pthread_cond_t frame_cond = PTHREAD_COND_INITIALIZER;
int got_frames = 0;

uint16_t t_gamma[2048];
freenect_context *f_ctx;

void *freenect_threadfunc(void* arg);
//	void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp);
//	void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp);
void send_event(const string& etype, const string& edata);

void* freenect_threadfunc(void* arg) {	//all this thread does is to fetch events from freenect
	cout << "freenect thread"<<endl;
	while(!die && freenect_process_events(f_ctx) >= 0 ) {}
	cout << "freenect die"<<endl;
	return NULL;
}

void depth_cb(freenect_device *dev, void *depth, uint32_t timestamp)
{
	pthread_mutex_lock(&buf_mutex);

	//copy to ocv buf...
//	memcpy(depthMat.data, depth, FREENECT_DEPTH_SIZE);

	got_frames++;
	pthread_cond_signal(&frame_cond);
	pthread_mutex_unlock(&buf_mutex);
}

void rgb_cb(freenect_device *dev, freenect_pixel *rgb, uint32_t timestamp)
{
	pthread_mutex_lock(&buf_mutex);
	got_frames++;
	//copy to ocv_buf..
//	memcpy(rgbMat.data, rgb, FREENECT_RGB_SIZE);

	pthread_cond_signal(&frame_cond);
	pthread_mutex_unlock(&buf_mutex);
}

//functions defined in the other file...
//extern  Scalar refineSegments(const Mat& img,
//							  Mat& mask,
//							  Mat& dst,
//							  vector<Point>& contour,
//							  Point2i& previous);
//extern void makePointsFromMask(Mat& maskm,vector<Point2f>& points, bool _add = false);
//extern void drawPoint(Mat& out,vector<Point2f>& points,Scalar color, Mat* maskm = NULL);

void send_event(const string& etype, const string& edata) {
//	s_sendmore (socket, "event");
	stringstream ss;
	ss << "{\"type\":\"" << etype << "\",\"data\":{" << edata << "}}";
//	s_send (socket, ss.str());
}


int initFreenect() {
	int res = 0;
	
	//setup Freenect...
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("freenect_init() failed\n");
		return 1;
	}
	
	freenect_set_log_level(f_ctx, FREENECT_LOG_ERROR);
	
	int nr_devices = freenect_num_devices (f_ctx);
	printf ("Number of devices found: %d\n", nr_devices);
	
	int user_device_number = 0;
//	if (argc > 1)
//		user_device_number = atoi(argv[1]);
//	
//	if (nr_devices < 1)
//		return 1;
	
	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("Could not open device\n");
		return 1;
	}
	
	freenect_set_tilt_degs(f_dev,freenect_angle);
	freenect_set_led(f_dev,LED_RED);
	freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_rgb_callback(f_dev, rgb_cb);
	freenect_set_rgb_format(f_dev, FREENECT_FORMAT_RGB);
	freenect_set_depth_format(f_dev, FREENECT_FORMAT_11_BIT);
	
	freenect_start_depth(f_dev);
	freenect_start_rgb(f_dev);
	
	//start the freenect thread to poll for events
//	res = pthread_create(&ocv_thread, NULL, freenect_threadfunc, NULL);
//	if (res) {
//		printf("pthread_create failed\n");
//		return 1;
//	}
	return 0;
}	

//void send_image(const Mat& img) {
////	s_sendmore(socket, "image");
//	
//	Mat _img;
//	if(img.type() == CV_8UC1) {
//		Mat _tmp; resize(img, _tmp,Size(160,120)); //better to resize the gray data and not RGB
//		cvtColor(_tmp, _img, CV_GRAY2RGB);
//	} else {
//		resize(img, _img, Size(160,120));
//	}
//	
////	s_send(socket, (const char*)_img.data);
//}

//Calculating the laplacian of a 2D curve. Thanks Y.Gingold!
//Mat laplacian_mtx(int N, bool closed_poly) {
//	Mat A = Mat::zeros(N, N, CV_64FC1);
//	Mat d = Mat::zeros(N, 1, CV_64FC1);
//    
//    //## endpoints
//	//if(closed_poly) {
//	A.at<double>(0,1) = 1;
//	d.at<double>(0,0) = 1;
//	
//	A.at<double>(N-1,N-2) = 1;
//	d.at<double>(N-1,0) = 1;
//	//} else {
//	//      A.at<double>(0,1) = 1;
//	//      d.at<double>(0,0) = 1;
//	//}
//    
//    //## interior points
//	for(int i = 1; i <= N-2; i++) {
//        A.at<double>(i, i-1) = 1;
//        A.at<double>(i, i+1) = 1;
//        
//        d.at<double>(i,0) = 0.5;
//	}
//    
//	Mat Dinv = Mat::diag( d );
//    
//	return Mat::eye(N,N,CV_64FC1) - Dinv * A;
//}
//
//void calc_laplacian(Mat& X, Mat& Xlap) {
//	static Mat lapX = laplacian_mtx(X.rows,false);
//		//a feeble attempt to save up in memory allocation.. in 99.9% of the cases this if fires
//	if(lapX.rows != X.rows) lapX = laplacian_mtx(X.rows,false); 
//	
//	Mat _X;	//handle non-64UC2 matrices
//	if (X.type() != CV_64FC2) {
//		X.convertTo(_X, CV_64FC2);
//	} else {
//		_X = X;
//	}
//	
//	vector<Mat> v; split(_X,v);
//	v[0] = v[0].t() * lapX.t();
//	v[1] = v[1].t() * lapX.t();
//	cv::merge(v,Xlap);
//	
//	Xlap = Xlap.t();
//}


/*
int ocv_main(int argc, char **argv)
{
	int res;

//	try {
//		socket.bind ("tcp://*:14444");
//		s_sendmore (socket, "event");
//        s_send (socket, "{type:\"up\"}");
//	}
//	catch (zmq::error_t e) {
//		cerr << "Cannot bind to socket: " <<e.what() << endl;
//		return -1;
//	}

	printf("Kinect camera test\n");

	int i;
	for (i=0; i<2048; i++) {
		float v = i/2048.0;
		v = powf(v, 3)* 6;
		t_gamma[i] = v*6*256;
	}

	g_argc = argc;
	g_argv = argv;

	//setup Freenect...
	if (freenect_init(&f_ctx, NULL) < 0) {
		printf("freenect_init() failed\n");
		return 1;
	}

	freenect_set_log_level(f_ctx, FREENECT_LOG_ERROR);

	int nr_devices = freenect_num_devices (f_ctx);
	printf ("Number of devices found: %d\n", nr_devices);

	int user_device_number = 0;
	if (argc > 1)
		user_device_number = atoi(argv[1]);

	if (nr_devices < 1)
		return 1;

	if (freenect_open_device(f_ctx, &f_dev, user_device_number) < 0) {
		printf("Could not open device\n");
		return 1;
	}

	freenect_set_tilt_degs(f_dev,freenect_angle);
	freenect_set_led(f_dev,LED_RED);
	freenect_set_depth_callback(f_dev, depth_cb);
	freenect_set_rgb_callback(f_dev, rgb_cb);
	freenect_set_rgb_format(f_dev, FREENECT_FORMAT_RGB);
	freenect_set_depth_format(f_dev, FREENECT_FORMAT_11_BIT);

	freenect_start_depth(f_dev);
	freenect_start_rgb(f_dev);

	//start the freenect thread to poll for events
	res = pthread_create(&ocv_thread, NULL, freenect_threadfunc, NULL);
	if (res) {
		printf("pthread_create failed\n");
		return 1;
	}

	Mat depthf;

	Mat frameMat(rgbMat);
	Mat out(frameMat.size(),CV_8UC1),
		outC(frameMat.size(),CV_8UC3);
	Mat prevImg(frameMat.size(),CV_8UC1),
		nextImg(frameMat.size(),CV_8UC1),
		prevDepth(depthMat.size(),CV_8UC1);
	vector<Point2f> prevPts,nextPts;
	vector<uchar> statusv;
	vector<float> errv;
	Rect cursor(frameMat.cols/2,frameMat.rows/2,10,10);
	bool update_bg_model = true;
	int fr = 1;
	int register_ctr = 0;
	bool registered = false;

	Point2i appear(-1,-1); double appearTS = -1;

	Point2i midBlob(-1,-1);
	Point2i lastMove(-1,-1);
	
	int hcr_ctr = -1;
	vector<int> hc_stack(20); int hc_stack_ptr = 0;
	
	while (!die) {
		fr++;

//		imshow("rgb", rgbMat);
		//Linear interpolation
		{
			Mat _tmp = (depthMat - 400.0);					//minimum observed value is ~440. so shift a bit
			_tmp.setTo(Scalar(2048), depthMat > 600.0);		//cut off at 600 to create a "box" where the user interacts
			_tmp.convertTo(depthf, CV_8UC1, 255.0/1648.0);	//values are 0-2048 (11bit), account for -400 = 1648
		}
		
//		{ //saving the frames to files for debug
//			stringstream ss; ss << "depth_"<<fr<<".png";
//			imwrite(ss.str(), depthf);
//		}
		
		//Logarithm interpolation - try it!, It should be more "sensitive" for closer depths
//		{
//			Mat tmp,tmp1;
//			depthMat.convertTo(tmp, CV_32FC1);
//			log(tmp,tmp1);
//			tmp1.convertTo(depthf, CV_8UC1, 255.0/7.6246189861593985);
//		}
//		imshow("depth",depthf);

				
		Mat tmp_bg_fg = depthf < 255;	//anything not white is "real" depth
		vector<Point> ctr;
		Scalar blb = refineSegments(Mat(),tmp_bg_fg,out,ctr,midBlob); //find contours in the foreground, choose biggest

		if(blb[0]>=0 && blb[3] > 500) {
			cvtColor(depthf, outC, CV_GRAY2BGR);

			//draw contour
			Scalar color(0,0,255);
			for (int idx=0; idx<ctr.size()-1; idx++)
				line(outC, ctr[idx], ctr[idx+1], color, 1);
			line(outC, ctr[ctr.size()-1], ctr[0], color, 1);

			//draw "major axis"
			Vec4f _line;
			Mat curve(ctr);
			fitLine(curve, _line, CV_DIST_L2, 0, 0.01, 0.01);
			line(outC, Point(blb[0]-_line[0]*70,blb[1]-_line[1]*70),
						Point(blb[0]+_line[0]*70,blb[1]+_line[1]*70),
						Scalar(255,255,0), 1);
						
			//blob center
			circle(outC, Point(blb[0],blb[1]), 50, Scalar(255,0,0), 3);
			
			//closest point to the camera
			Point minLoc; double minval;
			minMaxLoc(depthMat, &minval, NULL, &minLoc, NULL, out);
			circle(outC, minLoc, 5, Scalar(0,255,0), 3);
			vector<int> c;
			
//			cout << "min depth " << minval << endl;

			register_ctr = MIN((register_ctr + 1),60);

			if (register_ctr > 30 && !registered) {
				registered = true;
				appear.x = -1;
				cout << "register" << endl;
				send_event("Register", "");
				update_bg_model = false;
				
				lastMove.x = blb[0]; lastMove.y = blb[1];
			}

			if(registered) {
				stringstream ss; ss << "\"x\":" << (int)floor(blb[0]*100.0/640.0) << ",\"y\":"<<(int)floor(blb[1]*100.0/480.0);
				cout << "move: " << ss.str() << endl;
				send_event("Move", ss.str());
				
				//---------------------- fist detection ---------------------
				//calc laplacian of curve
				vector<Point> approxCurve;	//approximate curve
				approxPolyDP(curve, approxCurve, 10.0, true);
				Mat approxCurveM(approxCurve);
				
				Mat curve_lap;
				calc_laplacian(approxCurveM, curve_lap);	//calc laplacian
				
				hcr_ctr = 0;
				for (int i=0; i<approxCurve.size(); i++) {
					double n = norm(((Point2d*)(curve_lap.data))[i]);
					if (n > 5.0) {
						//high curvature point
						circle(outC, approxCurve[i], 3, Scalar(50,155,255), 2);
						hcr_ctr++;
					}
				}
				
				hc_stack.at(hc_stack_ptr) = hcr_ctr;
				hc_stack_ptr = (hc_stack_ptr + 1) % hc_stack.size();
				
				Scalar _avg = mean(Mat(hc_stack));
				if ((_avg[0] - (double)hcr_ctr) > 5.0) { //a big drop in curvature = hand fisted?
					cout << "Hand click!" << endl;
					send_event("HandClick", "");
				}
//				{	//some debug on screen..
//					stringstream ss; ss << "high curve pts " << hcr_ctr << ", avg " << _avg[0];
//					putText(outC, ss.str(), Point(50,50), CV_FONT_HERSHEY_PLAIN, 2.0,Scalar(0,0,255), 2);
//				}				
			} else {
				//not registered, look for gestures
				if(appear.x<0) {
					//first appearence of blob
					appear = midBlob;
//					update_bg_model = false;
					appearTS = getTickCount();
					cout << "appear ("<<appearTS<<") " << appear.x << "," << appear.y << endl;
				} else {
					//blob was seen before, how much time passed
					double timediff = ((double)getTickCount()-appearTS)/getTickFrequency();
					if (timediff > .1 && timediff < 1.0) {
						//enough time passed from appearence
						if (appear.x - blb[0] > 100) {
							cout << "right"<<endl; appear.x = -1;
							send_event("SwipeRight", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.x - blb[0] < -100) {
							cout << "left" <<endl; appear.x = -1;
							send_event("SwipeLeft", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.y - blb[1] > 150) {
							cout << "up" << endl; appear.x = -1;
							send_event("SwipeUp", "");
							update_bg_model = true;
							register_ctr = 0;
						} else if (appear.y - blb[1] < -150) {
							cout << "down" << endl; appear.x = -1;
							send_event("SwipeDown", "");
							update_bg_model = true;
							register_ctr = 0;
						}
					}
					if(timediff >= 1.0) {
						cout << "a ghost..."<<endl;
						update_bg_model = true;
						//a second passed from appearence - reset 1st appear
						appear.x = -1;
						appearTS = -1;
						midBlob.x = midBlob.y = -1;
					}
				}
			}
			imshow("blob",outC);
			send_image(outC);
		} else {
			imshow("blob",depthf);
			send_image(depthf);
			register_ctr = MAX((register_ctr - 1),0);
		}

		if (register_ctr <= 15 && registered) {
			midBlob.x = midBlob.y = -1;
			registered = false;
			update_bg_model = true;
			cout << "unregister" << endl;
			send_event("Unregister", "");
		}

        char k = cvWaitKey(5);
        if( k == 27 ) break;
        if( k == ' ' )
            update_bg_model = !update_bg_model;
		if (k=='s') {
			cout << "send test event" << endl;
			send_event("TestEvent", "");
		}
	}

	printf("-- done!\n");

	pthread_join(ocv_thread, NULL);
	pthread_exit(NULL);
	return 0;
}
*/