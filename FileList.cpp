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

void do_read(std::string path)
{
    std::ifstream file2read;
    int i;
    unsigned __int64 number_of_files = 0, current_file = 0;
    std::chrono::steady_clock::time_point start;
    std::streamsize file_size = 0;
    int speed = 0;
    bool loop = true;
    std::streamsize block_size = (std::streamsize)BLOCK_SIZE;
    std::string speed_s, loop_s, filenum_s;
    char* buffer_d = new char[(size_t)BLOCK_SIZE];
    size_t fieldWidth[4] = { 40, 15, 15, 20 };
    std::chrono::duration<double> elapsed_seconds;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        number_of_files++;
    }
    number_of_files++;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
    {
        current_file++;
        speed = 0;
        i = 0;
        loop = true;
        if (!entry.is_directory())
        {
            file_size = entry.file_size();
            file2read.open(entry.path().string(), std::ios_base::in | std::ios::binary);
            if (!file2read.is_open())
            {
                file2read.clear();
            }
            else while (loop == true)
            {
                
                std::cout << std::setw(fieldWidth[0]) << std::left << entry.path().filename().string();
                std::cout << std::setw(fieldWidth[1]) << std::right << fsize_f(file_size);
                start = std::chrono::high_resolution_clock::now();
                for (std::streamsize i = 0; i < file_size; i += block_size)
                {
                    file2read.read(buffer_d, block_size);
                }
                elapsed_seconds = std::chrono::high_resolution_clock::now() - start;
                speed = (int)(file_size / (pow(2, 20) * elapsed_seconds.count()));
                speed_s = std::to_string(speed) + "MB/sec";
                std::cout << std::setw(fieldWidth[1]) << std::right << speed_s;
                loop_s = "Try [" + std::to_string(++i) + "/5]";
                std::cout << std::setw(fieldWidth[2]) << std::right << loop_s;
                filenum_s = "File [" + std::to_string(current_file) + "/" + std::to_string(number_of_files) + "]";
                std::cout << std::setw(fieldWidth[3]) << std::right << filenum_s << "\n";
                if ((i >= 5) || (speed >= TARGET_SPEED))
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
        }
    }
    return;
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

int main()
{
    std::string path2read = "D:\\SquareEnix";

    while (true)
    {
        std::cout << "Reading " << path2read << ", block size " << fsize_f((std::streamsize)BLOCK_SIZE) << ", goal speed " << TARGET_SPEED << "\n";
        do_read(path2read);
        std::cout << "Press any key to NOT continue.\n";
        if (input_wait_for(5))
        {
            return 1;
        }
    }
    return 0;
}