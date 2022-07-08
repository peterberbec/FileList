#include <iostream>																// everybody need std::cout
#include <iomanip>																// for setw and precision
#include <string>																// for string stuff
#include <filesystem>															// for filesystem
#include <fstream>																// for std::basic_ifstream
#include <chrono>																// for time stuff
#include <sstream>																// for std::stringstream
#include <conio.h>																// for _kbhit
#include <stdexcept>															// for memory allocation handling
#include <random>																// for random number generation
#include <algorithm>															// for random number seeding

#define INITIAL_MEM_REQUEST	268435456												// start with 256MB of memory, work our way down until it doesn't fail
#define TO_SECONDS(A)		(A/10.0)												// convert tenths-of-a-second to seconds
#define BANDWIDTH_CALC(A, B)	((long long)(A/TO_SECONDS(B)))							// convert A bytes in B milliseconds to bytes/second
#define TARGET_BANDWIDTH		700000000												// over 700MB/sec means it either read from an SSD, RAM or a RAID array - any one is fine by me
#define WIDTH_0			36													// filename		used for std::setw formatting
#define WIDTH_1			20													// size			used for std::setw formatting
#define WIDTH_2			12													// bandwidth		used for std::setw formatting
#define WIDTH_3			14													// file number		used for std::setw formatting
#define WIDTH_4			7													// time			used for std::setw formatting
#define INPUT_TIMEOUT		5													// seconds to wait for user input

void do_output(std::string, std::string, std::string, std::string, std::string, int);		// Function definitions
void parse_args(int, char**, int&, std::string&);
void command_args();
void clear_line();

bool check_reread(int, double, long long);
bool check_goal_time(long long, int);
bool check_bandwidth(long long, int);
bool is_number(const std::string&);
bool input_wait_for(long long);
bool check_flag(std::string);

std::string truncate_dots(std::string, int);
std::string truncate(std::string, int);
std::string fsize_f(std::streamsize);
std::string seconds_f(long long);

size_t allocate_buffer(char*&, size_t);

long long do_read(std::string);
long double pow_ld(int, int);

auto rng_generator();

auto rng_generator()															// load up an RNG and seed it properly
{
	std::vector<unsigned int> random_data(512);
	std::random_device source;
	std::generate(begin(random_data), end(random_data), [&]() {return source(); });
	std::seed_seq seeds(begin(random_data), end(random_data));
	std::mt19937 seededEngine(seeds);
	return seededEngine;
}

long double pow_ld(int base, int exponent)											// pow() didn't work. I needed bigger.
{
	long double answer = (long double)base;
	while (--exponent > 0)
	{
		answer *= base;
	}

	return answer;
}

void do_output(std::string filename_s, std::string transferred_s, std::string 
	bandwidth_full_s, std::string filenum_s, std::string duration_s, int retries)			// output transfer information
{
	clear_line();																// ncurses is hard
	std::cout << "[";															// setw lets us use columns
	std::cout << std::setw(WIDTH_0) << std::left  << truncate_dots(  filename_s, WIDTH_0);	// filename, truncated, without path
	std::cout << std::setw(WIDTH_1) << std::right << truncate(	  transferred_s, WIDTH_1);	// transferred formatted
	std::cout << std::setw(WIDTH_2) << std::right << truncate( bandwidth_full_s, WIDTH_2);	// bandwidth formatted
	std::cout << std::setw(WIDTH_3) << std::right << truncate(		 filenum_s, WIDTH_3);	// files formatted
	switch (retries)															// depending on retries, add a character
	{
		case 1:	std::cout << "-"; break;
		case 2:	std::cout << "="; break;
		case 3:	std::cout << "*"; break;
		case 4:	std::cout << "#"; break;
		default:	std::cout << " "; break;
	}
	std::cout << std::setw(WIDTH_4) << std::right << duration_s;
	std::cout << "]";

	return;
}
 
bool check_reread(int attempts, double elapsed_seconds, long long size)					// should we read the file again?
{
	if (attempts > 5)															// have we already read it alot?
	{
		return false;
	}
	else if (BANDWIDTH_CALC(size, elapsed_seconds) >= TARGET_BANDWIDTH)					// did we already reach our bandwidth goal??
	{
		return false;
	}
	else if (size < 10000)														// is it a small file?
	{
		return false;
	}
	else																		// I'ma do it again! hyuck, hyuck...
	{
		return true;
	}
}

bool check_bandwidth(long long bandwidth, int goal_time)								// let's check if we reached our bandwidth goal
{	
	if (goal_time < 0)															// go forever?
	{																		// goal_time==-1, false | goal_time==0, no goal_time given | goal_time>0 we have a goal
		return false;
	}
	else if (bandwidth > TARGET_BANDWIDTH)											// hit our bandwidth goal?
	{
		return true;
	}
	else																		// I'ma do it again! hyuck, hyuck...
	{
		return false;
	}
}

bool check_goal_time(long long time_elapsed, int goal_time)								// let's check if we reached our goal time
{	
	time_elapsed = (long long)TO_SECONDS(time_elapsed);
	if (goal_time < 1)															// was the goal zero?
	{																		// goal_time==-1, false | goal_time==0, no goal_time given | goal_time>0 we have a goal
		return false;
	}
	else if (time_elapsed < goal_time)												// did we do it fast enough?
	{
		return true;
	}
	else																		// I'ma do it again! hyuck, hyuck...
	{
		return false;
	}
}

void command_args()																// Help text
{
	std::cout <<	"## Fill_SSD_Cache.exe\n\n"
				"Reads all file, recursively, in target directory. Loops forever, with user prompt\n"
				"to quit after every loop, or until read is accomplished in goal time or SSD-speed.\n"
				"Default behavior is to read directory until 700MB/second is reached.\n\n"
				"The purpose of this program is to load an SSD cache from a hard drive. I wrote it\n"
				"to load PrimoCache with FF14, as it was doing a bad job of loading. It grew from there\n"
				"and now can run on any directory. I've integrated it into my Windows \"Send-To\" menu.\n\n"
				"## Usage\n\n"
				"One argument is required - the directory to be scanned. Put it in Quotes to make\n"
				"life easier - long - filenames and spaces in directroy names.\n\n"
				"There are two options for the second argument, which is not required.\n"
				"Option A is goal time to quit after, in seconds.\n"
				"Option B is \"/forever\", \"/f\", \"-f\" or \"-forever\" which will ignore built-in speed\n"
				"goal and read forever.\n\n"
				"Argument order is not important.\n\n"
				"Example A -\n"
				"\tC:\\> filelist.exe \"C:\\Directory\"\n"
				"Example B -\n"
				"\tC:\\> filelist.exe 64 \"D:\\Directory\\With Spaces\"\n"
				"Example C -\n"
				"\tC:\\> filelist.exe \"E:\\Windows Games\" /forever\n";
	input_wait_for(15);
}

bool is_number(const std::string& s)												// Test to see if s is a number
{
	if (s.empty())																// empty is not a number
	{
		return false;
	}
	else																		// find the first occurance of not a number
	{																		// failure returns std::string::npos
		return (s.find_first_not_of("0123456789") == std::string::npos);
	}
}

std::string truncate_dots(std::string str, int width)									// truncate a string if longer than width characters
{																								
	if ((str.length() + 3) >= width)
	{
		return str.substr(0, width - 4) + "...";									// add an elipsis at the end
	}
	return str;
}

std::string truncate(std::string str, int width)										// truncate a string if longer than width characters
{
	if (str.length() >= width)
	{
		return str.substr(0, width);
	}
	return str;
}

void clear_line()																// curses is hard. hax are easy
{
	char to_print[3] = { '\b', ' ', '\b' };
	long long width_f = WIDTH_0 + WIDTH_1 + WIDTH_2 + WIDTH_3 + WIDTH_4 + 3;				// calculate the length of the line

	for (long long j = 0; j < 3; j++)												// loop through backspace, space and backspace
	{
		for (long long i = 0; i < width_f; i++)										// loop through line length
		{
			std::cout << to_print[j];											// output a character
		}
	}
	return;
}

std::string seconds_f(long long num_10th_seconds)										// format a number of seconds into 3d12h05m17s
{
	std::string ret_string = "";
	long long seconds_running = (long long)num_10th_seconds, temp, 
		minute = 60*60, hour = 60*minute, day = 24*hour;								// number of seconds in a 

	if (TO_SECONDS(num_10th_seconds) < minute)										// if we are given less than 60 seconds, use a decimal place
	{
		std::stringstream temp_ss;
		temp_ss << std::fixed << std::setprecision(1) << TO_SECONDS(num_10th_seconds);		// take care of pesky milliseconds
		ret_string = temp_ss.str() + "s";
		return ret_string;
	}
	else
	{
		if (seconds_running > day)
		{
			temp = seconds_running / day;											// figure out number of days
			ret_string = std::to_string(temp) + "d";
			seconds_running -= temp * day;										// subtract number of days
		}
		if (seconds_running > hour)
		{
			temp = seconds_running / hour;										// figure out number of hours
			if (temp < 10 && ret_string != "")										// if less than 10 hours, and we are using days, add a leading 0
			{
				ret_string = ret_string + "0" + std::to_string(temp) + "h";
			}
			else
			{
				ret_string = ret_string + std::to_string(temp) + "h";
			}
			seconds_running -= temp * hour;										// subtract number of hours
		}
		if (seconds_running > minute)
		{
			temp = seconds_running / minute;										// figure out number of minutes
			if (temp < 10 && ret_string != "")										// if less than 10 minutes, and we are using hours, add a leading 0
			{
				ret_string = ret_string + "0" + std::to_string(temp) + "m";
			}
			else
			{
				ret_string = ret_string + std::to_string(temp) + "m";
			}
			seconds_running -= temp * minute;										// subtract number of minutes
		}
		if (seconds_running < 10)												// if less than 10 seconds, and we are using minutes, add a leading 0
		{
			ret_string = ret_string + "0" + std::to_string(seconds_running) + "s";
		}
		else
		{
			ret_string = ret_string + std::to_string(seconds_running) + "s";
		}
	}

	return ret_string;
}

std::string fsize_f(std::streamsize number)											// format number of bytes to B, KB, MB, GB, TB and PB
{
	std::stringstream number_fs;													// for using std::fixed and std::precision but not in std::cout
	int precision = 0, power;
	char suffix[6][3] = { "B ", "KB", "MB", "GB", "TB", "PB" };							// Suffixes for powers of 2^1 up to 2^50

	for (power = 1; power < 7; power++)											// start at 1, go to 6
	{
		precision = 0;															// default at no decimal places
		if (number < pow_ld(2, 10*power))											// if our number is below 2^power
		{
			if (power > 2)														// if we are in quadruple digits
			{
				if ((number/pow_ld(2, 10 * (power-1))) < 10)							// between 1 and 9
				{
					precision = 2;												// we want 1.23
				}
				else if ((number/pow_ld(2, 10 * (power-1))) < 100)					// between 10 and 99
				{
					precision = 1;												// we want 12.3
				}
			}
			number_fs << std::fixed << std::setprecision(precision) << 
				(number / pow_ld(2, 10 * (power - 1)));								// put the output we want in a streamstream and then
			return number_fs.str() + suffix[power - 1];								// return it as a string with the proper suffix
		}
	}
	return std::to_string(number) + suffix[0];										// don't know how we'd get here, but let's just return 12341B
}

size_t allocate_buffer(char*& buffer_d, size_t buffer_size)								// allocate memory
{
	size_t real_buffer;
	std::exception_ptr p = NULL;
	auto prb_rng = rng_generator();												// startup the RNG
	std::uniform_int_distribution<long> distch(0, 255);								// make it evenly spit out chars

	for(real_buffer = buffer_size; real_buffer > 1; real_buffer = real_buffer/2)			// start at requested memory, half it every failure
	{
		buffer_d = new (std::nothrow) char[real_buffer];								// allocate the memory, but don't puke
		if (buffer_d)															// if we got memory
		{
			try																// Let's just see...
			{
				for (unsigned long long c = 0; c < real_buffer; c++)					// loop through the entire buffer
				{
					buffer_d[c] = (char)(distch(prb_rng));							// fill it with noise
				}
			}
			catch(...)														// OHSHIT catch everything
			{
				p = std::current_exception();										// grab the exception
			}
			if (p != NULL)														// we got an exception
			{
				delete buffer_d;												// delete the invalid pointer
				std::cout << "Memory exception thrown: ";

				try															// Let's just see...
				{
					std::rethrow_exception(p);									// Throw the same exception
				}
				catch (const std::exception& e)
				{
					std::cout <<  e.what() << "\n";								// WTF Happened?
					std::cout << "Please email peter@havetechwilltravel.nyc "
						"with this information\n";								// Please let me know what went on
				}
			}
			else																// Everything was hunky dory
			{
				break;														// let's get out of the for loop
			}
		}
		else																	// we didn't get any memory
		{
			delete buffer_d;													// delete the likely non-existant ram
		}
	}																		// I'ma do it again! hyuck, hyuck...
	delete buffer_d;															// get rid of all the random characters
	buffer_d = new (std::nothrow) char[real_buffer];									// allocate memory for real
	return real_buffer;															// return the size of the buffer
}

long long do_read(std::string path)												// the big read function
{
	using namespace std::chrono_literals;
		
	std::ifstream file2read;
	std::chrono::steady_clock::time_point start, start_full, start_display;
	std::chrono::duration<double, std::ratio<1, 10>> elapsed_full, 
		elapsed_file, refresh_time = 0.1s;											// std::ratio<1, 10> means time is kept in tenths of a second
	std::streamsize file_size = 0;
	std::string transferred_s, bandwidth_full_s, filenum_s, duration_s;
	unsigned long long block_size;
	long long number_of_files = 0, current_file = 0, total_size = 0, done_size = 0;
	long long bandwidth_full;
	int i = 0, retries;
	bool first = true;
	char* buffer_d;

	block_size = allocate_buffer(buffer_d, INITIAL_MEM_REQUEST);

	for (const auto& entry : std::filesystem::recursive_directory_iterator(path, 
		std::filesystem::directory_options::skip_permission_denied))
	{																		// iterate through the target directory
		if (!entry.is_directory())												// don't count directories
		{
			number_of_files++;													// total files
			total_size += entry.file_size();										// total bytes
		}
	}
	start_full = std::chrono::steady_clock::now();									// start the clock for the whole endevour
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path, 
		std::filesystem::directory_options::skip_permission_denied))
	{																		// iterate through the target directory
		retries = 0;
		if (!entry.is_directory())												// we only read files
		{
			current_file++;													// keep track of file number
			filenum_s = std::to_string(current_file) + "/" + 
				std::to_string(number_of_files);									// format files done and total files
			file_size = entry.file_size();										// check our current file's size
			file2read.open(entry.path().string(), std::ios_base::in | std::ios::binary);	// open file for read-only, binary data
			if (!file2read.is_open())											// if the file didn't open
			{
				file2read.clear();												// clear the error.
			}
			else
			{
				while (true)													// file is open. let's loop forever. (we break when needed)
				{
					first = true;
					start = std::chrono::steady_clock::now();						// start the clock
					start_display = start;
					for (std::streamsize bytes = 0; bytes < file_size; 
						bytes += block_size)									// don't go past the end of the file
					{
						file2read.read(buffer_d, block_size);						// read a block
						duration_s = seconds_f((long long)(elapsed_full = 
							(std::chrono::steady_clock::now() - start_full)).count());	// format time spent
						transferred_s = fsize_f(done_size += file2read.gcount()) + "/" 
							+ fsize_f(total_size);								// keep track of data transferred
						bandwidth_full_s = fsize_f(bandwidth_full = 
							BANDWIDTH_CALC(done_size, elapsed_full.count())) + "/sec";	// filesize/time = b/s for the whole transfer
						if (first || ((std::chrono::steady_clock::now() - 
							start_display) > refresh_time))						// if this is the first run-through, or we've waited refresh_time
						{
							first = false;										// not our first run anymore, right?
							start_display = std::chrono::steady_clock::now();			// start the clock again
							do_output(entry.path().filename().string(), transferred_s, 
								bandwidth_full_s, filenum_s, duration_s, retries);	// update screen
						}
					}
					elapsed_file = std::chrono::steady_clock::now() - start;
					if (!check_reread(++retries, elapsed_file.count(), file_size))		// we don't retry more than five times, reach TARGET_BANDWIDTH
					{														// or on small files. small files open/close time makes the bandwidth numbers ugly
						break;												// NOW we finally get out of this while loop
					}
					else														// we need to retry
					{
						done_size -= file_size;									// keep bandwidth sane, remove the bytes we just read.
						file2read.clear();										// clear the buffer's flags
						file2read.seekg(std::ios_base::beg);						// seek to zero
					}
				}
				file2read.close();												// done reading the file. close it
			}
		}
	}
	std::cout << "\n";															// we are done reading, so we don't need to ncurses anymore.

	return done_size;															// return the amount of data actually read
}

bool input_wait_for(long long timeout)												// wait for user input for timeout seconds
{
	std::time_t start = std::time(0);

	while(true)
	{
		if (std::difftime(std::time(0), start) >= timeout)							// check for timeout
		{
			return false;														// timeout reached before input
		}
		if (_kbhit())															// check if they keyboard was touched without needing an "Enter"
		{
			return true;														// input received before timeout
		}
	}
}

bool check_flag(std::string flag)
{
	if (flag == "/f")
	{
		return true;
	}
	else if (flag == "/forever")
	{
		return true;
	}
	else if (flag == "-f")
	{
		return true;
	}
	else if (flag == "-forever")
	{
		return true;
	}
	else
	{
		return false;
	}
}

void parse_args(int argc, char **argv, int &goal_time, std::string &path2read)				// parse command line arguments
{
	std::string temp;
	if (argc == 2)																// got a path, hopefully
	{
		path2read = argv[1];
		goal_time = 0;															// we didn't get a goal time

		if (!std::filesystem::is_directory(path2read))								// not a path
		{
			std::cout << "Argument passed \"" << path2read 
				<< "\" is NOT a valid directory.\n\n";
			command_args();													// help text
			exit(2);															// goodbye
		}
	}
	else if (argc == 3)															// got a path and goal time, hopefully
	{
		path2read = argv[1];
		temp = argv[2];

		if (!std::filesystem::is_directory(path2read))								// argv[1] isn't a directory
		{
			path2read = argv[2];
			temp = argv[1];

			if (!std::filesystem::is_directory(path2read))							//argv[2] isn't a directory
			{
				std::cout << "Arguments passed \"" << path2read << "\" and \""
					<< temp << "\" are NOT valid directorys.\n\n";
				command_args();												// help text	
				exit(2);														// goodbye
			}
			if (!is_number(temp))												// argv[2] is a directory, but argv[1] isn't a valid number
			{
				if (check_flag(temp))											// the don't stop flag
				{
					goal_time = -1;
					return;
				}
				std::cout << "Argument passed \"" << path2read 
					<< "\" is was a valid directory, BUT\n";
				std::cout << "Argument passed \"" << temp 
					<< "\" is NOT a valid number, / false or /f.\n\n";
				command_args();												// help text
				exit(2);														// goodbye
			}
			else
			{
				goal_time = std::stoi(temp);
				if (goal_time < 1)
				{
					goal_time = 0;												// can't use negative numbers, boss
				}
			}
		}
		else if (!is_number(temp))												// argv[1] is a directory, but argv[2] isn't a valid number
		{
			if (check_flag(temp))												// the don't stop flag
			{
				goal_time = -1;
				return;
			}
			std::cout << "Argument passed \"" << temp 
				<< "\" is NOT a valid number, /forever or /f, while\n";
			std::cout << "Argument passed \"" << path2read 
				<< "\" is was a valid directory.\n\n";
			command_args();													// help text
			exit(2);															// goodbye
		}
		else
		{
			goal_time = std::stoi(temp);
			if (goal_time < 1)
			{
				goal_time = 0;													// can't use negative numbers, boss
			}
		}
	}
	else																		// one or two arguments, people!
	{
		command_args();														// help text
		exit(2);																// goodbye
	}

	return;
}

int main(int argc, char** argv)
{
	std::string path2read, bandwidth_s;
	std::chrono::steady_clock::time_point start;
	std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds;
	long long transferred, bandwidth;
	int ret_val = 0, goal_time = 0;
	
	parse_args(argc, argv, goal_time, path2read);

	while (true)																// too many ways to end the loop for a simple conditional
	{
		clear_line();															// ncurses or gui sometime soon, please

		start = std::chrono::steady_clock::now();									// start the clock
		transferred = do_read(path2read);											// do the reading, return value is bytes transferred
		elapsed_seconds = std::chrono::steady_clock::now() - start;						// figure out how long it took
		bandwidth_s = fsize_f(bandwidth = 
			BANDWIDTH_CALC(transferred, elapsed_seconds.count())) + "/sec";				// calculate bandwidth

		std::cout << "\"" << truncate_dots(path2read, 22) << "\": read " <<
			fsize_f(transferred) << " " << " in " <<
			seconds_f((long long)elapsed_seconds.count()) << " at " << bandwidth_s;		// Output full-runthrough statistics

		if (check_goal_time((long long)elapsed_seconds.count(), goal_time))				// we got a goal_time, let's use it
		{
			std::cout << "; within goal time. Press any key to continue.";				// great news. probably quit, right?
			if (!input_wait_for(INPUT_TIMEOUT))									// user DIDN'T hit a key, time to go home
			{
				ret_val=1;
				break;
			}
		}
		else if (check_bandwidth(bandwidth, goal_time))								// we read in SSD speed, on average
		{
			std::cout << "; over goal speed. Press any key to continue.";				// great news. probably quit, right?
			if (!input_wait_for(INPUT_TIMEOUT))									// user DIDN'T hit a key, time to go home
			{
				ret_val = 1;
				break;
			}
		}
		else																	// We didn't hit goal_time AND didn't hit SSD speed
		{
			std::cout << ". Press any key to quit.";								// bag news. probably go again, right?
			if (input_wait_for(INPUT_TIMEOUT))										// user DID hit a key, time to go home.
			{
				ret_val = 1;
				break;
			}
		}
	}

	return ret_val;															// time to go home
}