#include <iostream>
#include <string>

#include "calibration.h"


using namespace calib;


const char* cmdOptions =
"{ v   video                             |         <none>        | video to process        }"
"{ noc number_of_cameras                 |           2           | number of cameras       }"
"{ w   width_pattern_size                |         <none>        | chessboard size width   }"
"{ h   height_pattern_size               |         <none>        | chessboard size height  }"
"{ s   square_size                       |          1.0          | chessboard square size  }"
"{ p   output_params                     |       params.yml      | path to calib params    }"
"{ wp  writer_path                       | calibration_video.avi | path to output video	   }"
"{ c   color                             |            1          | writer color            }"
"{ q ? help usage                        |                       | print help message      }";


std::int32_t CamerasCalibration(std::string video_path, cv::Size pattern_size,
	std::double_t square_size, std::string params, std::uint32_t number_of_cameras = 1);

bool getFrame(cv::Mat &frame, cv::VideoCapture &cap1, cv::VideoCapture &cap2, std::int32_t number_of_cameras = 1);


void info()
{
	std::cout << "Press Esc to start Calibration" << std::endl;
}


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
	std::uint32_t number_of_cameras = parser.get<std::int32_t>("noc");
	std::int32_t width_pat = parser.get<std::int32_t>("w");
	std::int32_t height_pat = parser.get<std::int32_t>("h");
	std::double_t square_size = parser.get<std::double_t>("s");
	std::string params = parser.get<std::string>("p");
	std::string writer_path = parser.get<std::string>("writer_path");
	bool writer_color = parser.get<bool>("color");

	CV_Assert(number_of_cameras == 1 || number_of_cameras == 2);

	// Calibration by input video
	if (!video_path.empty())
		return CamerasCalibration(video_path, cv::Size(width_pat, height_pat), square_size, params, number_of_cameras);

	// Cameras open
	cv::Mat frame;
	cv::VideoCapture cap1, cap2;


	// Writer initialization
	if (writer_path.empty())
		writer_path = "calibration_video.avi";
	std::cout << ">> Video path: " << writer_path << std::endl;

	if (!getFrame(frame, cap1, cap2, number_of_cameras))
		return -1;
	
	int fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
	std::double_t fps = cap1.get(cv::CAP_PROP_FPS);
	cv::VideoWriter writer(writer_path, fourcc, fps, frame.size(), writer_color);

	info();
	while (true)
	{
		if (!getFrame(frame, cap1, cap2, 1))
			break;

		writer << frame;

		std::string win_name = "Stereo recording";
		cv::namedWindow(win_name, cv::WINDOW_FREERATIO);
		cv::imshow(win_name, frame);

		std::int8_t key = cv::waitKey(1);
		if (key == 27)	break;
	}

	writer.release();
	cv::destroyAllWindows();

	return CamerasCalibration(writer_path, cv::Size(width_pat, height_pat), square_size, params, number_of_cameras);
}


std::int32_t CamerasCalibration(std::string video_path, cv::Size pattern_size,
	std::double_t square_size, std::string params, std::uint32_t number_of_cameras)
{
	CV_Assert(!video_path.empty() && pattern_size.width > 0 && pattern_size.height > 0 && square_size > 0.0);

	Calibration *calibration = nullptr;
	CalibrationReader *reader = nullptr;

	if (number_of_cameras == 1)
	{
		calibration = new SingleCalibration(Pattern::CHESSBOARD, pattern_size, square_size, params);
		reader = new SingleCalibrationReader(params);
	}
	if (number_of_cameras == 2)
	{
		calibration = new StereoCalibration(Pattern::CHESSBOARD, pattern_size, square_size, params);
		reader = new StereoCalibrationReader(params);
		//dynamic_cast<StereoCalibrationReader*>(reader)->computeParams();
	}

	system("cls");
	std::cout << ">> Start Calibration" << std::endl;
	cv::waitKey(1000);

	calibration->CalibrationByVideo(video_path);
	calibration->writeParams();
	reader->read();
	reader->show();


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

	std::cout << ">> Complete"
		<< "\n>> Exit\n" << std::endl;
	
	for (int i = 7; i > 0; i--)
	{
		std::cout << i << "ms" << std::endl;
		cv::waitKey(1000);
	}
	return 0;
}


bool getFrame(cv::Mat &frame, cv::VideoCapture &cap1, cv::VideoCapture &cap2, std::int32_t number_of_cameras)
{
	if (number_of_cameras == 1 && !cap1.isOpened())
	{
		cap1.open(1);
		CV_Assert(cap1.isOpened());
	}
	if (number_of_cameras == 2 && !cap1.isOpened() && !cap2.isOpened())
	{
		cap1.open(1);
		cap2.open(2);

		CV_Assert(cap1.isOpened() && cap2.isOpened());
	}


	if (cap1.isOpened() && cap2.isOpened())
	{
		cv::Mat left, right;
		cap1 >> left;
		cap2 >> right;

		CV_Assert(left.size() == right.size());
		if (left.empty() || right.empty())
			return false;

		cv::hconcat(left, right, frame);
	}
	else if (cap1.isOpened())
		cap1 >> frame;

	if (frame.empty())
		return false;

	return true;
}