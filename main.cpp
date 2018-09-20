#include <string>
#include <gtkmm.h>
#include <iostream>
#include <stack>
#include <iomanip>
#include "process.h"

const Glib::ustring app_id = "teste.escalonamento";

Gtk::Window* window_main = nullptr;
Gtk::Label* label_file = nullptr;
Gtk::Button* button_sched = nullptr;
Gtk::ComboBox* combo_alg = nullptr;
Gtk::RadioButton* radio_type = nullptr;
Gtk::Window* window_table = nullptr;
std::ostringstream oss_log;
Glib::RefPtr<Gtk::ListStore> list_rt;
Glib::RefPtr<Gtk::ListStore> list_lote;


static void on_button_sched_clicked();
static void on_button_open_clicked();
static void on_radio_lote_toggled();
static void on_button_log_clicked();
const std::string get_last_filename(const std::string& s);

int main(int argc, char *argv[]) {

    auto app = Gtk::Application::create(argc, argv, app_id);

    auto builder = Gtk::Builder::create();
    try
    {
        builder->add_from_file("main.glade");
    }
    catch(const std::exception& ex)
    {

        return 1;
    }

    builder->get_widget("window_main", window_main);
    Gtk::Button* button_open = nullptr;
    builder->get_widget("button_open", button_open);
    builder->get_widget("button_sched", button_sched);
    builder->get_widget("label_file", label_file);
    builder->get_widget("combo_alg", combo_alg);
    builder->get_widget("radio_lote", radio_type);
    list_rt = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(builder->get_object("list_rt"));
    list_lote = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(builder->get_object("list_lote"));
    if(!button_open || !button_sched || !label_file || !combo_alg || !window_main || !radio_type){
        std::cerr << "Impossível apontar widgets" << std::endl;
        return 2;
    }

    button_open->signal_clicked().connect(sigc::ptr_fun(on_button_open_clicked));
    radio_type->signal_toggled().connect(sigc::ptr_fun(on_radio_lote_toggled));
    button_sched->signal_clicked().connect(sigc::ptr_fun(on_button_sched_clicked));

    window_main->set_position(Gtk::WIN_POS_CENTER);

    return app->run(*window_main);
}

void on_button_open_clicked(){
    // Create the dialog box FileChooser
    Gtk::FileChooserDialog dialog("Selecione arquivo de processos", Gtk::FILE_CHOOSER_ACTION_OPEN);
    dialog.set_transient_for(*window_main);

    //Add response buttons the the dialog:
    dialog.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("_Open", Gtk::RESPONSE_OK);

    //Add filters, so that only certain file types can be selected:
    auto filter_text = Gtk::FileFilter::create();
    filter_text->set_name("Text files");
    filter_text->add_mime_type("text/plain");
    dialog.add_filter(filter_text);

    auto filter_any = Gtk::FileFilter::create();
    filter_any->set_name("Any files");
    filter_any->add_pattern("*");
    dialog.add_filter(filter_any);

    //Show the dialog and wait for a user response:
    int result = dialog.run();

    //Handle the response:
    switch(result)
    {
        case(Gtk::RESPONSE_OK):
            try{
                Glib::ustring filename = dialog.get_filename();
                Process::load_from_file(filename);
                Glib::ustring labelname = get_last_filename(filename);
                label_file->set_text(labelname);
                button_sched->set_sensitive(true);
                break;
            }catch (const std::exception& e){
                Gtk::MessageDialog dialog(*window_main, e.what(), false, Gtk::MESSAGE_ERROR);
                dialog.run();
            }
        case(Gtk::RESPONSE_CANCEL):
            label_file->set_text("");
            button_sched->set_sensitive(false);
            break;
        default:
            break;
    }

}

void on_radio_lote_toggled() {
    bool selected = radio_type->get_active();
    Glib::RefPtr<Gtk::TreeModel> model = combo_alg->get_model();
    if(selected){
        combo_alg->set_model((Glib::RefPtr<Gtk::TreeModel>&)list_lote);
        combo_alg->set_active(0);
    }else{
        combo_alg->set_model((Glib::RefPtr<Gtk::TreeModel>&)list_rt);
        combo_alg->set_active(0);
    }
}

const std::string get_last_filename(const std::string& s)
{
#ifdef _WIN32
    char c = '\\';
#else
    char c = '/';
#endif
    std::string buff{""};
    std::stack<std::string> v;

    for(auto n:s)
    {
        if(n != c) buff+=n; else
        if(n == c && buff != "") { v.push(buff); buff = ""; }
    }
    if(buff != "") v.push(buff);

    return v.top();
}

void print_example()
{
    std::cout << "Process";
    std::cout << std::setw(4) << "";
    std::cout << "Arrival";
    std::cout << std::setw(4) << "";
    std::cout << "Duration";
    std::cout << std::setw(4) << "";
    std::cout << "Waiting Time";
    std::cout << std::setw(4) << "";
    std::cout << "Turnaround Time";
    std::cout << std::endl;

    for(int i = 0; i < Process::get_list_size(); i++){
        std::cout << std::setw(7) << Process::get_id(i);
        std::cout << std::setw(4) << "";
        std::cout << std::setw(7) << Process::get_time_arrival(i);
        std::cout << std::setw(4) << "";
        std::cout << std::setw(8) << Process::get_time_job(i);
        std::cout << std::setw(4) << "";
        std::cout << std::setw(12) << Process::get_time_waiting(i);
        std::cout << std::setw(4) << "";
        std::cout << std::setw(15) << Process::get_time_turnaround(i);
        std::cout << std::endl;
    }

    std::cout << "Average Turnaround Time: ";
    std::cout << std::setprecision(4) << Process::get_time_turnaround_average() << std::endl;

}

class ModelColumnBatch : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumnBatch()
    { add(col_id); add(col_arrival); add(col_processing); add(col_waiting); add(col_turnaround);}

    Gtk::TreeModelColumn<unsigned int> col_id;
    Gtk::TreeModelColumn<unsigned int> col_arrival;
    Gtk::TreeModelColumn<unsigned int> col_processing;
    Gtk::TreeModelColumn<unsigned int> col_waiting;
    Gtk::TreeModelColumn<unsigned int> col_turnaround;
};

class ModelColumnRt : public Gtk::TreeModel::ColumnRecord
{
public:

    ModelColumnRt()
    { add(col_id); add(col_period); add(col_processing); add(col_deadline); add(col_missed);}

    Gtk::TreeModelColumn<unsigned int> col_id;
    Gtk::TreeModelColumn<unsigned int> col_period;
    Gtk::TreeModelColumn<unsigned int> col_processing;
    Gtk::TreeModelColumn<unsigned int> col_deadline;
    Gtk::TreeModelColumn<unsigned int> col_missed;
};

void on_button_sched_clicked() {
    auto builder_table = Gtk::Builder::create_from_file("table.glade");
    Gtk::Button* button_log = nullptr;
    Gtk::Label* label_att = nullptr;
    Gtk::Label* label_alg = nullptr;
    Gtk::Label* label_type = nullptr;

    Gtk::ScrolledWindow* scrollwindow_table = nullptr;
    Gtk::TreeView* table_sched = nullptr;
    builder_table->get_widget("window_table", window_table);
    builder_table->get_widget("button_log", button_log);
    builder_table->get_widget("label_att", label_att);
    builder_table->get_widget("scrollwindow_table", scrollwindow_table);
    builder_table->get_widget("table_sched", table_sched);
    builder_table->get_widget("label_alg", label_alg);
    builder_table->get_widget("label_type", label_type);
    window_table->set_transient_for(*window_main);
    window_table->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

    button_log->signal_clicked().connect(sigc::ptr_fun(on_button_log_clicked));
    oss_log.str("");
    bool lotes = radio_type->get_active();
    if(lotes){
        int alg = combo_alg->get_active_row_number();
        switch(alg) {
            case 0:
                label_alg->set_text("FCFS (FIFO)");
                Process::fcfs(oss_log);
                break;
            case 1:
                label_alg->set_text("SJF (Shortest Job First)");
                Process::sjf(oss_log);
                break;
            case 2:
                label_alg->set_text("SJF Preemptivo (Shortest Job First)");
                Process::sjf_preempt(oss_log);
                break;
            default:
                break;
        }
        ModelColumnBatch model_column;
        Glib::RefPtr<Gtk::ListStore> model_batch = Gtk::ListStore::create(model_column);
        table_sched->set_model(model_batch);
        for(unsigned int i = 0; i < Process::get_list_size(); i++){
            Gtk::TreeModel::Row row = *(model_batch->append());
            row[model_column.col_id] = Process::get_id(i);
            row[model_column.col_arrival] = Process::get_time_arrival(i);
            row[model_column.col_processing] = Process::get_time_job(i);
            row[model_column.col_waiting] = Process::get_time_waiting(i);
            row[model_column.col_turnaround] = Process::get_time_turnaround(i);
        }
        table_sched->append_column("ID", model_column.col_id);
        table_sched->append_column("Chegada", model_column.col_arrival);
        table_sched->append_column("Processamento", model_column.col_processing);
        table_sched->append_column("Espera", model_column.col_waiting);
        table_sched->append_column("Turnaround", model_column.col_turnaround);
        std::ostringstream s;
        s << std::fixed << std::setprecision(3) << Process::get_time_turnaround_average();
        label_type->set_text("Average Turnaround Time:");
        label_att->set_text(s.str());
        window_table->show_all();
    }else{
        if(Process::is_batch_only()){
            Gtk::MessageDialog dialog(*window_main, "Lista carregada suporta somente escalonamento em lotes.",
                                      false, Gtk::MESSAGE_ERROR);
            dialog.run();
        }else{
            try {
                Process::rt_rms(oss_log, 1000000);
                label_alg->set_text("RMS (Rate-Monotonic Scheduling)");
                ModelColumnRt model_column;
                Glib::RefPtr<Gtk::ListStore> model_rt = Gtk::ListStore::create(model_column);
                table_sched->set_model(model_rt);
                for (unsigned int i = 0; i < Process::get_list_size(); i++) {
                    if (!Process::is_schedulable(i))
                        break;
                    Gtk::TreeModel::Row row = *(model_rt->append());
                    row[model_column.col_id] = Process::get_id(i);
                    row[model_column.col_period] = Process::get_period(i);
                    row[model_column.col_processing] = Process::get_time_job(i);
                    row[model_column.col_deadline] = Process::get_time_deadline(i);
                    row[model_column.col_missed] = Process::get_missed(i);
                }
                table_sched->append_column("ID", model_column.col_id);
                table_sched->append_column("Período", model_column.col_period);
                table_sched->append_column("Processamento", model_column.col_processing);
                table_sched->append_column("Deadline", model_column.col_deadline);
                table_sched->append_column("Perdidos", model_column.col_missed);
                std::ostringstream s;
                s << Process::get_missed_deadline();
                label_type->set_text("Perdidos:");
                label_att->set_text(s.str());
                window_table->show();
            } catch(const std::exception& ex) {
                Gtk::MessageDialog dialog(*window_main, ex.what(),
                                          false, Gtk::MESSAGE_ERROR);
                dialog.run();
            }
        }
    }
}

static void on_button_log_clicked() {
    auto builder_log = Gtk::Builder::create_from_file("log.glade");
    Gtk::Window* window_log = nullptr;
    Gtk::TextView* textview_log = nullptr;
    builder_log->get_widget("window_log", window_log);
    builder_log->get_widget("textview_log", textview_log);

    window_log->set_transient_for(*window_table);
    window_log->set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

    Glib::RefPtr<Gtk::TextBuffer> buffer_log;
    buffer_log = Gtk::TextBuffer::create();

    buffer_log->set_text(oss_log.str());
    textview_log->set_buffer(buffer_log);
    window_log->show_all();
}