#include <vector>
#include <string>

class Process{
public:
    Process(unsigned int id, unsigned int time_arrival, unsigned int time_job);
    Process(unsigned int id, unsigned int time_arrival, unsigned int time_job,
            unsigned int qt_job, unsigned int time_deadline, unsigned int priority);

    static unsigned int get_id(unsigned int index);
    static unsigned int get_time_arrival(unsigned int index);
    static unsigned int get_time_job(unsigned int index);
    static unsigned int get_quantity_job(unsigned int index);
    static unsigned int get_time_deadline(unsigned int index);
    static unsigned int get_priority(unsigned int index);
    static unsigned int get_missed(unsigned int index);
    static unsigned int get_missed_deadline();
    static unsigned int get_period(unsigned int index);
    static bool is_schedulable(unsigned int index);
    static bool is_batch_only();

    static unsigned int get_time_waiting(unsigned int index);
    static unsigned int get_time_turnaround(unsigned int index);
    static unsigned int get_list_size();
    static double get_time_turnaround_average();

    static void append(unsigned int id, unsigned int time_arrival, unsigned int time_job);
    static void append(unsigned int id, unsigned int time_arrival, unsigned int time_job,
                       unsigned int qt_job, unsigned int time_deadline, unsigned int priority);
    static void load_from_file(std::string filename);

    static void fcfs(std::ostringstream& log);
    static void sjf(std::ostringstream& log);
    static void sjf_preempt(std::ostringstream& log);
    static void rt_rms(std::ostringstream& log, unsigned int max_limit);

private:
    unsigned int id;
    unsigned int time_arrival;
    unsigned int time_job;
    unsigned int qt_job;
    unsigned int time_deadline;
    unsigned int priority;
    unsigned int period;

    unsigned int time_available;
    unsigned int time_waiting;
    unsigned int time_processing;
    unsigned int time_remaining;
    unsigned int time_turnaround;
    unsigned int missed;

    bool schedulable;

    static std::vector<Process> P;
    static unsigned int time_turnaround_total;
    static unsigned int time_cpu;
    static bool batch_only;

    static unsigned int missed_deadline;
    static unsigned int not_schedulable;

    static bool sort_by_id(Process a, Process b);
    static bool sort_by_arrival(Process a, Process b);
    static bool sort_by_remaining(Process a, Process b);
    static bool sort_by_period(Process a, Process b);

    static unsigned long findlcm();

    static unsigned long gcd(unsigned long period, unsigned long lcm);
};
