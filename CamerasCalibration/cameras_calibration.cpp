#include <iostream>
#include <string>

#include "calibration.h"


using namespace calib;


const char* cmdOptions =
"{ v  video                             |         <none>        | video to process        }"
"{ w  width_pattern_size                |         <none>        | chessboard size width   }"
"{ h  height_pattern_size               |         <none>        | chessboard size height  }"
"{ s  square_size                       |          1.0          | chessboard square size  }"
"{ p  output_params                     |       params.yml      | path to calib params	  }"
"{ wp writer_path                       | calibration_video.avi | path to output video	  }"
"{ c  color                             |            1          | writer color            }"
"{ q ? help usage                       |                       | print help message      }";


std::int32_t CamerasCalibration(std::string video_path, cv::Size pattern_size, std::double_t square_size, std::string params);


int main(int argc, const char** argv)
{
	// Process input arguments
	cv::CommandLineParser parser(argc, argv, cmdOptions);

	if (parser.has("help"))
	{
		parser.printMessage();
		return -1;
	}
	if (!parser.check())
	{
		parser.printErrors();
		return -1;
	}

	// Load video and init parameters
	std::string video_path = parser.get<std::string>("video");
	std::int32_t width_pat = parser.get<std::int32_t>("w");
	std::int32_t height_pat = parser.get<std::int32_t>("h");
	std::double_t square_size = parser.get<std::double_t>("s");
	std::string params = parser.get<std::string>("p");
	std::string writer_path = parser.get<std::string>("writer_path");
	bool writer_color = parser.get<bool>("color");

	// Calibration by input video
	if (!video_path.empty())
		return CamerasCalibration(video_path, cv::Size(width_pat, height_pat), square_size, params);

	// Cameras open
	cv::Mat frame1, frame2;
	cv::VideoCapture cap1(1), cap2(2);
	CV_Assert(cap1.isOpened() && cap2.isOpened());

	cap1 >> frame1;
	cap2 >> frame2;

	CV_Assert(frame1.size() == frame2.size());

	std::cout << ">> Frame size:"
		      << "\n  Cap1: " << frame1.size()
		      << "\n  Cap2: " << frame2.size() << std::endl;


	// Writer initialization
	if (writer_path.empty())
		writer_path = "calibration_video.avi";
	std::cout << ">> Video path: " << writer_path << std::endl;

	int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
	cv::Size writer_size = cv::Size(frame1.size().width + frame2.size().width, frame1.size().height);
	std::double_t fps = cap1.get(cv::CAP_PROP_FPS);
	cv::VideoWriter writer(writer_path, fourcc, fps, writer_size, writer_color);

	while (true)
	{
		cap1 >> frame1;
		cap2 >> frame2;

		cv::Mat stereo;
		cv::hconcat(frame1, frame2, stereo);

		writer << stereo;

		std::string win_name = "Stereo recording";
		cv::namedWindow(win_name, cv::WINDOW_FREERATIO);
		cv::imshow(win_name, stereo);

		std::int8_t key = cv::waitKey(1);
		if (key == 27)	break;
	}

	writer.release();
	cv::destroyAllWindows();

	return CamerasCalibration(writer_path, cv::Size(width_pat, height_pat), square_size, params);
}


std::int32_t CamerasCalibration(std::string video_path, cv::Size pattern_size, std::double_t square_size, std::string params)
{
	CV_Assert(!video_path.empty() && pattern_size.width > 0 && pattern_size.height > 0 && square_size > 0.0);


	Calibration *calibration = nullptr;
	calibration = new StereoCalibration(Pattern::CHESSBOARD, pattern_size, square_size, params);
	calibration->CalibrationByVideo(video_path);
	calibration->writeParams();

	std::cout << params << std::endl;
	CalibrationReader *reader = nullptr;
	reader = new StereoCalibrationReader(params);
	dynamic_cast<StereoCalibrationReader*>(reader)->computeParams();
	dynamic_cast<StereoCalibrationReader*>(reader)->show();


	if (calibration)
	{
		delete calibration;
		calibration = nullptr;
	}
	if (reader)
	{
		delete reader;
		reader = nullptr;
	}


	return 0;
}