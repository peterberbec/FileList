#undef _DEBUG

#include <iostream>                 // everybody need std::cout
#include <string>                   // for string stuff
#include <filesystem>               // for filesystem
#include <fstream>                  // for std::basic_ifstream
#include <chrono>                   // for time stuff
#include <sstream>                  // for std::stringstream
#include <conio.h>                  // for _kbhit

#define BLOCK_SIZE              (pow(2, 25))                            // 32MB works well on my system. negligable bandwidth gains for higher memory footprint
#define to_seconds(A)           (A/10.0)                                // convert A tenths-of-a-second to seconds
#define bandwidth_calc(A, B)    ((long long)(A/to_seconds(B)))          // convert A bytes in B milliseconds to bytes/second
#define TARGET_BANDWIDTH        350000                                  // 500MB/sec means it hit the cache
#define FIELD_W_0               36                                      // filename     used for std::setw formatting
#define FIELD_W_1               20                                      // size         used for std::setw formatting
#define FIELD_W_2               12                                      // bandwidth    used for std::setw formatting
#define FIELD_W_3               14                                      // file number  used for std::setw formatting
#define FIELD_W_4               7                                       // tine         used for std::setw formatting

void do_output(std::string, std::string, std::string, std::string, std::string, long long);
void parse_args(int, char**, long long&, std::string&);
void command_args();
void clear_line();

bool check_reread(long long, double, long long);
bool check_goal_time(long long, long long);
bool check_bandwidth(long long, long long);
bool is_number(const std::string&);
bool input_wait_for(long long);

std::string truncate(std::string, size_t, bool);
std::string truncate(std::string, size_t);
std::string fsize_f(std::streamsize);
std::string seconds_f(long long);

long double pow_ld(long long, long long);
long long do_read(std::string);

long double pow_ld(long long base, long long exponent)
{
    long double answer = (long double)base;
    while (--exponent > 0)
    {
        answer *= (long double)base;
    }

    return answer;
}

void do_output(std::string filename_s, std::string transferred_s, std::string bandwidth_full_s, std::string filenum_s, std::string duration_s, long long retries)
{
    clear_line();   // ncurses is hard
    std::cout << "[";   // setw lets us use columns
    std::cout << std::setw(FIELD_W_0) << std::left << truncate(filename_s, FIELD_W_0, true);  // filename, truncated, without path
    std::cout << std::setw(FIELD_W_1) << std::right << truncate(transferred_s, FIELD_W_1);                          // transferred formatted
    std::cout << std::setw(FIELD_W_2) << std::right << truncate(bandwidth_full_s, FIELD_W_2);                            // bandwidth formatted
    std::cout << std::setw(FIELD_W_3) << std::right << truncate(filenum_s, FIELD_W_3);                              // files formatted
    switch (retries)    // depending on retries, add a character
    {
        case 0: std::cout << " "; break;
        case 1: std::cout << "-"; break;
        case 2: std::cout << "="; break;
        case 3: std::cout << "*"; break;
        case 4: std::cout << "#"; break;
    }
    std::cout << std::setw(FIELD_W_4) << std::right << duration_s;
    std::cout << "]";

    return;
}

bool check_reread(long long attempts, double elapsed_seconds, long long size) // should we read the file again?
{
    if (attempts >= 5)       // have we already read it alot?
    {
        return false;
    }
    else if (bandwidth_calc(size, elapsed_seconds) >= TARGET_BANDWIDTH)        // did we already reach our bandwidth goal??
    {
        return false;
    }
    else if (size < 10000)         // is it a small file?
    {
        return false;
    }
    else        // I'ma do it again! hyuck, hyuck...
    {
        return true;
    }
}

bool check_bandwidth(long long bandwidth, long long goal_time)      // let's check if we reached our bandwidth goal
{   // goal_time==-1, false | goal_time==0, no goal_time given | goal_time>0 we have a goal
    if (goal_time < 0) // go forever?
    {
        return false;
    }
    else if (bandwidth > TARGET_BANDWIDTH) // hit our bandwidth goal?
    {
        return true;
    }
    else         // I'ma do it again! hyuck, hyuck...
    {
        return false;
    }
}

bool check_goal_time(long long time_elapsed, long long goal_time)     // let's check if we reached our goal time
{   // goal_time==-1, false | goal_time==0, no goal_time given | goal_time>0 we have a goal
    time_elapsed = (long long)to_seconds(time_elapsed);
    if (goal_time < 1)        // was the goal zero?
    {
        return false;
    }
    else if (time_elapsed < goal_time)        // did we do it fast enough?
    {
        return true;
    }
    else         // I'ma do it again! hyuck, hyuck...
    {
        return false;
    }
}

void command_args()                 // Help text
{
    std::cout << "FileList.exe\n";
    std::cout << "Reads all file, recursively, in target directory. Loops forever, with user prompt\n";
    std::cout << "to quit after every loop, or until read is accomplished in goal time.\n";
    std::cout << "The purpose of this program is to load an SSD cache from a hard drive. I wrote it\n";
    std::cout << "to load PrimoCache with FF14, as it was doing a bad job of loading. It grew from there\n";
    std::cout << "and now can run on any directory. I've integrated it into my Windows \"Send-To\" menu.\n";
    std::cout << "One argument is required - the directory to be scanned. Put it in Quotes to make\n";
    std::cout << "life easier - long - filenames and spaces in directroy names.\n";
    std::cout << "Default behavior is to read directory until 350MB/second is reached.\n"
    std::cout << "There are two options for the second argument, which is not required.\n";
    std::cout << "Option A is goal time to quit after, in seconds.\n";
    std::cout << "Option B is \"/false\", which will ignore built-in speed goal and read forever.\n";
    std::cout << "Argument order is not important.\n";
    std::cout << "Example A -\n";
    std::cout << "C:\\> filelist.exe \"C:\\Directory\"\n";
    std::cout << "Example B -\n";
    std::cout << "C:\\> filelist.exe 64 \"D:\\Directory\\With Spaces\"\n";
    std::cout << "Example C -\n";
    std::cout << "C:\\> filelist.exe \"E:\\Windows Games\" /false\n";
    input_wait_for(15);
}

bool is_number(const std::string& s)    // Test to see if s is a number
{
    return !s.empty() 
        && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();

    /* 
    * if input string isn't empty, and
    * when searching from the beginning of s to the end of s, looking for chars, we find only digits and loop safely until the end
    * then this is a number
    */
}

std::string truncate(std::string str, size_t width, bool dots)        // truncate a string if longer than width characters
{
    if (str.length() >= width)
    {
        return str.substr(0, width);
    }
    return str;
}

std::string truncate(std::string str, size_t width)        // truncate a string if longer than width characters
{
    if (str.length() >= width)
    {
        return str.substr(0, width);
    }
    return str;
}

void clear_line()        // curses is hard. hax are easy
{
    char to_print[3] = { '\b', ' ', '\b' };
    long long width_f = FIELD_W_0 + FIELD_W_1 + FIELD_W_2 + FIELD_W_3 + FIELD_W_4 + 3;      // calculate the length of the line

    for (long long j = 0; j < 3; j++)    // loop through backspace, space and backspace
    {
        for (long long i = 0; i < width_f; i++)
        {
            std::cout << to_print[j];
        }
    }
    return;
}

std::string seconds_f(long long num_seconds)     // format a number of seconds into 3d12h5m17s
{
    std::string ret_string = "";
    double seconds_temp_f = to_seconds(num_seconds);      // take care of pesky milliseconds 
    long long seconds_running = (long long)seconds_temp_f, temp, minute = 60, hour = 3600, day = 86400;

    if (seconds_temp_f < minute)      // if we are given less than 60 seconds, use a decimal place
    {
        std::stringstream temp_ss;
        temp_ss << std::fixed << std::setprecision(1) << seconds_temp_f;
        ret_string = temp_ss.str() + "s";
        return ret_string;
    }
    else
    {
        if (seconds_running > day)
        {
            temp = seconds_running / day;     // figure out number of days
            ret_string = std::to_string(temp) + "d";
            seconds_running -= temp * day;    // subtract number of days
        }
        if (seconds_running > hour)
        {
            temp = seconds_running / hour;      // figure out number of hours
            if (temp < 10 && ret_string != "")      // if less than 10 hours, and we are using days, add a leading 0
            {
                ret_string = ret_string + "0" + std::to_string(temp) + "h";
            }
            else
            {
                ret_string = ret_string + std::to_string(temp) + "h";
            }
            seconds_running -= temp * hour;     // subtract number of hours
        }
        if (seconds_running > minute)
        {
            temp = seconds_running / minute;        // figure out number of minutes
            if (temp < 10 && ret_string != "")      // if less than 10 minutes, and we are using hours, add a leading 0
            {
                ret_string = ret_string + "0" + std::to_string(temp) + "m";
            }
            else
            {
                ret_string = ret_string + std::to_string(temp) + "m";
            }
            seconds_running -= temp * minute;       // subtract number of minutes
        }
        if (seconds_running < 10)  // if less than 10 seconds, and we are using minutes, add a leading 0
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

std::string fsize_f(std::streamsize number)     // format number of bytes to B, KB, MB, GB, TB and PB
{
    std::stringstream number_fs;
    long long precision = 0, power;
    char suffix[6][3] = { "B", "KB", "MB", "GB", "TB", "PB" };

    for (power = 1; power < 7; power++)
    {
        precision = 0;
        if (number < pow_ld(2, 10*power))
        {
            if (power > 2)
            {
                if ((number/pow_ld(2, 10 * (power-1))) < 10) // between 1 and 9
                {
                    precision = 2;
                }
                else if ((number/pow_ld(2, 10 * (power-1))) < 100) // between 10 and 99
                {
                    precision = 1;
                }
            }
            number_fs << std::fixed << std::setprecision(precision) << (number / pow_ld(2, 10 * (power - 1)));
            return number_fs.str() + suffix[power - 1];
        }
    }
    return std::to_string(number) + suffix[power - 1];
}

long long do_read(std::string path)      // the big read function
{
    using namespace std::chrono_literals;

    std::ifstream file2read;
    std::chrono::steady_clock::time_point start, start_full, start_display;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_full, elapsed_file, refresh_time = 0.1s;     // std::ratio<1, 10> means time is kept in tenths of a second
    std::streamsize file_size = 0, block_size = (std::streamsize)BLOCK_SIZE;
    std::string transferred_s, bandwidth_full_s, filenum_s, duration_s;
    char* buffer_d = new char[(size_t)BLOCK_SIZE];
    long long number_of_files = 0, current_file = 0, total_size = 0, done_size = 0;
    long long retries, bandwidth_full;
    long long i = 0;
    bool first = true;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))   // iterate through the target directory
    {
        if (!entry.is_directory())  // don't count directories
        {
            number_of_files++;  // total files
            total_size += entry.file_size();    // total bytes
        }
    }
    start_full = std::chrono::steady_clock::now();   // start the clock for the whole endevour
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))   // iterate through the target directory
    {
        retries = 0;
        if (!entry.is_directory())  // we only read files
        {
            current_file++;     // keep track of file number
            filenum_s = std::to_string(current_file) + "/" + std::to_string(number_of_files);   // format files done and total files
            file_size = entry.file_size();  // check our current file's size
            file2read.open(entry.path().string(), std::ios_base::in | std::ios::binary);    // open file for read-only, binary data
            if (!file2read.is_open())   // if the file didn't open
            {
                file2read.clear();  // clear the error.
            }
            else
            {
                while (true)                // file is open. let's loop forever. (we break when needed)
                {
                    first = true;
                    start = std::chrono::steady_clock::now();   // start the clock
                    start_display = start;
                    for (std::streamsize bytes = 0; bytes < file_size; bytes += block_size) // don't go past the end of the file
                    {
                        file2read.read(buffer_d, block_size);   // read a block
                        duration_s = seconds_f((long long)(elapsed_full = (std::chrono::steady_clock::now() - start_full)).count()); // format time spent
                        transferred_s = fsize_f(done_size += file2read.gcount()) + "/" + fsize_f(total_size);  // keep track of data transferred
                        bandwidth_full_s = fsize_f(bandwidth_full = bandwidth_calc(done_size, elapsed_full.count())) + "/sec";// filesize/time = b/s for the whole transfer
                        if (first || ((std::chrono::steady_clock::now() - start_display) > refresh_time))      // if this is the first run-through, or we've waited refresh_time
                        {
                            first = false;      // not our first run anymore, right?
                            start_display = std::chrono::steady_clock::now();       // start the clock again
                            do_output(entry.path().filename().string(), transferred_s, bandwidth_full_s, filenum_s, duration_s, retries); // update screen
                        }
                    }
                    elapsed_file = std::chrono::steady_clock::now() - start;
                    if (!check_reread(++retries, elapsed_file.count(), file_size))
                    {   // we don't retry more than five times, reach TARGET_BANDWIDTH or on small files. small files can be cached, but the open/close time makes the bandwidth numbers not match
                        break;
                    }
                    else    // we need to retry
                    {
                        done_size -= file_size; // keep bandwidth sane, remove the bytes we just read.
                        file2read.clear();  // clear the buffer's flags
                        file2read.seekg(std::ios_base::beg);     // seek to zero
                    }
                }
                file2read.close(); // done reading the file. close it
            }
        }
    }
    std::cout << "\n";  // we are done reading, so we don't need to ncurses anymore.

    return done_size;   // return the amount of data actually read
}

bool input_wait_for(long long timeout)       // wait for user input for timeout seconds
{
    std::time_t start = std::time(0);

    while(true)
    {
        if (std::difftime(std::time(0), start) >= timeout)      // check for timeout
        {
            return false;   // timeout reached before input
        }
        if (_kbhit())       // check if they keyboard was touched without needing an "Enter"
        {
            return true;    // input received before timeout
        }
    }
}

void parse_args(int argc, char **argv, long long &goal_time, std::string &path2read)      // parse command line arguments
{
    std::string temp;
    if (argc == 2)      // got a path, hopefully
    {
        path2read = argv[1];
        goal_time = 0; // we didn't get a goal time

        if (!std::filesystem::is_directory(path2read))      // not a path
        {
            std::cout << "Argument passed \"" << path2read << "\" is NOT a valid directory.\n\n";
            command_args();
            exit(2);
        }
    }
    else if (argc == 3)      // got a path and goal time, hopefully
    {
        path2read = argv[1];
        temp = argv[2];

        if (!std::filesystem::is_directory(path2read))  // argv[1] isn't a directory
        {
            path2read = argv[2];
            temp = argv[1];

            if (!std::filesystem::is_directory(path2read))  //argv[2] isn't a directory
            {
                std::cout << "Arguments passed \"" << path2read << "\" and \"" << temp << "\" are NOT valid directorys.\n\n";
                command_args();
                exit(2);
            }
            if (!is_number(argv[1]))    // argv[2] is a directory, but argv[1] isn't a valid number
            {
                if (temp == "/false" || temp == "/f") // don't stop
                {
                    goal_time = -1;
                    return;
                }
                std::cout << "Argument passed \"" << path2read << "\" is was a valid directory, BUT\n";
                std::cout << "Argument passed \"" << temp << "\" is NOT a valid number, / false or /f.\n\n";
                command_args();
                exit(2);
            }
            else
            {
                goal_time = std::stoi(temp);
            }
        }
        else if (!is_number(temp))   // argv[1] is a directory, but argv[2] isn't a valid number
        {
            if (temp == "/false" || temp == "/f") // don't stop
            {
                goal_time = -1;
                return;
            }
            std::cout << "Argument passed \"" << temp << "\" is NOT a valid number, /false or /f, while\n";
            std::cout << "Argument passed \"" << path2read << "\" is was a valid directory.\n\n";
            command_args();
            exit(2);
        }
        else
        {
            if (goal_time = std::stoi(temp) < 1)
            {
                goal_time = 0;
            }
        }
    }
    else        // one or two arguments, people!
    {
        command_args();
        exit(2);
    }

    return;
}

int main(int argc, char** argv)
{
    std::string path2read, bandwidth_s;
    std::chrono::steady_clock::time_point start;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds;
    long long transferred, bandwidth, goal_time = 0;
    
    parse_args(argc, argv, goal_time, path2read);

    while (true) // too many ways to end the loop for a simple conditional. sorry, but break it is
    {
        clear_line();   //ncurses or gui sometime soon, please

        start = std::chrono::steady_clock::now(); // start the clock
        transferred = do_read(path2read);   // do the reading, return value is bytes transferred
        elapsed_seconds = std::chrono::steady_clock::now() - start; // figure out how long it took
        bandwidth_s = fsize_f(bandwidth = bandwidth_calc(transferred, elapsed_seconds.count())) + "/sec";  // calculate bandwidth

        std::cout << "\"" << truncate(path2read, 22, true) << "\": read " << fsize_f(transferred) << " " << " in "
            << seconds_f((long long)elapsed_seconds.count()) << " at " << bandwidth_s;   // Output statistics

        if (check_goal_time((long long)elapsed_seconds.count(), goal_time))     // we got a goal_time, let's use it
        {
            std::cout << "; under goal of " << goal_time << " seconds. Press any key to continue.";    // great news. probably quit, right?
            if (!input_wait_for(20))  // they DIDN'T hit a key, time to go home
            {
                return 1;
            }
        }
        else if (check_bandwidth(bandwidth, goal_time)) // we read in SSD speed, on average
        {
            std::cout << ". Bandwidth goal reached.\n Press any key to continue.";    // great news. probably quit, right?
            if (!input_wait_for(20))  // they DIDN'T hit a key, time to go home
            {
                return 1;
            }
        }
        else
        {
            std::cout << ". Press any key to quit.";
            if (input_wait_for(5))  // they hit a key, time to go home.
            {
                return 1;
            }
        }
    }
    return 0;
}