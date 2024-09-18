#include <signal.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include <cmath>
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
    auto move_x_axis = op.add<popl::Value<unsigned int>>("", "move-x-axis", "movement x-axis number", 3);
    auto move_y_axis = op.add<popl::Value<unsigned int>>("", "move-y-axis", "movement y-axis number", 4);
    auto turn_axis = op.add<popl::Value<unsigned int>>("", "turn-axis", "turn axis number", 0);
    auto max_move_speed = op.add<popl::Value<float>>("", "move-speed", "max moving speed (m/s)", 0.5);
    auto max_turn_speed = op.add<popl::Value<float>>("", "turn-speed", "max turning speed (rad/s)", 0.5);
    auto key = op.add<popl::Value<std::string>>("", "key", "zenoh key", "rc/{device index}");

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
        std::cerr << "Joystick not found" << std::endl;
        return EXIT_FAILURE;
    }

    // Auto-generate publisher key if not set
    if (!key->is_set())
    {
        key->set_value("rc/" + std::to_string(device_index->value()));
    }

    // Start zenoh session
    zenoh::Config zenoh_config;
    zenoh_config.insert_json(Z_CONFIG_MODE_KEY, "\"peer\"");
    auto zenoh_session = zenoh::expect(zenoh::open(std::move(zenoh_config)));
    auto zenoh_publisher = zenoh::expect(zenoh_session.declare_publisher(key->value()));

    printf("Publisher key: %s\n", key->value().c_str());
    printf("Press Ctrl+C to exit\n");

    // Use mutex to protect shared data
    std::mutex mtx;
    float move_x_speed = 0.0;
    float move_y_speed = 0.0;
    float turn_speed = 0.0;

    // Read joystick events on separate thread
    std::thread joystick_thread([&]()
                                {
        while (!interrupted)
        {
            usleep(1000); // 1ms

            // Attempt to sample an event from the joystick
            JoystickEvent event;

            if (!joystick.sample(&event))
            {
                continue;
            }

            mtx.lock();

            // Button presses
            if (event.isButton() && event.value == 1)
            {
                printf("Button %u pressed\n", event.number);
            }

            // Check move x-axis
            if (event.isAxis() && event.number == move_x_axis->value())
            {
                move_x_speed = max_move_speed->value() * event.value / 32767.0;
            }

            // Check move y-axis
            if (event.isAxis() && event.number == move_y_axis->value())
            {
                move_y_speed = max_move_speed->value() * -event.value / 32767.0;
            }

            // Check turn axis
            if (event.isAxis() && event.number == turn_axis->value())
            {
                turn_speed = max_turn_speed->value() * -event.value / 32767.0;
            }

            mtx.unlock();
        } });

    // Publish speeds on main thread
    std::vector<uint8_t> speeds_message(12);

    while (!interrupted)
    {
        usleep(10000); // 10ms

        mtx.lock();
        memcpy(speeds_message.data(), &move_x_speed, 4);
        memcpy(speeds_message.data() + 4, &move_y_speed, 4);
        memcpy(speeds_message.data() + 8, &turn_speed, 4);
        mtx.unlock();

        zenoh_publisher.put(speeds_message);
    }

    // Wait for joystick thread to finish
    joystick_thread.join();

    // Publish 0 speeds on exit
    std::vector<uint8_t> speeds(12);
    zenoh_publisher.put(speeds);

    return EXIT_SUCCESS;
}
