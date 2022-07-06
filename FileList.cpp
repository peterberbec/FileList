#undef _DEBUG

#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <conio.h>
#define BLOCK_SIZE (pow(2, 25))
#define TARGET_SPEED 350
#define FIELD_WIDTH {36,20,12,14,7} /* filename, size, speed, filenumber, time*/

void command_args();
bool is_number(const std::string&);
std::string truncate(std::string, size_t, bool);
void clear_line(int);
std::string seconds_f(int);
std::string fsize_f(std::streamsize);
unsigned __int64 do_read(std::string);
bool input_wait_for(int);

void command_args()
{
    std::cout << "One argument is required - the directory to be scanned.\n";
    std::cout << "One argument is optional - the directory goal time to quit after, in seconds.\n";
    std::cout << "Argument order is not important.\n\n";
    std::cout << "Example A:\n\n";
    std::cout << "C:\\> filelist.exe \"C:\\Directory\"\n";
    std::cout << "Example B:\n\n";
    std::cout << "C:\\> filelist.exe 64 \"C:\\Directory\"\n";
    input_wait_for(15);
}

bool is_number(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::string truncate(std::string str, size_t width, bool dots=false)
{
    if(dots)
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

void clear_line(int number = 0)
{
    char to_print[3] = { '\b', ' ', '\b' };
    size_t fieldWidth[5] = FIELD_WIDTH;
    size_t width_f = fieldWidth[0] + fieldWidth[1] + fieldWidth[2] + fieldWidth[3] + fieldWidth[4] + 3;
    if (number != 0)
    {
        width_f = size_t(number);
    }

    for (unsigned int j = 0; j < 3; j++)
    {
        for (unsigned int i = 0; i < width_f; i++)
        {
            std::cout << to_print[j];
        }
    }
    return;
}

std::string seconds_f(int num_seconds)
{
    std::string ret_string = "";

    if (num_seconds < 600)
    {
        std::stringstream ret;
        ret << std::fixed << std::setprecision(2) << (num_seconds/10.0);
        ret_string = ret.str() + "s";
        return ret_string;
    }
    if (num_seconds > 864000)
    {
        ret_string = std::to_string((int)(num_seconds / 864000)) + "d";
        num_seconds -= (int)((num_seconds / 86400) * 864000);
    }
    if (num_seconds > 36000)
    {
        ret_string = ret_string + std::to_string((int)(num_seconds / 36000)) + "h";
        num_seconds -= (int)((num_seconds / 3600) * 3600);
    }
    if (num_seconds > 600)
    {
        ret_string = ret_string + std::to_string((int)(num_seconds / 600)) + "m";
        num_seconds -= (int)((num_seconds / 60) * 60);
    }
    ret_string = ret_string + std::to_string((int)(num_seconds/10)) + "s";

    return ret_string;
}

std::string fsize_f(std::streamsize number)
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

unsigned __int64 do_read(std::string path)
{
    std::ifstream file2read;
    std::chrono::steady_clock::time_point start, start_out;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds, elapsed_out;
    std::streamsize file_size = 0, block_size = (std::streamsize)BLOCK_SIZE;
    std::string bandw_s, speed_s, filenum_s, duration_s;
    char* buffer_d = new char[(size_t)BLOCK_SIZE];
    unsigned __int64 number_of_files = 0, current_file = 0, total_size = 0, done_size = 0;
    size_t fieldWidth[5] = FIELD_WIDTH;
    bool loop = true;
    int i, speed;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        if (!entry.is_directory())
        {
            number_of_files++;
            total_size += entry.file_size();
        }
    }
    start_out = std::chrono::steady_clock::now();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        speed = 0;
        i = 0;
        loop = true;
        if (!entry.is_directory())
        {
            current_file++;
            file_size = entry.file_size();
            file2read.open(entry.path().string(), std::ios_base::in | std::ios::binary);
            if (!file2read.is_open())
            {
                file2read.clear();
            }
            else while (loop == true)
            {                
                bandw_s = fsize_f(file_size) + "/" + fsize_f(done_size) + "/" + fsize_f(total_size);
                start = std::chrono::steady_clock::now();
                for (std::streamsize bytes = 0; bytes < file_size; bytes += block_size)
                {
                    file2read.read(buffer_d, block_size);
                }
                elapsed_seconds = std::chrono::steady_clock::now() - start;
                elapsed_out = std::chrono::steady_clock::now() - start_out;
                speed = (int)((file_size * 10) / (pow(2, 20) * elapsed_seconds.count()));
                speed_s = std::to_string(speed) + "MB/sec";
                filenum_s = std::to_string(current_file) + "/" + std::to_string(number_of_files);
                duration_s = seconds_f((int)elapsed_out.count());
                clear_line();
                std::cout << "[";
                std::cout << std::setw(fieldWidth[0]) << std::left << truncate(entry.path().filename().string(), fieldWidth[0], true);
                std::cout << std::setw(fieldWidth[1]) << std::right << truncate(bandw_s, fieldWidth[1]);
                std::cout << std::setw(fieldWidth[2]) << std::right << truncate(speed_s, fieldWidth[2]);
                std::cout << std::setw(fieldWidth[3]) << std::right << truncate(filenum_s, fieldWidth[3]);
                switch (i)
                {
                    case 0: std::cout << " "; break;
                    case 1: std::cout << "-"; break;
                    case 2: std::cout << "="; break;
                    case 3: std::cout << "*"; break;
                    case 4: std::cout << "#"; break;
                }
                std::cout << std::setw(fieldWidth[4]) << std::right << duration_s;
                std::cout << "]";
                if ((++i >= 5) || (speed >= TARGET_SPEED) || (file_size < 10000))
                {
                    break;
                }
                else
                {
                    file2read.clear();
                    file2read.seekg(0, std::ios_base::beg);
                }
            }
            file2read.close();
            done_size += file_size;
        }
    }
    std::cout << "\n";

    return total_size;
}

bool input_wait_for(int timeout)
{
    std::time_t start = std::time(0);

    while(true)
    {
        if (std::difftime(std::time(0), start) >= timeout)
        {
            return false;
        }
        if (_kbhit()) 
        {
            return true;
        }
    }
}

int main(int argc, char** argv)
{
    std::string path2read, speed_s, goal_time_s = "none";
    std::chrono::steady_clock::time_point start;
    std::chrono::duration<double, std::ratio<1, 10>> elapsed_seconds;
    unsigned __int64 transferred, goal_time = 0;

    if (argc == 1 || argc > 3)
    {
        command_args();
        return 2;
    }

    if (argc == 2)
    {
        path2read = argv[1];

        if (!std::filesystem::is_directory(path2read))
        {
            std::cout << "Argument passed \"" << path2read << "\" is not a valid directory.\n\n";
            command_args();
            return 2;
        }
    }
    if (argc == 3)
    {
        path2read = argv[1];
        goal_time_s = argv[2];

        if (!std::filesystem::is_directory(path2read))
        {
            path2read = argv[2];
            if (!std::filesystem::is_directory(path2read))
            {
                std::cout << "Argument passed \"" << path2read << "\" is not a valid directory.\n\n";
                command_args();
                return 2;
            }
            goal_time_s = argv[1];
            if (!is_number(goal_time_s))
            {
                std::cout << "Argument passed \"" << goal_time_s << "\" is not a valid number.\n\n";
                command_args();
                return 2;
            }
        }
        else if (!is_number(goal_time_s))
        {
            std::cout << "Argument passed \"" << goal_time_s << "\" is not a valid number.\n\n";
            command_args();
            return 2;
        }
    }
    if (goal_time_s != "none")
    {
        goal_time = std::stoi(goal_time_s);
    }
    else
    {
        goal_time = 604800000;
    }

    while (true)
    {
        clear_line();
        start = std::chrono::steady_clock::now();
        transferred = do_read(path2read);

        elapsed_seconds = std::chrono::steady_clock::now() - start;
        speed_s = std::to_string((int)((transferred * 10) / (pow(2, 20) * elapsed_seconds.count()))) + "MB/sec";
        std::cout << "\"" << truncate(path2read, 22, true) << "\": read " << fsize_f(transferred) << " in " << seconds_f((int)elapsed_seconds.count()) << " at " << speed_s;
        if ((elapsed_seconds.count()/10) < goal_time)
        {
            std::cout << ".\nData read in under " << goal_time_s << ". Quitting.\n";
            return 1;
        }
        std::cout << ". Press any key to quit.";
        if (input_wait_for(5))
        {
            return 1;
        }
    }
    return 0;
}