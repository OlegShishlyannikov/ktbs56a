#include "events_id.hpp"
#include "shell.hpp"
#include "signal.hpp"

#define TEMP_STATUS_OK true
#define TEMP_STATUS_FAULT false
#define FSM_SETUP_STATE false
#define FSM_MEASUREMENT_STATE true

#define FSM_SETUP_VOLTAGE_STATE 0
#define FSM_SETUP_CURRENT_STATE 1
#define FSM_SETUP_MISTAKE_STATE 2

#define DEVICE_NUMBER 5
#define DUTS_NUMBER 35
#define ADCS_NUMBER 18

#define DUT_CURRENT_STEP 5.0f

static double response = 0;
static double adc_spi_data = 0.0f;
static double adc_spi_temp = 0.0f;
static int dut_number = 0;
static const std::vector<double> output_voltages = {0.0f, 5.0f, 8.0f, 12.0f, 15.0f};
static std::map<std::string, double> static_errors_k;
static std::map<std::string, double> device_temperature;
static const double maximum_device_temperature = 70.0f;
static bool temp_status = TEMP_STATUS_OK;

static double normal_voltage = 5.0f;
static double dut_current = 0.0f;
static double measurement_mistake = 2.0f;

static bool fsm_state = FSM_SETUP_STATE;
static int fsm_setup_state = FSM_SETUP_VOLTAGE_STATE;

static double calibration_table[DUTS_NUMBER * DEVICE_NUMBER];

void testing(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  static TimerHandle_t blink_timer;
  char rcvd_char = '\0';
  signal<int, double> cell_overheat;

  static_cast<signal<void *> *>(shell::callback_signals[USART_CHAR_RECEIVED_EVENT_ID])
      ->connect([&rcvd_char](void *args = nullptr) -> void {
        rcvd_char = *static_cast<char *>(shell::hardware[STD_IO_DRIVER].read(portIO_MAX_DELAY));
      });

  static_cast<signal<void *> *>(shell::callback_signals[TIMER_0_EVENT_ID])->connect([](void *args = nullptr) -> void {
    if (temp_status == TEMP_STATUS_FAULT) {

      for (unsigned int i = 0; i < 2; i++) {

        shell::hardware["fpga_driver"].write("%u_red_led_on", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_on",
                                             ((dut_number % 2) == 1) ? dut_number - 1 : dut_number + 1);
        vTaskDelay(25);
        shell::hardware["fpga_driver"].write("%u_red_led_off", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_off",
                                             ((dut_number % 2) == 1) ? dut_number - 1 : dut_number + 1);
        vTaskDelay(25);
        shell::hardware["fpga_driver"].write("%u_red_led_on", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_on",
                                             ((dut_number % 2) == 1) ? dut_number - 1 : dut_number + 1);
        vTaskDelay(25);
        shell::hardware["fpga_driver"].write("%u_red_led_off", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_off",
                                             ((dut_number % 2) == 1) ? dut_number - 1 : dut_number + 1);
        vTaskDelay(25);
      }

    } else if ((temp_status == TEMP_STATUS_OK) && (fsm_state == FSM_SETUP_STATE)) {

      if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

        shell::hardware["fpga_driver"].write("%u_red_led_on", 34);
        shell::hardware["fpga_driver"].write("%u_red_led_on", 33);
        vTaskDelay(50);
        shell::hardware["fpga_driver"].write("%u_red_led_off", 34);
        shell::hardware["fpga_driver"].write("%u_red_led_off", 33);

      } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

        shell::hardware["fpga_driver"].write("%u_green_led_on", 34);
        shell::hardware["fpga_driver"].write("%u_green_led_on", 33);
        vTaskDelay(50);
        shell::hardware["fpga_driver"].write("%u_green_led_off", 34);
        shell::hardware["fpga_driver"].write("%u_green_led_off", 33);

      } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

        shell::hardware["fpga_driver"].write("%u_red_led_on", 34);
        shell::hardware["fpga_driver"].write("%u_red_led_on", 33);
        shell::hardware["fpga_driver"].write("%u_green_led_on", 34);
        shell::hardware["fpga_driver"].write("%u_green_led_on", 33);
        vTaskDelay(50);
        shell::hardware["fpga_driver"].write("%u_red_led_off", 34);
        shell::hardware["fpga_driver"].write("%u_red_led_off", 33);
        shell::hardware["fpga_driver"].write("%u_green_led_off", 34);
        shell::hardware["fpga_driver"].write("%u_green_led_off", 33);
      }
    }
  });

  cell_overheat.connect([](int cell_number, double temp) -> void {
    shell::hardware["gp_timers_driver"].write("tim_0_start");
    shell::hardware["gp_timers_driver"].read(portIO_MAX_DELAY);
    shell::hardware["dut_loads_driver"].write("%lf_ma_set_current", 0.0f);
    shell::hardware["fpga_driver"].write("reset_all");
    shell::hardware["leds_driver"].write("led_%u_reset", 1);
    shell::hardware["leds_driver"].write("led_%u_set", 2);
    shell::hardware[STD_IO_DRIVER].write(
        ASCII_CONTROL_SET_ATTRS + "[!] Overheat on cell #%u (%lf C%s). Task stopped.\r\n" + ASCII_CONTROL_SET_ATTRS,
        ASCII_TERM_ATTRS_BRIGHT, ASCII_TERM_ATTRS_BLACK_BG, ASCII_TERM_ATTRS_RED_FG, dut_number / 2, adc_spi_temp,
        ASCII_EXT_CH_MASCILINE_IND.c_str(), ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS, ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS,
        ASCII_TERM_ATTRS_CLEAR_ALL_ATTRS);
  });

  if (std::get<0>(*p_args)->size() > 0)
    shell::hardware[STD_IO_DRIVER].write("\r\n");

  for (unsigned int i = 0; i < std::get<0>(*p_args)->size(); i++)
    shell::hardware[STD_IO_DRIVER].write("arg#%u : %s\r\n", i, std::get<0>(*p_args)->at(i).c_str());

  shell::hardware["fpga_driver"].write("reset_all");

  for (unsigned int i = 1; i < 5; i++) {

    shell::hardware["leds_driver"].write("led_%u_set", i);
    vTaskDelay(25);
    shell::hardware["leds_driver"].write("led_%u_reset", i);
    vTaskDelay(25);
  }

  shell::hardware["fpga_driver"].write("get_conf_status");

  if (*static_cast<int *>(shell::hardware["fpga_driver"].read(portIO_MAX_DELAY))) {

    shell::hardware["leds_driver"].write("led_%u_read_state", 3);
    (*static_cast<int *>(shell::hardware["leds_driver"].read(portIO_MAX_DELAY)) == 0)
        ? shell::hardware["leds_driver"].write("led_%u_set", 3)
        : (void)0;

    shell::hardware["leds_driver"].write("led_%u_read_state", 4);
    (*static_cast<int *>(shell::hardware["leds_driver"].read(portIO_MAX_DELAY)) == 1)
        ? shell::hardware["leds_driver"].write("led_%u_reset", 4)
        : (void)0;

  } else {

    shell::hardware["leds_driver"].write("led_%u_read_state", 4);
    (*static_cast<int *>(shell::hardware["leds_driver"].read(portIO_MAX_DELAY)) == 0)
        ? shell::hardware["leds_driver"].write("led_%u_set", 4)
        : (void)0;

    shell::hardware["leds_driver"].write("led_%u_read_state", 3);
    (*static_cast<int *>(shell::hardware["leds_driver"].read(portIO_MAX_DELAY)) == 1)
        ? shell::hardware["leds_driver"].write("led_%u_reset", 3)
        : (void)0;
  }

  shell::hardware["adc_spi_driver"].write("read_calibration_table_size_%u", DUTS_NUMBER * output_voltages.size());
  double *p_readed_table = reinterpret_cast<double *>(
      static_cast<size_t>((*static_cast<double *>(shell::hardware["adc_spi_driver"].read(portIO_MAX_DELAY)))));

  for (unsigned int i = 0; i < DUTS_NUMBER * output_voltages.size(); i++) {

    if (static_errors_k.find("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                             std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF") == static_errors_k.end())
      static_errors_k.insert(std::make_pair("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                                                std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF",
                                            p_readed_table[i]));

    else
      static_errors_k.at("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                         std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF") = p_readed_table[i];
  }

  shell::hardware["dut_loads_driver"].write("calibrate");
  shell::hardware["dut_loads_driver"].write("get_adc_cal_val");
  int adc_cal_val = *static_cast<int *>(shell::hardware["dut_loads_driver"].read(portIO_MAX_DELAY));
  shell::hardware["dut_loads_driver"].write("get_dac_cal_val");
  int dac_cal_val = *static_cast<int *>(shell::hardware["dut_loads_driver"].read(portIO_MAX_DELAY));
  shell::hardware["dut_loads_driver"].write("get_diff_cal_val");
  int diff_cal_val = *static_cast<int *>(shell::hardware["dut_loads_driver"].read(portIO_MAX_DELAY));

  std::function<double(double)> get_range = [](double data) -> double {
    double min = 0.0f;

    for (double output_voltage : output_voltages) {

      if (data >= output_voltage)
        min = output_voltage;
    }

    return min;
  };

  static_cast<signal<void *> *>(shell::callback_signals[BUTTON_MINUS_PRESS_EVENT_ID])
      ->connect([&rcvd_char](void *args = nullptr) -> void {
        if (fsm_state == FSM_SETUP_STATE) {

          if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

            if (normal_voltage >= 1.0f) {

              normal_voltage -= 1.0f;
              shell::hardware[STD_IO_DRIVER].write("Voltage norm is : %lf\r\n", normal_voltage);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
              }
            }
          } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

            if (dut_current >= DUT_CURRENT_STEP) {

              dut_current -= DUT_CURRENT_STEP;
              shell::hardware[STD_IO_DRIVER].write("DUTs current is : %lf\r\n", dut_current);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }
          } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

            if (measurement_mistake >= 2.0f) {

              measurement_mistake -= 1.0f;
              shell::hardware[STD_IO_DRIVER].write("Measurement mistake is : %lf\r\n", measurement_mistake);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }
          }
        }
      });

  static_cast<signal<void *> *>(shell::callback_signals[BUTTON_PLUS_PRESS_EVENT_ID])
      ->connect([&rcvd_char](void *args = nullptr) -> void {
        if (fsm_state == FSM_SETUP_STATE) {

          if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

            if (normal_voltage < 24.0f) {

              normal_voltage += 1.0f;
              shell::hardware[STD_IO_DRIVER].write("Voltage norm is : %lf\r\n", normal_voltage);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
              }
            }
          } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

            if (dut_current < DUT_CURRENT_STEP * 30.0f) {

              dut_current += DUT_CURRENT_STEP;
              shell::hardware[STD_IO_DRIVER].write("DUTs current is : %lf\r\n", dut_current);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }
          } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

            if (measurement_mistake < 10.0f) {

              measurement_mistake += 1.0f;
              shell::hardware[STD_IO_DRIVER].write("Measurement mistake is : %lf\r\n", measurement_mistake);
              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }
          }
        }
      });

  static_cast<signal<void *> *>(shell::callback_signals[BUTTON_RUN_PRESS_EVENT_ID])
      ->connect([&rcvd_char](void *args = nullptr) -> void {
        bool keep_button_flag = false;

        for (unsigned int i = 0; i < 20; i++) {

          shell::hardware["buttons_driver"].write("get_run_button_state");

          if (!(*static_cast<int *>(shell::hardware["buttons_driver"].read(portMAX_DELAY)))) {

            keep_button_flag = true;
            vTaskDelay(100);

          } else {

            keep_button_flag = false;
            break;
          }
        }

        if (fsm_state == FSM_SETUP_STATE) {

          if (keep_button_flag == false) {

            if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }

              fsm_setup_state = FSM_SETUP_CURRENT_STATE;

            } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }

              fsm_setup_state = FSM_SETUP_MISTAKE_STATE;

            } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
              }

              fsm_setup_state = FSM_SETUP_VOLTAGE_STATE;
            }
          } else if (keep_button_flag == true) {

            shell::hardware["fpga_driver"].write("reset_all");
            shell::hardware["leds_driver"].write("led_%u_reset", 1);
            shell::hardware["leds_driver"].write("led_%u_reset", 2);

            for (unsigned int i = 0; i < 4; i++) {

              shell::hardware["fpga_driver"].write("%u_red_led_on", 34);
              shell::hardware["fpga_driver"].write("%u_red_led_on", 33);
              shell::hardware["fpga_driver"].write("%u_green_led_on", 34);
              shell::hardware["fpga_driver"].write("%u_green_led_on", 33);
              shell::hardware["fpga_driver"].write("%u_red_led_off", 34);
              shell::hardware["fpga_driver"].write("%u_red_led_off", 33);
              shell::hardware["fpga_driver"].write("%u_green_led_off", 34);
              shell::hardware["fpga_driver"].write("%u_green_led_off", 33);
            }

            fsm_state = FSM_MEASUREMENT_STATE;
            return;
          }
        } else if (fsm_state == FSM_MEASUREMENT_STATE) {

          if (keep_button_flag == true) {

            shell::hardware["fpga_driver"].write("reset_all");
            shell::hardware["leds_driver"].write("led_%u_reset", 1);
            shell::hardware["leds_driver"].write("led_%u_reset", 2);

            for (unsigned int i = 0; i < 4; i++) {

              shell::hardware["fpga_driver"].write("%u_red_led_on", 34);
              shell::hardware["fpga_driver"].write("%u_red_led_on", 33);
              shell::hardware["fpga_driver"].write("%u_green_led_on", 34);
              shell::hardware["fpga_driver"].write("%u_green_led_on", 33);
              shell::hardware["fpga_driver"].write("%u_red_led_off", 34);
              shell::hardware["fpga_driver"].write("%u_red_led_off", 33);
              shell::hardware["fpga_driver"].write("%u_green_led_off", 34);
              shell::hardware["fpga_driver"].write("%u_green_led_off", 33);
            }

            if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
              }
            } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }

            fsm_state = FSM_SETUP_STATE;
            return;
          } else if ((keep_button_flag == false) && (temp_status == TEMP_STATUS_FAULT)) {

            if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
              }
            } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

              shell::hardware["fpga_driver"].write("reset_all");

              for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

                shell::hardware["fpga_driver"].write("%u_red_led_on", i);
                shell::hardware["fpga_driver"].write("%u_green_led_on", i);
              }
            }

            temp_status = TEMP_STATUS_OK;
            fsm_state = FSM_SETUP_STATE;
          }
        }
      });

  if (fsm_state == FSM_SETUP_STATE) {

    shell::hardware["gp_timers_driver"].write("tim_0_start");
    shell::hardware["gp_timers_driver"].read(portIO_MAX_DELAY);

    if (fsm_setup_state == FSM_SETUP_VOLTAGE_STATE) {

      shell::hardware["fpga_driver"].write("reset_all");

      for (int i = 0; i < static_cast<int>(normal_voltage); i++) {

        shell::hardware["fpga_driver"].write("%u_red_led_on", i);
      }

    } else if (fsm_setup_state == FSM_SETUP_CURRENT_STATE) {

      shell::hardware["fpga_driver"].write("reset_all");

      for (int i = 0; i < static_cast<int>(dut_current / DUT_CURRENT_STEP); i++) {

        shell::hardware["fpga_driver"].write("%u_green_led_on", i);
      }

    } else if (fsm_setup_state == FSM_SETUP_MISTAKE_STATE) {

      shell::hardware["fpga_driver"].write("reset_all");

      for (int i = 0; i < static_cast<int>(measurement_mistake); i++) {

        shell::hardware["fpga_driver"].write("%u_red_led_on", i);
        shell::hardware["fpga_driver"].write("%u_green_led_on", i);
      }
    }
  }

  while (true) {

    if (fsm_state == FSM_MEASUREMENT_STATE) {

      shell::hardware["gp_timers_driver"].write("tim_0_stop");
      shell::hardware["dut_loads_driver"].write("get_adc_val");
      int dut_loads_adc_val = *static_cast<int *>(shell::hardware["dut_loads_driver"].read(portIO_MAX_DELAY));
      shell::hardware["dut_loads_driver"].write("get_dac_val");
      int dut_loads_dac_val = *static_cast<int *>(shell::hardware["dut_loads_driver"].read(portIO_MAX_DELAY));
      shell::hardware["dut_loads_driver"].write("%lf_ma_set_current", dut_current);

      shell::hardware["fpga_driver"].write("%u_red_led_off", dut_number);
      shell::hardware["fpga_driver"].write("%u_green_led_off", dut_number);
      shell::hardware["fpga_driver"].write("%u_adc_select", dut_number / 2);

      shell::hardware["adc_spi_driver"].write("ch_%u_%u_get_data", dut_number, 1);
      adc_spi_data = *static_cast<double *>(shell::hardware["adc_spi_driver"].read(portIO_MAX_DELAY * 50));
      double vref_range = get_range(adc_spi_data);

      if (static_errors_k.find("ch_" + std::to_string(dut_number) + "_" + std::to_string(vref_range) + "_VREF") !=
          static_errors_k.end())
        adc_spi_data +=
            static_errors_k.at("ch_" + std::to_string(dut_number) + "_" + std::to_string(vref_range) + "_VREF");

      shell::hardware["adc_spi_driver"].write("ch_%u_%u_get_temp", dut_number, 1);
      adc_spi_temp = *static_cast<double *>(shell::hardware["adc_spi_driver"].read(portIO_MAX_DELAY * 50));
      shell::hardware["fpga_driver"].write("%u_adc_deselect", dut_number / 2);

      if ((adc_spi_data > (normal_voltage + normal_voltage * (measurement_mistake) / 100.0f)) ||
          (adc_spi_data < (normal_voltage - normal_voltage * (measurement_mistake) / 100.0f))) {

        shell::hardware["fpga_driver"].write("%u_green_led_off", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_on", dut_number);

      } else {

        shell::hardware["fpga_driver"].write("%u_green_led_on", dut_number);
        shell::hardware["fpga_driver"].write("%u_red_led_off", dut_number);
      }

      shell::hardware[STD_IO_DRIVER].write(
          "CS_ADC : %i\tCS_DAC : %i, Setting current : %lf, Measured : %lf, Cell_temperature : %lf, DUTn : %u\r\n",
          dut_loads_adc_val, dut_loads_dac_val, dut_current, adc_spi_data, adc_spi_temp, dut_number + 1);

      if (adc_spi_temp >= maximum_device_temperature)
        temp_status = TEMP_STATUS_FAULT;
      else
        temp_status = TEMP_STATUS_OK;

      if (temp_status == TEMP_STATUS_FAULT) {

        shell::hardware["dut_loads_driver"].write("%lf_ma_set_current", 0.0f);
        shell::hardware["fpga_driver"].write("reset_all");
        cell_overheat.emit(dut_number, adc_spi_temp);

        while (temp_status == TEMP_STATUS_FAULT)
          vTaskDelay(100);

        continue;

      } else {

        shell::hardware["leds_driver"].write("led_%u_set", 1);
        shell::hardware["leds_driver"].write("led_%u_reset", 2);
      }

      if (rcvd_char == ASCII_CTRL_CH_CR.at(0)) {

        temp_status = TEMP_STATUS_FAULT;
        shell::hardware["dut_loads_driver"].write("%lf_ma_set_current", 0.0f);
        shell::hardware["fpga_driver"].write("reset_all");
        rcvd_char = '\0';
        cell_overheat.emit(dut_number, adc_spi_temp);

        while (temp_status == TEMP_STATUS_FAULT)
          vTaskDelay(100);

        continue;
      }

      if (rcvd_char == ' ') {

        cell_overheat.emit(dut_number, adc_spi_temp);
        break;
      }

      if (fsm_state == FSM_SETUP_STATE) {

        shell::hardware["dut_loads_driver"].write("%lf_ma_set_current", 0.0f);
      }

      if (dut_number == DUTS_NUMBER - 1)
        dut_number = 0;
      else
        dut_number++;
    }
  }
}

void calibrate_adcs(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  char received_char = '\0';
  static_cast<signal<void *> *>(shell::callback_signals[USART_CHAR_RECEIVED_EVENT_ID])
      ->connect([&received_char](void *args = nullptr) -> void {
        received_char = *static_cast<char *>(shell::hardware[STD_IO_DRIVER].read(portIO_MAX_DELAY));
      });

  for (unsigned int k = 0; k < output_voltages.size(); k++) {

    shell::hardware[STD_IO_DRIVER].write("\r\nSet input voltage to %lfV and press enter ...\r\n", output_voltages[k]);

    while (received_char != '\r')
      vTaskDelay(100);

    received_char = '\0';
    shell::hardware[STD_IO_DRIVER].write("Calibrating on %lf input voltage ...\r\n", output_voltages[k]);

    for (unsigned int i = 0; i < DUTS_NUMBER; i++) {

      double tmp = 0.0f;
      shell::hardware["fpga_driver"].write("%u_adc_select", i / 2);

      for (double j = 0; j < 8.0f; j += 1.0f) {

        shell::hardware["adc_spi_driver"].write("ch_%u_%u_get_data", i, 0);
        tmp += *static_cast<double *>(shell::hardware["adc_spi_driver"].read(portIO_MAX_DELAY));
      }

      shell::hardware["fpga_driver"].write("%u_adc_deselect", i / 2);
      double measured_voltage = tmp / 8.0f;
      calibration_table[i + k * DUTS_NUMBER] = output_voltages[k] - measured_voltage;
      static_errors_k.insert(std::make_pair(
          "ch_" + std::to_string(i) + "_" + std::to_string(output_voltages[k]) + "_VREF", calibration_table[i]));
      shell::hardware[STD_IO_DRIVER].write(".");
    }
  }

  shell::hardware["adc_spi_driver"].write("write_calibration_table_%p_%u", calibration_table,
                                          DUTS_NUMBER * output_voltages.size());
  shell::hardware["adc_spi_driver"].write("read_calibration_table_size_%u", DUTS_NUMBER * output_voltages.size());
  double *p_readed_table = reinterpret_cast<double *>(
      static_cast<size_t>((*static_cast<double *>(shell::hardware["adc_spi_driver"].read(portIO_MAX_DELAY)))));

  for (unsigned int i = 0; i < DUTS_NUMBER * output_voltages.size(); i++) {

    if (static_errors_k.find("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                             std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF") == static_errors_k.end())
      static_errors_k.insert(std::make_pair("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                                                std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF",
                                            p_readed_table[i]));

    else
      static_errors_k.at("ch_" + std::to_string(i % DUTS_NUMBER) + "_" +
                         std::to_string(output_voltages[i / DUTS_NUMBER]) + "_VREF") = p_readed_table[i];
  }
}

void clear(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  shell::hardware[STD_IO_DRIVER].write(ASCII_CONTROL_ERASE_SCREEN + ASCII_CONTROL_CURSOR_HOME);
}

void test(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  shell::hardware["fpga_driver"].write("%u_red_led_on", 1);
}

void cowsay(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  std::string text;
  for (unsigned int i = 1; i < std::get<0>(*p_args)->size(); i++)
    text += std::get<0>(*p_args)->at(i) + " ";

  if (!text.length())
    text = "What?";

  shell::hardware[STD_IO_DRIVER].write("\r\n");
  shell::hardware[STD_IO_DRIVER].write(" " + std::string(text.length(), '_') + "\r\n");
  shell::hardware[STD_IO_DRIVER].write("(%s)\r\n", text.c_str());
  shell::hardware[STD_IO_DRIVER].write(" " + std::string(text.length(), '-') + "\r\n");
  shell::hardware[STD_IO_DRIVER].write("     \\  ^___^\r\n");
  shell::hardware[STD_IO_DRIVER].write("      \\ (ooo)\\_______\r\n");
  shell::hardware[STD_IO_DRIVER].write("        (___)\\       )~~~\r\n");
  shell::hardware[STD_IO_DRIVER].write("             ||----w |\r\n");
  shell::hardware[STD_IO_DRIVER].write("             ||     ||\r\n");
}

void tasks(std::tuple<std::vector<std::string> *, SemaphoreHandle_t *, shell *> *p_args) {
  char buffer[1024];
  std::memset(buffer, '\0', std::strlen(buffer));
  vTaskList(buffer);
  shell::hardware[STD_IO_DRIVER].write("\r\n%s\r\n", buffer);
}
