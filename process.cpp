#include "process.h"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <numeric>

const std::vector<std::string> explode(const std::string& s, const char& c)
{
    std::string buff{""};
    std::vector<std::string> v;

    for(auto n:s)
    {
        if(n != c) buff+=n; else
        if(n == c && buff != "") { v.push_back(buff); buff = ""; }
    }
    if(buff != "") v.push_back(buff);

    return v;
}

bool isInteger(const std::string &s)
{
    if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

    char * p ;
    strtol(s.c_str(), &p, 10) ;

    return (*p == 0);
}


std::vector<Process> Process::P;
unsigned int Process::time_turnaround_total = 0;
unsigned int Process::time_cpu = 0;
bool Process::batch_only = false;
unsigned int Process::missed_deadline = 0;
unsigned int Process::not_schedulable = 0;

Process::Process(unsigned int id, unsigned int time_arrival, unsigned int time_job) : id(id),
                                                                                      time_arrival(time_arrival),
                                                                                      time_job(time_job)

{
    this->time_available = time_arrival;
    this->time_remaining = time_job;
    this->time_processing = 0;
    this->time_waiting = 0;
    this->time_turnaround = 0;
    this->missed = 0;
    schedulable = true;
}

Process::Process(unsigned int id, unsigned int time_arrival, unsigned int time_job, unsigned int qt_job,
                 unsigned int time_deadline, unsigned int priority) : id(id),
                                                                      time_arrival(time_arrival),
                                                                      time_job(time_job),
                                                                      qt_job(qt_job),
                                                                      time_deadline(time_deadline),
                                                                      priority(priority)
{
    this->time_available = time_arrival;
    this->time_remaining = time_job;
    this->time_processing = 0;
    this->time_waiting = 0;
    this->time_turnaround = 0;
    this->missed = 0;

    if(time_deadline < time_job) {
        schedulable = false;
        not_schedulable++;
        period = (unsigned int)-1;
    } else {
        schedulable = true;
        period = qt_job*time_deadline;
    }
}

unsigned int Process::get_id(unsigned int index)
{
    return P[index].id;
}

unsigned int Process::get_time_arrival(unsigned int index)
{
    return P[index].time_arrival;
}

unsigned int Process::get_time_job(unsigned int index)
{
    return P[index].time_job;
}

unsigned int Process::get_time_waiting(unsigned int index)
{
    return P[index].time_waiting;
}

unsigned int Process::get_time_turnaround(unsigned int index)
{
    return P[index].time_turnaround;
}

unsigned int Process::get_priority(unsigned int index)
{
    return P[index].priority;
}

unsigned int Process::get_quantity_job(unsigned int index)
{
    return P[index].qt_job;
}

double Process::get_time_turnaround_average()
{
    return (double)time_turnaround_total / (double)P.size();
}

unsigned int Process::get_list_size()
{
    return (unsigned int)P.size();
}

bool Process::is_batch_only() {
    return batch_only;
}

void Process::append(unsigned int id, unsigned int time_arrival, unsigned int time_job)
{
    Process aux(id, time_arrival, time_job);
    P.push_back(aux);
}

void Process::append(unsigned int id, unsigned int time_arrival, unsigned int time_job, unsigned int qt_job,
                     unsigned int time_deadline, unsigned int priority)
{
    Process aux(id, time_arrival, time_job, qt_job, time_deadline, priority);
    P.push_back(aux);
}

void Process::load_from_file(std::string filename)
{
    P.clear();

    std::fstream file;
    file.open(filename, std::fstream::in);
    if(!file.is_open())
        throw std::runtime_error("Impossível abrir arquivo.");

    std::string buffer;
    std::getline(file, buffer);

    std::vector<std::string> buffer_split = explode(buffer, ' ');
    if(buffer_split.size() == 3)
        batch_only = true;
    else if(buffer_split.size() == 6)
        batch_only = false;
    else{
        file.close();
        throw std::runtime_error("Número de colunas inválido.");
    }

    //Evita que a primeira linha seja header
    unsigned int value;
    std::vector<unsigned int> inputs;
    for(auto& s : buffer_split){
        if(isInteger(s)){
            std::istringstream(s) >> value;
            inputs.push_back(value);
        }else{
            inputs.clear();
            break;
        }
    }

    if(inputs.size() == 3 || inputs.size() == 6) {
        if (batch_only)
            Process::append(inputs[0], inputs[1], inputs[2]);
        else
            Process::append(inputs[0], inputs[1], inputs[2], inputs[3], inputs[4], inputs[5]);
    }

    while(!file.eof()) {
        std::getline(file, buffer);
        if(!buffer.size())
            break;
        buffer_split.clear();
        inputs.clear();
        buffer_split = explode(buffer, ' ');
        if ((batch_only && buffer_split.size() != 3) || (!batch_only && buffer_split.size() != 6)) {
            file.close();
            buffer_split.clear();
            throw std::runtime_error("Número de colunas inválido.");
        }
        for (auto &s : buffer_split) {
            try {
                std::istringstream(s) >> value;
                inputs.push_back(value);
            } catch (const std::exception &e) {
                inputs.clear();
                buffer_split.clear();
                file.close();
                throw std::runtime_error("Valor inválido.");
            }
        }

        if(batch_only)
            Process::append(inputs[0], inputs[1], inputs[2]);
        else
            Process::append(inputs[0], inputs[1], inputs[2], inputs[3], inputs[4], inputs[5]);

    }
    inputs.clear();
    buffer_split.clear();

}

bool Process::sort_by_id(Process a, Process b)
{
    return a.id < b.id;
}

bool Process::sort_by_arrival(Process a, Process b)
{
    if(a.time_arrival == b.time_arrival)
        return sort_by_id(a, b);
    return a.time_arrival < b.time_arrival;
}

bool Process::sort_by_remaining(Process a, Process b)
{
    if(a.time_remaining == b.time_remaining)
        return sort_by_arrival(a, b);
    return a.time_remaining < b.time_remaining;
}

bool Process::sort_by_period(Process a, Process b)
{
    if(a.period == b.period)
        return sort_by_remaining(a, b);
    return a.period < b.period;
}

void Process::fcfs(std::ostringstream& log)
{
    if(log)
        log << "Escalonamento FCFS" << std::endl;

    time_cpu = 0;
    time_turnaround_total = 0;
    for(auto& p : P){
        p.time_remaining = p.time_job;
        p.time_available = p.time_arrival;
        p.time_processing = 0;
        p.time_waiting = 0;
        p.time_turnaround = 0;
    }
    if(log)
        log << "Processos ordenados por tempo de chegada" << std::endl;

    //Ordena o vector pelo tempo de chegada
    std::sort(P.begin(), P.end(), sort_by_arrival);

    //Calcula waiting time e turnaround time. Acumula turnaround total.
    for(auto it = P.begin(); it != P.end(); ++it){
        //Caso não precise esperar, garante que o tempo atual seja o de chegada.
        if(time_cpu <= it->time_arrival){
            it->time_waiting = 0;
            time_cpu = it->time_arrival;
        }else{
            it->time_waiting = time_cpu - it->time_arrival;
            if(log)
                log << "P: " << it->id << " esperou " << it->time_waiting << "." << std::endl;

        }
        if(log)
            log << "P: " << "T: " << time_cpu << ". ";

        it->time_turnaround = it->time_job + it->time_waiting;
        time_turnaround_total += it->time_turnaround;
        if(log)
            log << "P: " << it->id << " executou " << it->time_job << "." << std::endl;

        //Tempo atual é o tempo que o último processo encerrou.
        time_cpu += it->time_job;
    }
}

void Process::sjf(std::ostringstream& log)
{
    if(log)
        log << "Escalonamento SJF" << std::endl;

    time_cpu = 0;
    time_turnaround_total = 0;
    for(auto& p : P){
        p.time_remaining = p.time_job;
        p.time_available = p.time_arrival;
        p.time_processing = 0;
        p.time_waiting = 0;
        p.time_turnaround = 0;
    }
    if(log)
        log << "Processos ordenados por tempo de chegada." << std::endl;

    //Ordena o vector pelo tempo de chegada
    std::sort(P.begin(), P.end(), sort_by_arrival);

    for(auto it  = P.begin(); it != P.end(); ++it){
        //Caso não tenha nenhum processo no tempo atual, pula para o tempo do
        //próximo processo
        if(time_cpu < it->time_arrival)
            time_cpu = it->time_arrival;

        //Verifica até qual índice o tempo de chegada já passou
        auto it2 = it;
        while(++it2 != P.end() && it2->time_arrival <= time_cpu);
        //--it2; //Remover essa linha??

        //Ordena os processos que já chegaram com base na duração
        std::sort(it, it2, sort_by_remaining);
        auto it3 = it;
        ++it3;
        if(it3 != it2 && it3 != P.end()) {
            if(log)
                log << "Mais de um processo disponível. Ordenando por duração." << std::endl;

        }


        if(time_cpu <= it->time_arrival){
            it->time_waiting = 0;
        }else{
            it->time_waiting = time_cpu - it->time_arrival;
            if(log)
                log << "P: " << it->id << " esperou " << it->time_waiting << "." << std::endl;

        }

        it->time_turnaround = it->time_job + it->time_waiting;
        time_turnaround_total += it->time_turnaround;
        if(log)
            log << "T: " << time_cpu << ". P: " << it->id << " executou " << it->time_job << std::endl;

        //Tempo atual é o tempo que o último processo encerrou.
        time_cpu += it->time_job;
    }
}

void Process::sjf_preempt(std::ostringstream& log)
{
    if(log)
        log << "Escalonamento SJF Preemptivo" << std::endl;

    time_cpu = 0;
    time_turnaround_total = 0;
    for(auto& p : P){
        p.time_remaining = p.time_job;
        p.time_available = p.time_arrival;
        p.time_processing = 0;
        p.time_waiting = 0;
        p.time_turnaround = 0;
    }
    if(log)
        log << "Ordena processos por chegada." << std::endl;

    //Ordena o vector pelo tempo de chegada
    std::sort(P.begin(), P.end(), sort_by_arrival);

    auto it = P.begin();
    while(it != P.end()){
        if(time_cpu < it->time_available)
            time_cpu = it->time_available;

        auto it2 = it;
        while(++it2 != P.end() && it2->time_available <= time_cpu);
        //Ordena os processos que já chegaram com base na duração
        std::sort(it, it2, sort_by_remaining);
        auto it3 = it;
        ++it3;
        if(it3 != it2 && it3 != P.end()){
            if(log)
                log << "Mais de um processo disponível. Ordenando por tempo restante." << std::endl;

        }

        unsigned int t_process;
        if(it2 == P.end())
            t_process = it->time_remaining;
        else {
            t_process = it2->time_available - time_cpu;
            if(t_process > it->time_remaining) t_process = it->time_remaining;
        }

        it->time_waiting += (time_cpu - it->time_available);
        it->time_remaining -= t_process;
        it->time_processing += t_process;
        it->time_available += t_process;

        if(log)
            log << "T: " << time_cpu << ". P: " << it->id << " executou " << t_process << ".";

        time_cpu += t_process;

        if(!(it->time_remaining)){
            it->time_turnaround = it->time_waiting + it->time_processing;
            time_turnaround_total += it->time_turnaround;
            ++it;
        }else{
            if(log)
                log << " Resta " << it->time_remaining << std::endl << "Preemptou";

        }
        if(log)
            log << std::endl;

    }
}

void Process::rt_rms(std::ostringstream& log, unsigned int max_limit) {

    log << "Escalonamento RT RMS" << std::endl;

    missed_deadline = 0;
    time_cpu = 0;
    time_turnaround_total = 0;
    for(auto& p : P){
        p.time_remaining = p.time_job;
        p.time_available = 0;
        p.time_processing = 0;
        p.time_waiting = 0;
        p.time_turnaround = 0;
        p.missed = 0;
        not_schedulable=0;
        if(p.time_deadline < p.time_job) {
            p.schedulable = false;
            not_schedulable++;
            p.period = (unsigned int)-1;
        } else {
            p.schedulable = true;
            p.period = p.qt_job*p.time_deadline;
        }
    }

    log << "Ordena processos por período." << std::endl;
    //Sort por menor periodo
    std::sort(P.begin(), P.end(), sort_by_period);

    while(true){
        unsigned int process_number = 0;
        double cpu_utilization = 0;
        for(auto it = P.begin(); it != P.end(); ++it) {
            if(!it->schedulable)
                break;
            //Encontra o número de processos válidos e acumula a utilização de cpu
            process_number++;
            cpu_utilization += (double)it->time_job/it->period;
        }
        //Caso 0 processos sejam possíveis, finaliza
        if(!process_number)
            throw std::runtime_error("Lista não é escalonável utilizando RMS.");
        //Verificar se é escalonável: fórmula
        if(cpu_utilization > (double)process_number*(std::pow(2, 1/(double)process_number))) {
            //Se não for, elimina o último e verifica novamente
            for(auto it = P.rbegin(); it != P.rend(); ++it) {
                if(it->schedulable) {
                    if(log)
                        log << "Processo " << it->id << " não é escalonável utilizando RMS" << std::endl;
                    it->schedulable = false;
                    not_schedulable++;
                    //Marcar period como -1 unsigned caso seja missed: facilita na execução
                    it->period = (unsigned int)-1;
                    break;
                }
            }
            continue;
        }
        log << "Lista escalonável com " << cpu_utilization*100 << "% de uso de CPU" << std::endl;
        break;
    }

    //Define o tempo limite baseado no MMC de todos periodos. No exemplo da lista é 90.090
    //A partir do tempo definido pelo MMC, os processos periódicos irão repetir as iterações que fizeram até esse tempo
    unsigned long mmc = findlcm();

    //Para evitar processamento demorado demais em listas muito grandes, é definido um limite
    while(time_cpu < mmc && time_cpu < max_limit) {
        //Seleciona o processo que está disponível há mais tempo
        auto it = P.begin();
        for(auto it2 = P.begin(); (it2 != P.end() && it2->schedulable); ++it2) {
            if(it2->time_available < it->time_available)
                it = it2;
        }
        //Seleciona o primeiro que encontrar se já passou o tempo do CPU (maior prioridade)
        for(auto it2 = P.begin(); it2 != it; ++it2) {
            if(time_cpu >= it2->time_available) {
                it = it2;
                break;
            }
        }

        //Voltando de preempção
        if(it->time_remaining != it->time_job) {
            //Verifica se é válida a tentativa de continuar a execução ou se o deadline já passou
            if(time_cpu > it->time_available+it->period ||
               time_cpu+ it->time_remaining > it->time_deadline+it->time_available ||
               time_cpu+ it->time_remaining > it->time_available+it->period) {
                log << "P " << it->id << " perdido apos voltar da preempção" << std::endl;
                it->missed++;
                missed_deadline++;
                it->time_available += it->period;
                it->time_remaining = it->time_job;
                //Se já passou, descarta e passa para o próximo job
                continue;
            }
        }

        //Define um limite de tempo para executar até preemptar baseado na prioridade dos jobs
        unsigned int time_limit = it->time_available+it->period;
        for(auto it2 = P.begin(); (it2 != P.end() && it2->schedulable); ++it2) {
            if(it2 == it)
                continue;
            if(it2->time_available < time_limit && it2->time_available > it->time_available && it2->period < it->period)
                time_limit = it2->time_available;
        }

        //Pula tempo em idle. Em situações reais, esse tempo em idle é útil para tarefas não-rt
        if(it->time_available > time_cpu)
            time_cpu = it->time_available;

        //Define o tempo que o job será computado com base no tempo de limite
        unsigned int t_process;
        if(time_limit >= it->time_remaining+time_cpu) {
            t_process = it->time_remaining;
        } else {
            t_process = time_limit - time_cpu;
        }

        //Detecta se a deadline é impossível de ser cumprida e descarta o processo antes de executar por qualquer tempo
        if(t_process >= it->time_remaining &&
           time_cpu+it->time_remaining > it->time_available + it->time_deadline) {
            it->missed++;
            missed_deadline++;
            it->time_available += it->period;
            it->time_remaining = it->time_job;
            log << "P " << it->id << " perdido" << std::endl;
            continue;
        }

        //Executa
        it->time_remaining -= t_process;
        log << "T: " << time_cpu << ". P " << it->id << " executou por " << t_process;
        time_cpu += t_process;

        //Se finalizou marca o tempo de chegada do próximo job igual
        if(!it->time_remaining) {
            it->time_available += it->period;
            it->time_remaining = it->time_job;
            log << " FINALIZADO";
        }
        log << std::endl;
    }

}

unsigned int Process::get_time_deadline(unsigned int index) {
    return P[index].time_deadline;
}

unsigned int Process::get_missed(unsigned int index) {
    return P[index].missed;
}

bool Process::is_schedulable(unsigned int index) {
    return P[index].schedulable;
}

unsigned int Process::get_missed_deadline() {
    return missed_deadline;
}

unsigned int Process::get_period(unsigned int index) {
    return P[index].period;
}

unsigned long Process::findlcm() {
    unsigned long lcm = P.begin()->period;
    for(auto p : P) {
        if(!p.schedulable)
            break;
        lcm =  (p.period*lcm) / gcd(p.period, lcm);
    }
    return lcm;
}

unsigned long Process::gcd(unsigned long period, unsigned long lcm) {
    if(!lcm)
        return period;
    return gcd(lcm, period%lcm);
}
