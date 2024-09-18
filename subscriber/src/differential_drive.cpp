#include <signal.h>
#include <thread>
#include <chrono>
#include <cmath>
#include <mutex>
#include <MoteusAPI.h>
#include <popl.hpp>
#include <zenoh.hxx>

bool interrupted = false;

void interrupt_handler(int)
{
    interrupted = true;
}

int main(int argc, char *argv[])
{
    // Register interrupt handler
    struct sigaction sig_int_handler;
    sig_int_handler.sa_handler = interrupt_handler;
    sigemptyset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;
    sigaction(SIGINT, &sig_int_handler, NULL);

    // Parse arguments
    popl::OptionParser op("Allowed options");
    auto help = op.add<popl::Switch>("h", "help", "produce help message");
    auto key = op.add<popl::Value<std::string>>("", "key", "zenoh key", "rc/0");
    auto r = op.add<popl::Value<float>>("r", "wheel-radius", "wheel radius (m) for differential drive calculation", 0.08);
    auto b = op.add<popl::Value<float>>("b", "vehicle-width", "distance between wheels (m) for differential drive calculation", 0.31);
    auto device = op.add<popl::Value<std::string>>("d", "device", "device path", "/dev/ttyACM0");
    auto max_move_speed = op.add<popl::Value<float>>("", "max-move-speed", "max moving speed (m/s)", 1.0);
    auto max_turn_speed = op.add<popl::Value<float>>("", "max-turn-speed", "max turning speed (rad/s)", 2.0);
    auto left_motor_id = op.add<popl::Value<unsigned int>>("", "left-motor-id", "left motor ID", 1);
    auto right_motor_id = op.add<popl::Value<unsigned int>>("", "right-motor-id", "right motor ID", 2);
    auto kill_timeout = op.add<popl::Value<unsigned int>>("", "kill-timeout", "stop motors if no commands received for this time (ms)", 250);
    auto stop_threshold = op.add<popl::Value<float>>("", "stop-threshold", "stop motors if move and turn speeds below this value (m/s or rad/s)", 0.025);
    auto max_torque = op.add<popl::Value<float>>("", "max-torque", "Moteus max_torque", 1.0);
    auto feedforward_torque = op.add<popl::Value<float>>("", "feedforward-torque", "Moteus feedforward_torque", 0.0);
    auto kp_scale = op.add<popl::Value<float>>("", "kp-scale", "Moteus kp_scale", 4.0);
    auto kd_scale = op.add<popl::Value<float>>("", "kd-scale", "Moteus kd_scale", 4.0);
    auto motor_speed_multiplier = op.add<popl::Value<float>>("m", "--motor-speed-multiplier", "Multipler to convert wheel rotation speed to motor speed value", 0.67);

    try
    {
        op.parse(argc, argv);
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << std::endl;
        std::cerr << op << std::endl;
        return EXIT_FAILURE;
    }

    if (help->is_set())
    {
        std::cerr << op << std::endl;
        return EXIT_FAILURE;
    }

    // Use mutex for move speed and turn speed
    std::mutex mtx;
    float move_speed = 0.0;
    float turn_speed = 0.0;
    auto last_command_time = std::chrono::steady_clock::now();
    const auto kill_duration = std::chrono::milliseconds(kill_timeout->value());

    // Send motor commands on separate thread
    string dev_name(device->value());
    MoteusAPI left_motor(dev_name, left_motor_id->value());
    MoteusAPI right_motor(dev_name, right_motor_id->value());

    // Send stop command immediately when program is started
    left_motor.SendStopCommand();
    right_motor.SendStopCommand();

    std::thread motors_thread([&]()
                              {
                                  while (!interrupted)
                                  {
                                      usleep(1000); // 1ms

                                      mtx.lock();

                                        // Set speeds to 0 if no commands have been received recently
                                      if (std::chrono::steady_clock::now() - last_command_time > kill_duration)
                                      {
                                        move_speed = 0;
                                        turn_speed = 0;
                                      }
                                      
                                    // Make sure max speeds are not exceeded
                                    if (std::abs(move_speed) > max_move_speed->value()) {
                                        move_speed = move_speed/std::abs(move_speed)*max_move_speed->value();
                                    }

                                    if (std::abs(turn_speed) > max_turn_speed->value()) {
                                        turn_speed = turn_speed/std::abs(turn_speed)*max_turn_speed->value();
                                    }

                                      // Calculate wheel speeds based on r, b
                                      if (std::abs(move_speed) < stop_threshold->value() && std::abs(turn_speed) < stop_threshold->value()) {
                                        left_motor.SendStopCommand();
                                        right_motor.SendStopCommand();
                                      } else {
                                        auto left_wheel_rot_speed = (move_speed - turn_speed * b->value()/2) / r->value();
                                        auto right_wheel_rot_speed = (move_speed + turn_speed * b->value()/2) / r->value();

                                        left_wheel_rot_speed *= motor_speed_multiplier->value();
                                        right_wheel_rot_speed *= motor_speed_multiplier->value();

                                        left_motor.SendPositionCommand(NAN, -left_wheel_rot_speed, max_torque->value(), feedforward_torque->value(), kp_scale->value(), kd_scale->value());
                                        right_motor.SendPositionCommand(NAN, right_wheel_rot_speed, max_torque->value(), feedforward_torque->value(), kp_scale->value(), kd_scale->value());

                                      printf("L: %f, R: %f\n", left_wheel_rot_speed, right_wheel_rot_speed);
                                      }

                                      mtx.unlock();
                                  }

                                  // Stop motors on interrupt
                                  left_motor.SendStopCommand();
                                  right_motor.SendStopCommand(); });

    // Start zenoh session
    zenoh::Config zenoh_config;
    zenoh_config.insert_json(Z_CONFIG_MODE_KEY, "\"peer\"");
    auto zenoh_session = zenoh::expect(zenoh::open(std::move(zenoh_config)));
    auto zenoh_subscriber = zenoh::expect(zenoh_session.declare_subscriber(key->value(), [&](zenoh::Sample sample)
                                                                           {
        // read move speed and turn speed as float from payload
        mtx.lock();
        memcpy(&move_speed, sample.payload.start + 4, 4);
        memcpy(&turn_speed, sample.payload.start + 8, 4);
        last_command_time = std::chrono::steady_clock::now();
        mtx.unlock(); }));

    printf("Subscriber key: %s\n", key->value().c_str());
    printf("Press Ctrl+C to exit\n");

    // Join motors thread on exit
    motors_thread.join();

    return 0;
}
