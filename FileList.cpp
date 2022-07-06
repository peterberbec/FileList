#undef _DEBUG

#include <iostream>                 // everybody need std::cout
#include <string>                   // for string stuff
#include <filesystem>               // for filesystem
#include <fstream>                  // for std::basic_ifstream
#include <chrono>                   // for time stuff
#include <sstream>                  // for std::stringstream
#include <conio.h>                  // for _kbhit

#define BLOCK_SIZE (pow(2, 25))     // 32MB works well on my system. negligable bandwidth gains for higher memory footprint. lower is slower
#define TARGET_BANDWIDTH 350            // 350MB/sec means it hit the cache
#define FIELD_WIDTH {36,20,12,14,7} // filename, size, bandwidth, filenumber, time - used for std::setw formatting

void command_args();
bool is_number(const std::string&);
std::string truncate(std::string, size_t, bool);
void clear_line(unsigned __int64);
std::string seconds_f(unsigned __int64);
std::string fsize_f(std::streamsize);
unsigned __int64 do_read(std::string);
bool input_wait_for(unsigned __int64);

void command_args()                 // Help text
{
    std::cout << "FileList.exe\n";
    std::cout << "\n";
    std::cout << "Reads all file, recursively, in target directory. Loops forever, with user prompt\n";
    std::cout << "to quit after every loop, or until read is accomplished in goal time.\n";
    std::cout << "\n";
    std::cout << "The purpose of this program is to load an SSD cache from a hard drive. I wrote it\n";
    std::cout << "to load PrimoCache with FF14, as it was doing a bad job of loading. It grew from there\n";
    std::cout << "and now can run on any directory. I've integrated it into my Windows \"Send-To\" menu.\n";
    std::cout << "\n";
    std::cout << "One argument is required - the directory to be scanned. Put it in Quotes to make\n";
    std::cout << "life easier - long - filenames and spaces in directroy names.\n";
    std::cout << "One argument is optional - the directory goal time to quit after, in seconds.\n";
    std::cout << "Argument order is not important.\n";
    std::cout << "\n";
    std::cout << "Example A:\n";
    std::cout << "\n";
    std::cout << "C:\\> filelist.exe \"C:\\Directory\"\n";
    std::cout << "\n";
    std::cout << "Example B:\n";
    std::cout << "\n";
    std::cout << "C:\\> filelist.exe 64 \"D:\\Directory\\With Spaces\"\n";
    input_wait_for(15);
}

bool is_number(const std::string& s)    // Test to see if s is a number
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::string truncate(std::string str, size_t width, bool dots=false)        // truncate a string if longer than width characters
{
    if(dots)    // add an elipsis if asked to
    {
        if ((str.length()+3) >= width)
        {
            return str.substr(0, width-4) + "...";
        }
    }
    else if (str.length() >= width)
    {
        return str.substr(0, width);
    }
    return str;
}

void clear_line(unsigned __int64 number = 0)        // curses is hard. hax are easy
{
    char to_print[3] = { '\b', ' ', '\b' };
    size_t fieldWidth[5] = FIELD_WIDTH;
    unsigned __int64 width_f = (unsigned __int64)fieldWidth[0] + (unsigned __int64)fieldWidth[1] \
        + (unsigned __int64)fieldWidth[2] + (unsigned __int64)fieldWidth[3] + (unsigned __int64)fieldWidth[4] + 3;      // calculate the length of the line
    if (number != 0)        // if we get an argument, clear that number of spaces
    {
        width_f = size_t(number);
    }

    for (unsigned __int64 j = 0; j < 3; j++)    // loop through backspace, space and backspace
    {
        for (unsigned __int64 i = 0; i < width_f; i++)
        {
            std::cout << to_print[j];
        }
    }
    return;
}

std::string seconds_f(unsigned __int64 num_seconds)     // format a number of seconds into 3d12h5m17s
{
    std::string ret_string = "";
    unsigned __int64 temp = 0;

    if (num_seconds < 600)      // if we are given less than 60 seconds, use a decimal place
    {
        temp = (unsigned __int64)(num_seconds / 10);
        std::stringstream temp_ss;
        temp_ss << std::fixed << std::setprecision(2) << (temp);
        ret_string = temp_ss.str() + "s";
        return ret_string;
    }
    if (num_seconds > 864000)   // days
    {
        temp = (unsigned __int64)(num_seconds / 864000);
        ret_string = std::to_string(temp) + "d";
        num_seconds -= temp * 864000;
    }
    if (num_seconds > 36000)    // hours
    {
        temp = (unsigned __int64)(num_seconds / 36000);
        if (temp < 10 && ret_string != "")      // if less than 10 hours, and we are using days, add a leading 0
        {
            ret_string = "0" + ret_string + std::to_string(temp) + "h";
        }
        else
        {
            ret_string = ret_string + std::to_string(temp) + "h";
        }
        num_seconds -= temp * 3600;
    }
    if (num_seconds > 600)      // minutes
    {
        temp = (unsigned __int64)(num_seconds / 600);
        if (temp < 10 && ret_string != "")      // if less than 10 minutes, and we are using hours, add a leading 0
        {
            ret_string = "0" + ret_string + std::to_string(temp) + "m";
        }
        else
        {
            ret_string = ret_string + std::to_string(temp) + "m";
        }
        num_seconds -= temp * 600;
    }
    temp = (unsigned __int64)(num_seconds / 10);
    if (temp < 10 && ret_string != "")  // if less than 10 seconds, and we are using minutes, add a leading 0
    {
        ret_string = "0" + ret_string + std::to_string(temp) + "s";
    }
    else
    {
        ret_string = ret_string + std::to_string(temp) + "s";
    }

    return ret_string;
}

std::string fsize_f(std::streamsize number)     // format number of bytes to B, KB, MB, GB, TB and PB
{
    std::stringstream number_fs;

    if (number < pow(2, 10))
    {
        number_fs << std::fixed << std::setprecision(0) << (number / pow(2, 00)) <<  "B";
        return number_fs.str();
    }
    else if (number < pow(2, 20))
    {
        number_fs << std::fixed << std::setprecision(1) << (number / pow(2, 10)) << "KB";
        return number_fs.str();
    }
    else if (number < pow(2, 30))
    {
        number_fs << std::fixed << std::setprecision(1) << (number / pow(2, 20)) << "MB";
        return number_fs.str();
    }
    else if (number < pow(2, 40))
    {
        number_fs << std::fixed << std::setprecision(1) << (number / pow(2, 30)) << "GB";
        return number_fs.str();
    }
    else if (number < pow(2, 50))
    {
        number_fs << std::fixed << std::setprecision(1) << (number / pow(2, 40)) << "TB";
        return number_fs.str();
    }
    else
    {
        number_fs << std::fixed << std::setprecision(1) << (number / pow(2, 50)) << "PB";
        return number_fs.str();
    }
}

unsigned __int64 do_read(std::string path)      // the big read function
{
    std::ifstream file2read;
    std::chrono::steady_clock::time_point start, start_out;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds, elapsed_full;     // std::ratio<1, 10> means time is kept in MILLI-seconds, so we need to /10 here and there
    std::streamsize file_size = 0, block_size = (std::streamsize)BLOCK_SIZE;
    std::string transferred_s, bandwidth_full_s, filenum_s, duration_s;
    char* buffer_d = new char[(size_t)BLOCK_SIZE];
    unsigned __int64 number_of_files = 0, current_file = 0, total_size = 0, done_size = 0;
    size_t fieldWidth[5] = FIELD_WIDTH;
    unsigned __int64 retries, bandwidth, bandwidth_full;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))   // iterate through the target directory
    {
        if (!entry.is_directory())  // we only count files
        {
            number_of_files++;  // total files
            total_size += entry.file_size();    // total bytes
        }
    }
    start_out = std::chrono::steady_clock::now();   // start the clock for the whole endevour
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))   // iterate through the target directory
    {
        retries = 0;
        if (!entry.is_directory())  // we only read files
        {
            current_file++;     // keep track of file number
            file_size = entry.file_size();  // check our current file's size
            file2read.open(entry.path().string(), std::ios_base::in | std::ios::binary);    // open file for read-only, binary data
            if (!file2read.is_open())   // if the file didn't open
            {
                file2read.clear();  // clear the error. since we skip doing anything else, we can now reuse safely
            }
            else
            {
                done_size += file_size;     // keep track of total data transferred.
                while (true)                // file is open. let's loop forever. (we break when needed)
                {
                    transferred_s = fsize_f(file_size) + "/" + fsize_f(done_size) + "/" + fsize_f(total_size);  // keep track of data transferred
                    start = std::chrono::steady_clock::now();   // start the clock
                    for (std::streamsize bytes = 0; bytes < file_size; bytes += block_size) // don't go past the end of the file
                    {
                        file2read.read(buffer_d, block_size);   // read a block
                    }
                    elapsed_seconds = std::chrono::steady_clock::now() - start; // time of current transfer
                    elapsed_full = std::chrono::steady_clock::now() - start_out;    // total time
                    /*
                    bandwidth = (unsigned __int64)((file_size * 10) / (pow(2, 20) * elapsed_seconds.count()));  // filesize/time = b/s
                    bandwidth_s = std::to_string(bandwidth) + "MB/sec";     // format bandwidth
                    */
                    bandwidth = (unsigned __int64)((file_size * 10) / elapsed_seconds.count());  // filesize/time = b/s for the current file
//                  bandwidth_s = fsize_f(bandwidth) + "/sec";
                    bandwidth_full = (unsigned __int64)((done_size * 10) / elapsed_full.count());  // filesize/time = b/s for the whole transfer
                    bandwidth_full_s = fsize_f(bandwidth_full) + "/sec";
                    /**/
                    filenum_s = std::to_string(current_file) + "/" + std::to_string(number_of_files);   // format files done and total files
                    duration_s = seconds_f((unsigned __int64)elapsed_full.count()); // format time spent
                    clear_line();   // ncurses is hard
                    std::cout << "[";   // setw lets us use columns
                    std::cout << std::setw(fieldWidth[0]) << std::left << truncate(entry.path().filename().string(), fieldWidth[0], true);  // filename, truncated, without path
                    std::cout << std::setw(fieldWidth[1]) << std::right << truncate(transferred_s, fieldWidth[1]);                          // transferred formatted
                    std::cout << std::setw(fieldWidth[2]) << std::right << truncate(bandwidth_full_s, fieldWidth[2]);                            // bandwidth formatted
                    std::cout << std::setw(fieldWidth[3]) << std::right << truncate(filenum_s, fieldWidth[3]);                              // files formatted
                    switch (retries)    // depending on retries, add a character
                    {
                    case 0: std::cout << " "; break;
                    case 1: std::cout << "-"; break;
                    case 2: std::cout << "="; break;
                    case 3: std::cout << "*"; break;
                    case 4: std::cout << "#"; break;
                    }
                    std::cout << std::setw(fieldWidth[4]) << std::right << duration_s;
                    std::cout << "]";
                    if ((++retries >= 5) || (bandwidth >= TARGET_BANDWIDTH) || (file_size < 10000))
                    {   // we don't retry more than five times, reach TARGET_BANDWIDTH or on small files. small files can be cached, but the open/close time makes the bandwidth numbers not match
                        break;
                    }
                    else    // we need to retry
                    {
                        file2read.clear();  // clear the buffer's flags
                        file2read.seekg(0, std::ios_base::beg);     // seek to zero
                    }
                }
                file2read.close(); // done reading the file. close it
            }
        }
    }
    std::cout << "\n";  // we are done reading, so we don't need to ncurses anymore.

    return done_size;   // return the data actually read
}

bool input_wait_for(unsigned __int64 timeout)       // wait for user input for timeout seconds
{
    std::time_t start = std::time(0);

    while(true)
    {
        if (std::difftime(std::time(0), start) >= timeout)      // check for timeout
        {
            return false;
        }
        if (_kbhit())       // check if they keyboard was touched without needing an "Enter"
        {
            return true;
        }
    }
}

int main(int argc, char** argv)
{
    std::string path2read, bandwidth_s, goal_time_s = "none";
    std::chrono::steady_clock::time_point start;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds;
    unsigned __int64 transferred, goal_time = 0, bandwidth;

    if (argc == 1 || argc > 3)  // one or two arguments, people!
    {
        command_args();
        return 2;
    }

    if (argc == 2)      // got a path, hopefully
    {
        path2read = argv[1];

        if (!std::filesystem::is_directory(path2read))      // not a path
        {
            std::cout << "Argument passed \"" << path2read << "\" is NOT a valid directory.\n\n";
            command_args();
            return 2;
        }
    }
    if (argc == 3)      // got a path and goal time, hopefully
    {
        path2read = argv[1];
        goal_time_s = argv[2];

        if (!std::filesystem::is_directory(path2read))  // argv[1] isn't a directory
        {
            path2read = argv[2];
            if (!std::filesystem::is_directory(path2read))  //argv[2] isn't a directory
            {
                std::cout << "Argument passed \"" << path2read << "\" is NOT a valid directory.\n\n";
                command_args();
                return 2;
            }
            goal_time_s = argv[1];
            if (!is_number(goal_time_s))    // argv[2] is a directory, but argv[1] isn't a valid number
            {
                std::cout << "Argument passed \"" << path2read << "\" is was a valid directory, BUT\n";
                std::cout << "Argument passed \"" << goal_time_s << "\" is NOT a valid number.\n\n";
                command_args();
                return 2;
            }
        }
        else if (!is_number(goal_time_s))   // argv[1] is a directory, but argv[2] isn't a valid number
        {
            std::cout << "Argument passed \"" << path2read << "\" is was a valid directory, BUT\n";
            std::cout << "Argument passed \"" << goal_time_s << "\" is NOT a valid number.\n\n";
            command_args();
            return 2;
        }
    }
    if (goal_time_s != "none")  // we got a goal_time, let's use it
    {
        goal_time = std::stoi(goal_time_s);
    }

    while (true)
    {
        clear_line();

        start = std::chrono::steady_clock::now(); // start the clock
        transferred = do_read(path2read);   // do the reading
        elapsed_seconds = std::chrono::steady_clock::now() - start; // figure out how long it took
        bandwidth = (unsigned __int64)((transferred * 10) / elapsed_seconds.count()); // calculate transfer speed
        bandwidth_s = fsize_f(bandwidth) + "/sec";  // pretty it up
        std::cout << "\"" << truncate(path2read, 22, true) << "\": read " << fsize_f(transferred) << " in " \
            << seconds_f((unsigned __int64)elapsed_seconds.count()) << " at " << bandwidth_s;   // Output statistics

        if (goal_time_s != "none" && ((elapsed_seconds.count()/10) < goal_time))     // we got a goal_time, let's use it
        {
            std::cout << "; under goal of " << goal_time_s << "seconds. Quitting.\n";
            return 1;
        }
        if (bandwidth > TARGET_BANDWIDTH) // we read in SSD speed, on average
        {
            std::cout << ". Bandwidth goal reached. Press any key to continue.";    // great news. probably quit, right?
            if (!input_wait_for(5))  // they DIDN'T hit a key, time to go home
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