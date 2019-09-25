#include "shell.hpp"

#define REGISTER_APPLICATION(x)                                                                                        \
  TaskHandle_t x##_##task_handle;                                                                                      \
  p_task_name_vect.push_back(std::string(#x));                                                                         \
  p_task_vect.push_back(x);                                                                                            \
  task_handles.push_back(&x##_##task_handle);

typedef void (*p_task)(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *);

static std::vector<p_task> p_task_vect;
static std::vector<std::string> p_task_name_vect;
static std::vector<std::string> startup_applications = {"clear", "testing"};
static std::vector<TaskHandle_t *> task_handles;

std::vector<std::string> *get_startup_applications() { return &startup_applications; }

TaskHandle_t *get_task_handle(std::string task_name) {
  int index;

  for (unsigned int i = 0; i < p_task_name_vect.size(); i++) {

    if ((task_name == p_task_name_vect[i]) && (task_name.length() > 0)) {

      index = i;
      break;

    } else
      index = -1;
  }

  return (index != -1) ? task_handles[index] : nullptr;
}

std::vector<std::string> *get_app_names() { return &p_task_name_vect; }

bool exec(std::string task_name, std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  int index;

  for (unsigned int i = 0; i < p_task_name_vect.size(); i++) {

    if ((task_name == p_task_name_vect[i]) && (task_name.length() > 0)) {

      index = i;
      break;

    } else
      index = -1;
  }

  if (index != -1) {

    p_task_vect[index](p_args);
    return true;

  } else {

    if (task_name.length() > 0)
      shell::hardware[STD_IO_DRIVER].write("\r\nUnknown command \"%s\".\r\n", task_name.c_str());

    return false;
  }
}

/* Declare and register applications */
extern void testing(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);
extern void clear(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);
extern void cowsay(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);
extern void tasks(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);
extern void calibrate_adcs(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);
extern void test(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args);

void load_applications() {
  REGISTER_APPLICATION(tasks);
  REGISTER_APPLICATION(testing);
  REGISTER_APPLICATION(clear);
  REGISTER_APPLICATION(cowsay);
  REGISTER_APPLICATION(calibrate_adcs);
  REGISTER_APPLICATION(test);
}
