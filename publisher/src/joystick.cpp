#include <signal.h>
#include <iostream>
#include <string>
#include <joystick.hh>
#include <popl.hpp>
#include <unistd.h>
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
    auto device_index = op.add<popl::Value<unsigned int>>("d", "device", "device index", 0);
    auto move_axis = op.add<popl::Value<unsigned int>>("", "move-axis", "move axis", 4);
    auto turn_axis = op.add<popl::Value<unsigned int>>("", "turn-axis", "turn axis", 0);
    auto max_move_speed = op.add<popl::Value<float>>("", "move-speed", "max moving speed (m/s)", 0.5);
    auto max_turn_speed = op.add<popl::Value<float>>("", "turn-speed", "max turning speed (rad/s)", 0.5);
    auto key = op.add<popl::Value<std::string>>("", "key", "zenoh publisher key", "remote-control/{device index}");

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

    // Find joystick
    Joystick joystick(device_index->value());

    if (!joystick.isFound())
    {
        std::cerr << "No joystick found." << std::endl;
        return EXIT_FAILURE;
    }

    // Auto-generate publisher key if not set
    if (!key->is_set())
    {
        key->set_value("remote-control/" + std::to_string(device_index->value()));
    }

    // Start zenoh session
    zenoh::Config zenoh_config;
    zenoh_config.insert_json(Z_CONFIG_MODE_KEY, "\"peer\"");
    auto zenoh_session = zenoh::expect(zenoh::open(std::move(zenoh_config)));
    auto zenoh_publisher = zenoh::expect(zenoh_session.declare_publisher(key->value()));

    printf("Publisher key: %s\n", key->value().c_str());
    printf("Press Ctrl+C to exit\n");

    // Publish speeds
    float move_speed = 0.0;
    float turn_speed = 0.0;

    while (!interrupted)
    {
        // Restrict rate
        usleep(1000);

        // Attempt to sample an event from the joystick
        JoystickEvent event;
        bool publish = false;

        if (!joystick.sample(&event))
        {
            continue;
        }

        // Button presses
        if (event.isButton() && event.value == 1)
        {
            printf("Button %u down\n", event.number);
        }

        // Check move axis
        if (event.isAxis() && event.number == move_axis->value())
        {
            move_speed = max_move_speed->value() * -event.value / 32767.0;
            publish = true;
            printf("Move speed: %f\n", move_speed);
        }

        // Check turn axis
        if (event.isAxis() && event.number == turn_axis->value())
        {
            turn_speed = max_turn_speed->value() * event.value / 32767.0;
            publish = true;
            printf("Turn speed: %f\n", turn_speed);
        }

        // Publish speeds
        if (publish)
        {
            printf("move_speed: %f, turn_speed: %f\n", move_speed, turn_speed);
            // Serialize speeds as bytes
            std::vector<uint8_t> speeds(8);
            memcpy(speeds.data(), &move_speed, sizeof(float));
            memcpy(speeds.data() + 4, &turn_speed, sizeof(float));
            zenoh_publisher.put(speeds);
        }
    }

    // Send 0 speeds on exit
    std::vector<uint8_t> speeds(8);
    zenoh_publisher.put(speeds);

    return EXIT_SUCCESS;
}
