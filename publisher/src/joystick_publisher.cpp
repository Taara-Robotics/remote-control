#include <iostream>
#include <string>
#include <joystick.hh>
#include <popl.hpp>
#include <unistd.h>

int main(int argc, char *argv[])
{
    // Parse arguments
    popl::OptionParser op("Allowed options");
    auto help = op.add<popl::Switch>("h", "help", "produce help message");
    auto device_index = op.add<popl::Value<unsigned int>>("d", "device", "device index", 0);
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

    // Read joystick events
    Joystick joystick(device_index->value());

    if (!joystick.isFound())
    {
        std::cerr << "No joystick found." << std::endl;
        return EXIT_FAILURE;
    }

    while (true)
    {
        // Restrict rate
        usleep(1000);

        // Attempt to sample an event from the joystick
        JoystickEvent event;

        if (joystick.sample(&event))
        {
            if (event.isButton())
            {
                printf("Button %u is %s\n", event.number, event.value == 0 ? "up" : "down");
            }

            // Check move axis
            if (event.isAxis() && event.number == move_axis->value())
            {
                float move_speed = max_move_speed->value() * -event.value / 32767.0;
                printf("Move speed: %f\n", move_speed);
            }

            // Check turn axis
            if (event.isAxis() && event.number == turn_axis->value())
            {
                float turn_speed = max_turn_speed->value() * event.value / 32767.0;
                printf("Turn speed: %f\n", turn_speed);
            }
        }
    }

    return 0;
}
