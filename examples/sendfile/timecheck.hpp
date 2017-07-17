#ifndef TIMECHECK_HPP
#define TIMECHECK_HPP

#include <chrono>   
#include <iostream>
#include <string>

class TimeCheck
{
	public:
		TimeCheck(std::string strfuncName="") { funcName_=strfuncName;start_ = std::chrono::system_clock::now(); };
		~TimeCheck() 
		{ 
			if ( !bChecked  )
			{
				TotalTime();
			}
		};

		void TotalTime()
		{
			end_  = std::chrono::system_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_);
			if ( funcName_.size() )
			{
				std::cout << "file:"<<funcName_<<",花费了 " 
     				<< double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den 
     				<< "秒" << std::endl;
			}
			else
			{
				std::cout <<  "花费了 " 
     				<< double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den 
     				<< "秒" << std::endl;
			}
			
		};
	private:
		std::chrono::time_point<std::chrono::system_clock> start_;
		std::chrono::time_point<std::chrono::system_clock> end_;
		std::string funcName_;
		bool	bChecked = false;
};


#endif /* TIMECHECK_HPP */
