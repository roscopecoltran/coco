#include <chrono>
#include <thread>
#include <coco/coco.h>

class TaskLatStart : public coco::TaskContext
{
public:
    coco::OutputPort<int long> out_time_ = {this, "time_OUT"};
    coco::Attribute<int long> asleep_time_ = {this, "sleep_time", sleep_time_};

    void init() {}
    void onConfig() {}

    void onUpdate()
    {
        auto time = coco::util::time();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));

        out_time_.write(time);
    }
private:
    int long sleep_time_ = 10;
};

COCO_REGISTER(TaskLatStart)

class TaskLatMiddle : public coco::TaskContext
{
public:
    coco::InputPort<int long> in_time_ = {this, "time_IN", true};
    coco::OutputPort<int long> out_time_ = {this, "time_OUT"};

    coco::Attribute<int long> asleep_time_ = {this, "vec_length", vec_length_};

    void init()
    {
        vec_.resize(vec_length_);
        std::iota(vec_.begin(), vec_.end(), 0);
    }
    void onConfig() {}

    void onUpdate()
    {
        int long time;
        in_time_.read(time);

        /*
         * 100000 . 45ms
         *  50000 - 30ms
         * 100000 - 15ms
         * 10000  -  6ms
         */

        //std::random_shuffle(vec_.begin(), vec_.end());
        std::sort(vec_.begin(), vec_.end());

        out_time_.write(time);
    }
private:
    int long vec_length_ = 10;

    std::vector<double> vec_;

};

COCO_REGISTER(TaskLatMiddle)


class TaskLatSink : public coco::TaskContext
{
public:
    coco::InputPort<int long> in_time_ = {this, "time_IN", true};
    coco::Attribute<int long> asleep_time_ = {this, "sleep_time", sleep_time_};

    void init() {}
    void onConfig() {}

    void onUpdate()
    {
        int long time;
        in_time_.read(time);

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));


        latency_ += coco::util::time() - time;
        static int count = 1;
        if (count++ % 10 == 0)
        {
            COCO_LOG(1) << "Latency: " << latency_ / count;
        }
    }
private:

    long latency_ = 0;
    int long sleep_time_ = 10;
};

COCO_REGISTER(TaskLatSink)