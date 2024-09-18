#include <thread>
#include <chrono>
#include <cmath>
#include <MoteusAPI.h>
#include <popl.hpp>

int main(int argc, char *argv[])
{
    // Parse arguments
    popl::OptionParser op("Allowed options");
    auto help = op.add<popl::Switch>("h", "help", "produce help message");
    auto device = op.add<popl::Value<std::string>>("d", "device", "device path", "/dev/ttyACM0");
    auto move_axis = op.add<popl::Value<unsigned int>>("", "move-axis", "move axis", 4);
    auto turn_axis = op.add<popl::Value<unsigned int>>("", "turn-axis", "turn axis", 0);
    auto max_move_speed = op.add<popl::Value<float>>("", "move-speed", "max moving speed (m/s)", 0.5);
    auto max_turn_speed = op.add<popl::Value<float>>("", "turn-speed", "max turning speed (rad/s)", 0.5);

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

    string dev_name("/dev/ttyACM0");
    MoteusAPI servo_left(dev_name, 1);
    MoteusAPI servo_right(dev_name, 2);

    // send one Velocity command
    double stop_position = NAN;
    // double velocity = 0.2;
    double max_torque = 1;
    double feedforward_torque = 0;

    for (int i = 0; i < 300; ++i)
    {
        double t = i * 0.01;
        double velocity = 1 * std::sin(2 * M_PI * 0.5 * t);

        servo_left.SendPositionCommand(stop_position, velocity, max_torque,
                                       feedforward_torque);

        servo_right.SendPositionCommand(stop_position, velocity, max_torque,
                                        feedforward_torque);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    servo_left.SendStopCommand();
    servo_right.SendStopCommand();

    return 0;
}
