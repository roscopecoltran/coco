/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include "loader.h"
#include "xml_creator.h"
#include "util/timing.h"
#include <boost/program_options.hpp>
//#include <boost/any.hpp>

class InputParser
{
public:
    InputParser(int argc, char ** argv)
        : argc_(argc), argv_(argv)
    {
        
        description_.add_options()
                ("help,h", "Print help message and exit.")
                ("config_file,x", boost::program_options::value<std::string>(),
                        "Xml file with the configurations of the application.")
                ("profiling,p", boost::program_options::value<int>()->implicit_value(5),
                    "Enable the collection of statistics of the executions. Use only during debug as it slow down the performances.")
                ("graph,g", boost::program_options::value<std::string>(),
                        "Create the graph of the varius components and of their connections.")
                ("lib,l", boost::program_options::value<std::string>(), 
                        "Print the xml template for all the components contained in the library.");

        boost::program_options::store(boost::program_options::command_line_parser(argc_, argv_).
                options(description_).run(), vm_);
        boost::program_options::notify(vm_);
    }

    bool get(std::string value)
    {
        if(vm_.count(value))
            return true;
        return false;
    }

    std::string getString(std::string value)
    {
        if(vm_.count(value))
            return vm_[value].as<std::string>();
        return "";
    }

    float getFloat(std::string value)
    {
        if(vm_.count(value))
            return vm_[value].as<float>();
        return std::numeric_limits<double>::quiet_NaN();
    }

    int getInt(std::string value)
    {
        if (vm_.count(value))
            return vm_[value].as<int>();
        return std::numeric_limits<int>::quiet_NaN();
    }
    void print()
    {
        std::cout << description_ << std::endl;
    }

    void print(std::ostream & os)
    {
        description_.print(os);
    }

private:
    int argc_;
    char ** argv_;
    boost::program_options::options_description description_ = {"Universal app to call any coco applications.\n"
                                                                "Usage: coco_launcher [options]\n"
                                                                "Use COCO_PREFIX_PATH to specify lookup paths for resources and libraries\n"
                                                            };
    boost::program_options::variables_map vm_;
};

coco::CocoLauncher *launcher = {nullptr};
std::atomic<bool> stop_execution = {false};
std::mutex launcher_mutex, statistics_mutex;
std::condition_variable launcher_condition_variable, statistics_condition_variable;

void handler(int sig)
{
  void *array[10];
  size_t size;

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void terminate(int sig)
{
    if (launcher)
        launcher->killApp();

    stop_execution = true;
    launcher_condition_variable.notify_all();
    statistics_condition_variable.notify_all();
}

void printStatistics(int interval)
{
    while(!stop_execution)
    {
        COCO_PRINT_ALL_TIME

        std::unique_lock<std::mutex> mlock(statistics_mutex);
        statistics_condition_variable.wait_for(mlock, std::chrono::seconds(interval));
    }
}

void launchApp(std::string confing_file_path, bool profiling, const std::string &graph)
{
    launcher = new coco::CocoLauncher(confing_file_path.c_str());
    launcher->createApp(profiling);
    if (!graph.empty())
        launcher->createGraph(graph);

    launcher->startApp();

    COCO_LOG(0) << "Application is running!";

    std::unique_lock<std::mutex> mlock(launcher_mutex);
    launcher_condition_variable.wait(mlock);
}

int main(int argc, char **argv)
{
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGINT, terminate);

    InputParser options(argc, argv);

    std::string config_file = options.getString("config_file");
    if (!config_file.empty())
    {
        std::thread statistics;
        
        bool profiling = options.get("profiling");
        if (profiling)
        {
            int interval = options.getInt("profiling");
            statistics = std::thread(printStatistics, interval);
        }
        
        std::string graph = options.getString("graph");;
        
        launchApp(config_file, profiling, graph);

        if (statistics.joinable())
        {
            statistics.join();
        }
        return 0;
    }

    std::string library_name = options.getString("lib");
    if (!library_name.empty())
    {
        COCO_INIT_LOG();
        coco::printXMLSkeletonLibrary(library_name, "", true, true);
        return 0;
    }

    if (options.get("help"))
    {
        options.print();
        return 0;
    }

    options.print();
    return 1;
}
